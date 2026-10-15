#include "linscreendaemon.h"
#include "core/linscreen.h"
#include "tools/pin/pinwidget.h"
#include "utils/abstractlogger.h"
#include "utils/confighandler.h"
#include "utils/globalvalues.h"
#include "utils/screenshotsaver.h"
#include "widgets/capture/capturewidget.h"
#include "widgets/trayicon.h"

#include <QApplication>
#include <QClipboard>
#include <QDataStream>
#include <QIODevice>
#include <QPixmap>
#include <QRect>

#if !(defined(Q_OS_MACOS) || defined(Q_OS_WIN))
#include <QDBusConnection>
#include <QDBusMessage>
#endif

#if !defined(DISABLE_UPDATE_CHECKER)
#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>
#include <QVersionNumber>
#endif

namespace {
constexpr qsizetype maxInterprocessMessageBytes = 64 * 1024 * 1024;
constexpr qint64 maxInterprocessPixmapPixels = 100LL * 1024 * 1024;

bool isValidInterprocessPixmap(const QPixmap& pixmap)
{
    return !pixmap.isNull() && pixmap.width() > 0 && pixmap.height() > 0 &&
           static_cast<qint64>(pixmap.width()) * pixmap.height() <=
             maxInterprocessPixmapPixels;
}

#if !defined(DISABLE_UPDATE_CHECKER)
constexpr qint64 maxUpdateResponseBytes = 1024 * 1024;
constexpr int updateRequestTimeoutMs = 15000;
#endif
}

#if defined(USE_KDSINGLEAPPLICATION) &&                                        \
  (defined(Q_OS_MACOS) || defined(Q_OS_WIN))
#include <QBuffer>
#include <kdsingleapplication.h>
#endif

#ifdef Q_OS_WIN
#include "core/globalshortcutfilter.h"
#endif

/**
 * @brief A way of accessing the linscreen daemon both from the daemon itself,
 * and from subcommands.
 *
 * The daemon is necessary in order to:
 * - Host the system tray,
 * - Listen for hotkey events that will trigger captures,
 * - Host pinned screenshot widgets,
 * - Host the clipboard on X11, where the clipboard gets lost once linscreen
 *   quits.
 *
 * If the `autoCloseIdleDaemon` option is true, the daemon will close as soon as
 * it is not needed to host pinned screenshots and the clipboard.
 *
 * Both the daemon and non-daemon linscreen processes use the same public API,
 * which is implemented as static methods. In the daemon process, this class is
 * also instantiated as a singleton, so it can listen to D-Bus calls via the
 * sigslot mechanism. The instantiation is done by calling `start` (this must be
 * done only in the daemon process). Any instance (as opposed to static) members
 * can only be used if the current process is a daemon.
 *
 * @note The daemon will be automatically launched where necessary, via D-Bus.
 * This applies only to Linux.
 */
LinScreenDaemon::LinScreenDaemon()
  : m_persist(false)
  , m_hostingClipboard(false)
  , m_clipboardSignalBlocked(false)
  , m_trayIcon(nullptr)
#if !defined(DISABLE_UPDATE_CHECKER)
  , m_appLatestVersion(QStringLiteral(APP_VERSION).replace("v", ""))
  , m_showManualCheckAppUpdateStatus(false)
  , m_networkCheckUpdates(nullptr)
#endif
{
    connect(
      QApplication::clipboard(), &QClipboard::dataChanged, this, [this]() {
          if (!m_hostingClipboard || m_clipboardSignalBlocked) {
              m_clipboardSignalBlocked = false;
              return;
          }
          m_hostingClipboard = false;
          quitIfIdle();
      });

    m_persist = !ConfigHandler().autoCloseIdleDaemon();
    connect(ConfigHandler::getInstance(),
            &ConfigHandler::fileChanged,
            this,
            [this]() {
                ConfigHandler config;
                enableTrayIcon(!config.disabledTrayIcon());
                m_persist = !config.autoCloseIdleDaemon();
            });

#if !defined(DISABLE_UPDATE_CHECKER)
    if (ConfigHandler().checkForUpdates()) {
        getLatestAvailableVersion();
    }
#endif
}

