#include "history.h"
#include "utils/confighandler.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QSettings>
#include <QStringList>

namespace {
constexpr QFileDevice::Permissions privateDirectoryPermissions =
  QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner;
constexpr QFileDevice::Permissions privateFilePermissions =
  QFileDevice::ReadOwner | QFileDevice::WriteOwner;
}

History::History()
{
    // Get cache history path
    ConfigHandler config;
#ifdef Q_OS_WIN
    m_historyPath = QDir::homePath() + "/AppData/Roaming/linscreen/history/";
#else
    QString cachepath = QProcessEnvironment::systemEnvironment().value(
      "XDG_CACHE_HOME", QDir::homePath() + "/.cache");
    m_historyPath = cachepath + "/linscreen/history/";
#endif

    // Check if directory for history exists and create if doesn't
    QDir dir = QDir(m_historyPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
#ifndef Q_OS_WIN
    QFile::setPermissions(m_historyPath, privateDirectoryPermissions);
#endif
}

const QString& History::path()
{
    return m_historyPath;
}

void History::save(const QPixmap& pixmap, const QString& fileName)
{
    // scale preview only in local disk
    QPixmap pixmapScaled = QPixmap(pixmap);
    if (pixmap.height() / HISTORYPIXMAP_MAX_PREVIEW_HEIGHT >=
        pixmap.width() / HISTORYPIXMAP_MAX_PREVIEW_WIDTH) {
        pixmapScaled = pixmap.scaledToHeight(HISTORYPIXMAP_MAX_PREVIEW_HEIGHT,
                                             Qt::SmoothTransformation);
    } else {
        pixmapScaled = pixmap.scaledToWidth(HISTORYPIXMAP_MAX_PREVIEW_WIDTH,
                                            Qt::SmoothTransformation);
    }

    // save preview
    QFile file(path() + fileName);
    if (file.open(QIODevice::WriteOnly)) {
        pixmapScaled.save(&file, "PNG");
        file.close();
#ifndef Q_OS_WIN
        QFile::setPermissions(file.fileName(), privateFilePermissions);
#endif
    }

    history();
}

void History::remove(const QString& fileName)
{
    const QString baseName = QFileInfo(fileName).fileName();
    QFile(path() + baseName).remove();
    removeToken(baseName);
}

const QList<QString>& History::history()
{
    QDir directory(path());
    QStringList images = directory.entryList(QStringList() << "*.png"
                                                           << "*.PNG",
                                             QDir::Files,
                                             QDir::Time);
    int cnt = 0;
    int max = ConfigHandler().uploadHistoryMax();
    m_thumbs.clear();
    for (const auto& fileName : images) {
        if (++cnt <= max) {
            m_thumbs.append(fileName);
        } else {
            remove(fileName);
        }
    }
    return m_thumbs;
}

const HistoryFileName& History::unpackFileName(const QString& fileNamePacked)
{
    const QString baseName = QFileInfo(fileNamePacked).fileName();
    const int firstSeparator = baseName.indexOf('-');
    m_unpackedFileName = {};
    if (firstSeparator < 0) {
        m_unpackedFileName.file = baseName;
        return m_unpackedFileName;
    }

    m_unpackedFileName.type = baseName.left(firstSeparator);
    m_unpackedFileName.token = tokenFor(baseName);

    if (baseName.mid(firstSeparator, 2) == QLatin1String("--")) {
        m_unpackedFileName.file = baseName.mid(firstSeparator + 2);
        return m_unpackedFileName;
    }

    m_unpackedFileName.file = baseName.mid(firstSeparator + 1);

    // Backward compatibility for history created before tokens were moved
    // out of filenames. Imgur names and delete hashes do not contain '-'.
    if (m_unpackedFileName.token.isEmpty()) {
        const int legacySeparator = baseName.indexOf('-', firstSeparator + 1);
        if (legacySeparator > firstSeparator) {
            m_unpackedFileName.token = baseName.mid(
              firstSeparator + 1, legacySeparator - firstSeparator - 1);
            m_unpackedFileName.file = baseName.mid(legacySeparator + 1);
        }
    }
    return m_unpackedFileName;
}

const QString& History::packFileName(const QString& storageType,
                                     const QString& deleteToken,
                                     const QString& fileName)
{
    m_packedFileName = fileName;
    if (storageType.length() > 0) {
        m_packedFileName = storageType + "--" + m_packedFileName;
        if (!deleteToken.isEmpty()) {
            storeToken(m_packedFileName, deleteToken);
        }
    }
    return m_packedFileName;
}

QString History::tokenKey(const QString& fileName) const
{
    return QString::fromLatin1(QFileInfo(fileName).fileName().toUtf8().toBase64(
      QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}

QString History::tokenFor(const QString& fileName) const
{
    QSettings settings(m_historyPath + "upload-tokens.ini",
                       QSettings::IniFormat);
    return settings.value("tokens/" + tokenKey(fileName)).toString();
}

void History::storeToken(const QString& fileName, const QString& token) const
{
    QSettings settings(m_historyPath + "upload-tokens.ini",
                       QSettings::IniFormat);
    settings.setValue("tokens/" + tokenKey(fileName), token);
    settings.sync();
#ifndef Q_OS_WIN
    QFile::setPermissions(settings.fileName(), privateFilePermissions);
#endif
}

void History::removeToken(const QString& fileName) const
{
    QSettings settings(m_historyPath + "upload-tokens.ini",
                       QSettings::IniFormat);
    settings.remove("tokens/" + tokenKey(fileName));
    settings.sync();
#ifndef Q_OS_WIN
    if (QFile::exists(settings.fileName())) {
        QFile::setPermissions(settings.fileName(), privateFilePermissions);
    }
#endif
}