void LinScreenDaemon::start()
{
    if (!m_instance) {
        m_instance = new LinScreenDaemon();
        // Tray icon needs LinScreenDaemon::instance() to be non-null
        m_instance->initTrayIcon();
        qApp->setQuitOnLastWindowClosed(false);
    }
}

void LinScreenDaemon::createPin(const QPixmap& capture, QRect geometry)
{
    if (instance()) {
        instance()->attachPin(capture, geometry);
        return;
    }

    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);

#if defined(USE_KDSINGLEAPPLICATION) &&                                        \
  (defined(Q_OS_MACOS) || defined(Q_OS_WIN))
    auto kdsa = KDSingleApplication(QStringLiteral("org.linscreen.LinScreen"));
    stream << QStringLiteral("attachPin") << capture << geometry;
    kdsa.sendMessage(data);
#else
    stream << capture << geometry;
    QDBusMessage m = createMethodCall(QStringLiteral("attachPin"));
    m << data;
    call(m);
#endif
}

void LinScreenDaemon::copyToClipboard(const QPixmap& capture)
{
    if (instance()) {
        instance()->attachScreenshotToClipboard(capture);
        return;
    }

    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);

#if defined(USE_KDSINGLEAPPLICATION) &&                                        \
  (defined(Q_OS_MACOS) || defined(Q_OS_WIN))
    auto kdsa = KDSingleApplication(QStringLiteral("org.linscreen.LinScreen"));
    stream << QStringLiteral("attachScreenshotToClipboard") << capture;
    kdsa.sendMessage(data);
#else
    stream << capture;
    QDBusMessage m =
      createMethodCall(QStringLiteral("attachScreenshotToClipboard"));

    m << data;
    call(m);
#endif
}

void LinScreenDaemon::copyToClipboard(const QString& text,
                                      const QString& notification)
{
    if (instance()) {
        instance()->attachTextToClipboard(text, notification);
        return;
    }

#if defined(USE_KDSINGLEAPPLICATION) &&                                        \
  (defined(Q_OS_MACOS) || defined(Q_OS_WIN))
    auto kdsa = KDSingleApplication(QStringLiteral("org.linscreen.LinScreen"));
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream << QStringLiteral("attachTextToClipboard") << text << notification;
    kdsa.sendMessage(data);
#else
    auto m = createMethodCall(QStringLiteral("attachTextToClipboard"));
    m << text << notification;
    call(m);
#endif
}

/**
 * @brief Is this instance of linscreen hosting any windows as a daemon?
 */
bool LinScreenDaemon::isThisInstanceHostingWidgets()
{
    return instance() && !instance()->m_widgets.isEmpty();
}

void LinScreenDaemon::sendTrayNotification(const QString& text,
                                           const QString& title,
                                           const int timeout)
{
    if (m_trayIcon) {
        m_trayIcon->showMessage(
          title, text, QIcon(GlobalValues::iconPath()), timeout);
    }
}

#if !defined(DISABLE_UPDATE_CHECKER)
void LinScreenDaemon::showUpdateNotificationIfAvailable(CaptureWidget* widget)
{
    if (!m_appLatestUrl.isEmpty() &&
        ConfigHandler().ignoreUpdateToVersion().compare(m_appLatestVersion) <
          0) {
        widget->showAppUpdateNotification(m_appLatestVersion, m_appLatestUrl);
    }
}

void LinScreenDaemon::getLatestAvailableVersion()
{
    // This features is required for MacOS and Windows user and for Linux users
    // who installed LinScreen not from the repository.
    QNetworkRequest requestCheckUpdates(QUrl(LINSCREEN_APP_VERSION_URL));
    requestCheckUpdates.setRawHeader("Accept", "application/vnd.github+json");
    requestCheckUpdates.setRawHeader("User-Agent", "LinScreen-update-checker");
    requestCheckUpdates.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                                     QNetworkRequest::NoLessSafeRedirectPolicy);
    requestCheckUpdates.setTransferTimeout(updateRequestTimeoutMs);
    if (nullptr == m_networkCheckUpdates) {
        m_networkCheckUpdates = new QNetworkAccessManager(this);
        connect(m_networkCheckUpdates,
                &QNetworkAccessManager::finished,
                this,
                &LinScreenDaemon::handleReplyCheckUpdates);
    }
    QNetworkReply* reply = m_networkCheckUpdates->get(requestCheckUpdates);
    reply->setReadBufferSize(maxUpdateResponseBytes + 1);
    connect(reply,
            &QNetworkReply::downloadProgress,
            reply,
            [reply](qint64 received, qint64) {
                if (received > maxUpdateResponseBytes) {
                    reply->abort();
                }
            });

    // check for updates each 24 hours
    QTimer::singleShot(1000 * 60 * 60 * 24, [this]() {
        if (ConfigHandler().checkForUpdates()) {
            this->getLatestAvailableVersion();
        }
    });
}

void LinScreenDaemon::checkForUpdates()
{
    bool autoCheckEnabled = ConfigHandler().checkForUpdates();

    if (autoCheckEnabled) {
        if (!m_appLatestUrl.isEmpty()) {
            QDesktopServices::openUrl(QUrl(m_appLatestUrl));
        }
    } else {
        m_showManualCheckAppUpdateStatus = true;

        if (m_appLatestUrl.isEmpty()) {
            getLatestAvailableVersion();
        } else {
            QVersionNumber appLatestVersion =
              QVersionNumber::fromString(m_appLatestVersion);
            if (LinScreen::instance()->getVersion() < appLatestVersion) {
                QDesktopServices::openUrl(QUrl(m_appLatestUrl));
            } else {
                sendTrayNotification(tr("You have the latest version"),
                                     "LinScreen");
            }
        }
    }
}
#endif

/**
 * @brief Return the daemon instance.
 *
 * If this instance of linscreen is the daemon, a singleton instance of
 * `LinScreenDaemon` is returned. As a side effect`start` will called if it
 * wasn't called earlier. If this instance of linscreen is not the daemon,
 * `nullptr` is returned.
 *
 * This strategy is used because the daemon needs to receive signals from D-Bus,
 * for which an instance of a `QObject` is required. The singleton serves as
 * that object.
 */
LinScreenDaemon* LinScreenDaemon::instance()
{
    // Because we don't use DBus on MacOS, each instance of linscreen is its own
    // mini-daemon, responsible for hosting its own persistent widgets (e.g.
    // pins).
#if defined(Q_OS_MACOS)
    start();
#endif
    return m_instance;
}

/**
 * @brief Quit the daemon if it has nothing to do and the 'persist' flag is not
 * set.
 */
void LinScreenDaemon::quitIfIdle()
{
    if (m_persist) {
        return;
    }
    if (!m_hostingClipboard && m_widgets.isEmpty()) {
        qApp->exit(E_OK);
    }
}

// SERVICE METHODS

void LinScreenDaemon::attachPin(const QPixmap& pixmap, QRect geometry)
{
    auto* pinWidget = new PinWidget(pixmap, geometry);
    m_widgets.append(pinWidget);
    connect(pinWidget, &QObject::destroyed, this, [=, this]() {
        m_widgets.removeOne(pinWidget);
        quitIfIdle();
    });

    pinWidget->show();
    pinWidget->activateWindow();
}

void LinScreenDaemon::attachScreenshotToClipboard(const QPixmap& pixmap)
{
    m_hostingClipboard = true;
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->blockSignals(true);
    // This variable is necessary because the signal doesn't get blocked on
    // windows for some reason
    m_clipboardSignalBlocked = true;
    saveToClipboard(pixmap);
    clipboard->blockSignals(false);
}

// D-BUS / KDSingleApplication METHODS

void LinScreenDaemon::attachPin(const QByteArray& data)
{
    if (data.size() > maxInterprocessMessageBytes) {
        qWarning() << "Rejected oversized pin payload";
        return;
    }
    QDataStream stream(data);
    QPixmap pixmap;
    QRect geometry;

    stream >> pixmap;
    stream >> geometry;

    if (stream.status() != QDataStream::Ok ||
        !isValidInterprocessPixmap(pixmap)) {
        qWarning() << "Rejected invalid pin payload";
        return;
    }
    attachPin(pixmap, geometry);
}

void LinScreenDaemon::attachScreenshotToClipboard(const QByteArray& screenshot)
{
    if (screenshot.size() > maxInterprocessMessageBytes) {
        qWarning() << "Rejected oversized screenshot payload";
        return;
    }
    QDataStream stream(screenshot);
    QPixmap p;
    stream >> p;

    if (stream.status() != QDataStream::Ok || !isValidInterprocessPixmap(p)) {
        qWarning() << "Rejected invalid screenshot payload";
        return;
    }
    attachScreenshotToClipboard(p);
}

void LinScreenDaemon::attachTextToClipboard(const QString& text,
                                            const QString& notification)
{
    // Must send notification before clipboard modification on linux
    if (!notification.isEmpty()) {
        AbstractLogger::info() << notification;
    }

    m_hostingClipboard = true;
    QClipboard* clipboard = QApplication::clipboard();

    clipboard->blockSignals(true);
    // This variable is necessary because the signal doesn't get blocked on
    // windows for some reason
    m_clipboardSignalBlocked = true;
    clipboard->setText(text);
    clipboard->blockSignals(false);
}

void LinScreenDaemon::initTrayIcon()
{
    if (!ConfigHandler().disabledTrayIcon()) {
        enableTrayIcon(true);
    }
#if defined(Q_OS_WIN)
    GlobalShortcutFilter* nativeFilter = new GlobalShortcutFilter(this);
    qApp->installNativeEventFilter(nativeFilter);
#endif
}

void LinScreenDaemon::enableTrayIcon(bool enable)
{
    if (enable) {
        if (m_trayIcon == nullptr) {
            m_trayIcon = new TrayIcon();
        } else {
            m_trayIcon->show();
            return;
        }
    } else if (m_trayIcon) {
        m_trayIcon->hide();
    }
}

#if !defined(DISABLE_UPDATE_CHECKER)
void LinScreenDaemon::handleReplyCheckUpdates(QNetworkReply* reply)
{
    reply->deleteLater();
    if (!ConfigHandler().checkForUpdates() &&
        !m_showManualCheckAppUpdateStatus) {
        return;
    }

    const auto reportFailure = [this](const QString& detail) {
        qWarning() << "Failed to get valid latest-version information:"
                   << detail;
        if (m_showManualCheckAppUpdateStatus && LinScreenDaemon::instance()) {
            LinScreenDaemon::instance()->sendTrayNotification(
              tr("Failed to get information about the latest version."),
              "LinScreen");
        }
    };

    const qint64 contentLength =
      reply->header(QNetworkRequest::ContentLengthHeader).toLongLong();
    const QByteArray body = reply->readAll();
    if (reply->error() == QNetworkReply::NoError &&
        contentLength <= maxUpdateResponseBytes &&
        body.size() <= maxUpdateResponseBytes) {
        QJsonParseError parseError;
        const QJsonDocument response =
          QJsonDocument::fromJson(body, &parseError);
        const QJsonObject json = response.object();
        const QString tagName = json["tag_name"].toString();
        const QString latestVersion =
          tagName.startsWith(QChar('v')) ? tagName.mid(1) : tagName;
        const QVersionNumber appLatestVersion =
          QVersionNumber::fromString(latestVersion);
        const QUrl releaseUrl(json["html_url"].toString());
        const bool validVersion = !appLatestVersion.isNull() &&
                                  appLatestVersion.toString() == latestVersion;
        const bool validUrl =
          releaseUrl.scheme() == QLatin1String("https") &&
          releaseUrl.host() == QLatin1String("github.com") &&
          releaseUrl.path() ==
            QLatin1String("/aurelson101/linscreen/releases/tag/") + tagName;

        if (parseError.error != QJsonParseError::NoError ||
            !response.isObject() || !validVersion || !validUrl) {
            reportFailure(parseError.error == QJsonParseError::NoError
                            ? QStringLiteral("unexpected response fields")
                            : parseError.errorString());
        } else if (LinScreen::instance()->getVersion() < appLatestVersion) {
            m_appLatestVersion = latestVersion;
            m_appLatestUrl = releaseUrl.toString();
            emit newVersionAvailable(appLatestVersion);
            if (m_showManualCheckAppUpdateStatus) {
                QDesktopServices::openUrl(QUrl(m_appLatestUrl));
            }
        } else if (m_showManualCheckAppUpdateStatus) {
            sendTrayNotification(tr("You have the latest version"),
                                 "LinScreen");
        }
    } else {
        reportFailure(reply->errorString());
    }
    m_showManualCheckAppUpdateStatus = false;
}
#endif

#if !(defined(Q_OS_MACOS) || defined(Q_OS_WIN))
QDBusMessage LinScreenDaemon::createMethodCall(const QString& method)
{
    QDBusMessage m =
      QDBusMessage::createMethodCall(QStringLiteral("org.linscreen.LinScreen"),
                                     QStringLiteral("/"),
                                     QLatin1String(""),
                                     method);
    return m;
}

void LinScreenDaemon::checkDBusConnection(const QDBusConnection& connection)
{
    if (!connection.isConnected()) {
        AbstractLogger::error() << tr("Unable to connect via DBus");
        qApp->exit(E_DBUSCONN);
    }
}

void LinScreenDaemon::call(const QDBusMessage& m)
{
    QDBusConnection sessionBus = QDBusConnection::sessionBus();
    checkDBusConnection(sessionBus);
    sessionBus.call(m);
}
#endif

#if defined(USE_KDSINGLEAPPLICATION) &&                                        \
  (defined(Q_OS_MACOS) || defined(Q_OS_WIN))
void LinScreenDaemon::messageReceivedFromSecondaryInstance(
  const QByteArray& message)
{
    if (message.size() > maxInterprocessMessageBytes) {
        qWarning() << "Rejected oversized message from secondary instance";
        return;
    }
    // qDebug() << "Received message from second instance:" << message;

    QByteArray messageCopy = message;
    QBuffer buffer(&messageCopy);
    buffer.open(QIODevice::ReadOnly);
    QDataStream stream(&buffer);
    QString methodCall;
    stream >> methodCall;
    // qDebug() << "Method:" << methodCall;

    if (methodCall == QStringLiteral("attachPin")) {
        QPixmap capture;
        QRect geometry;
        stream >> capture >> geometry;
        // qDebug() << "Pixmap:" << capture;
        // qDebug() << "Geometry:" << geometry;
        if (stream.status() == QDataStream::Ok &&
            isValidInterprocessPixmap(capture)) {
            LinScreenDaemon::instance()->attachPin(capture, geometry);
        } else {
            qWarning() << "Received \"attachPin\" from second instance, but "
                          "pixmap is empty!";
        }
    } else if (methodCall == QStringLiteral("attachScreenshotToClipboard")) {
        QPixmap capture;
        stream >> capture;
        // qDebug() << "Pixmap:" << capture;
        if (stream.status() == QDataStream::Ok &&
            isValidInterprocessPixmap(capture)) {
            LinScreenDaemon::instance()->attachScreenshotToClipboard(capture);
        } else {
            qWarning() << "Received \"attachScreenshotToClipboard\" from "
                          "second instance, but pixmap is empty!";
        }
    } else if (methodCall == (QStringLiteral("attachTextToClipboard"))) {
        QString text;
        QString notification;
        stream >> text >> notification;
        // qDebug() << "Text:" << text;
        // qDebug() << "Notification:" << notification;
        if (stream.status() == QDataStream::Ok && !text.isEmpty() &&
            text.size() <= 1024 * 1024 && notification.size() <= 16 * 1024) {
            LinScreenDaemon::instance()->attachTextToClipboard(text,
                                                               notification);
        } else {
            qWarning() << "Received \"attachTextToClipboard\" from second "
                          "instance, but text is empty!";
        }
    } else {
        qWarning() << "Received unknown message from second instance:"
                   << message;
    }
}
#endif

// STATIC ATTRIBUTES
LinScreenDaemon* LinScreenDaemon::m_instance = nullptr;
