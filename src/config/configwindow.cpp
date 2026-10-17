// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2017-2019 Alejandro Sirgo Rica & Contributors

#include "configwindow.h"
#include "config/configresolver.h"
#include "config/filenameeditor.h"
#include "config/generalconf.h"
#include "config/shortcutswidget.h"
#include "config/visualseditor.h"
#include "core/qguiappcurrentscreen.h"
#include "utils/colorutils.h"
#include "utils/confighandler.h"
#include "utils/globalvalues.h"
#include "utils/pathinfo.h"

#include <QApplication>
#include <QDialogButtonBox>
#include <QFileSystemWatcher>
#include <QIcon>
#include <QKeyEvent>
#include <QLabel>
#include <QScreen>
#include <QScrollArea>
#include <QSizePolicy>
#include <QStyle>
#include <QTabBar>
#include <QTextStream>
#include <QVBoxLayout>

// ConfigWindow contains the menus where you can configure the application

ConfigWindow::ConfigWindow(QWidget* parent)
  : QWidget(parent)
{
    // We wrap QTabWidget in a QWidget because of a Qt bug
    auto* layout = new QVBoxLayout(this);
    const int margin = style()->pixelMetric(QStyle::PM_LayoutLeftMargin);
    const int spacing = style()->pixelMetric(QStyle::PM_LayoutVerticalSpacing);
    layout->setContentsMargins(margin, margin, margin, margin);
    layout->setSpacing(spacing);
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->tabBar()->setUsesScrollButtons(true);
    m_tabWidget->tabBar()->setElideMode(Qt::ElideRight);
    m_tabWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
#if defined(Q_OS_MACOS)
    // Fix Qt6 macOS bug where tab pane content renders behind the tab bar
    m_tabWidget->setStyleSheet(
      "QTabWidget::pane { border-top: 2px solid palette(mid); }");
#endif
    layout->addWidget(m_tabWidget);

    setAttribute(Qt::WA_DeleteOnClose);
    setWindowIcon(QIcon(GlobalValues::iconPath()));
    setWindowTitle(tr("Configuration"));

    if (QScreen* screen = QGuiAppCurrentScreen().currentScreen()) {
        const QSize available = screen->availableGeometry().size();
        const QSize maximum(qMax(320, available.width() - 48),
                            qMax(280, available.height() - 48));
        setMinimumSize(qMin(440, maximum.width()), qMin(360, maximum.height()));
        resize(qMin(860, maximum.width()), qMin(680, maximum.height()));
    }

    connect(ConfigHandler::getInstance(),
            &ConfigHandler::fileChanged,
            this,
            &ConfigWindow::updateChildren);

    QColor background = this->palette().window().color();
    bool isDark = ColorUtils::colorIsDark(background);
    QString modifier =
      isDark ? PathInfo::whiteIconPath() : PathInfo::blackIconPath();

    // general
    m_generalConfig = new GeneralConf();
    m_generalConfigTab = new QWidget();
    auto* generalConfigLayout = new QVBoxLayout(m_generalConfigTab);
    m_generalConfigTab->setLayout(generalConfigLayout);
    generalConfigLayout->addWidget(m_generalConfig);
    m_tabWidget->addTab(
      m_generalConfigTab, QIcon(modifier + "config.svg"), tr("General"));

    // visuals
    m_visuals = new VisualsEditor();
    m_visualsTab = new QWidget();
    auto* visualsLayout = new QVBoxLayout(m_visualsTab);
    m_visualsTab->setLayout(visualsLayout);
    visualsLayout->addWidget(m_visuals);
    m_tabWidget->addTab(
      m_visualsTab, QIcon(modifier + "graphics.svg"), tr("Interface"));

    // filename
    m_filenameEditor = new FileNameEditor();
    m_filenameEditorTab = new QWidget();
    auto* filenameEditorLayout = new QVBoxLayout(m_filenameEditorTab);
    m_filenameEditorTab->setLayout(filenameEditorLayout);
    filenameEditorLayout->addWidget(m_filenameEditor);
    m_tabWidget->addTab(m_filenameEditorTab,
                        QIcon(modifier + "name_edition.svg"),
                        tr("Filename Editor"));

    // shortcuts
    m_shortcuts = new ShortcutsWidget();
    m_shortcutsTab = new QWidget();
    auto* shortcutsLayout = new QVBoxLayout(m_shortcutsTab);
    m_shortcutsTab->setLayout(shortcutsLayout);
    shortcutsLayout->addWidget(m_shortcuts);
    m_tabWidget->addTab(
      m_shortcutsTab, QIcon(modifier + "shortcut.svg"), tr("Shortcuts"));

    // connect update sigslots
    connect(this,
            &ConfigWindow::updateChildren,
            m_filenameEditor,
            &FileNameEditor::updateComponents);
    connect(this,
            &ConfigWindow::updateChildren,
            m_visuals,
            &VisualsEditor::updateComponents);
    connect(this,
            &ConfigWindow::updateChildren,
            m_generalConfig,
            &GeneralConf::updateComponents);

    // Error indicator (this must come last)
    initErrorIndicator(m_visualsTab, m_visuals);
    initErrorIndicator(m_filenameEditorTab, m_filenameEditor);
    initErrorIndicator(m_generalConfigTab, m_generalConfig);
    initErrorIndicator(m_shortcutsTab, m_shortcuts);

    // Keep every setting reachable when the screen is smaller than a page.
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        QWidget* page = m_tabWidget->widget(i);
        const QIcon icon = m_tabWidget->tabIcon(i);
        const QString title = m_tabWidget->tabText(i);
        m_tabWidget->removeTab(i);
        auto* scrollArea = new QScrollArea(m_tabWidget);
        scrollArea->setFrameShape(QFrame::NoFrame);
        scrollArea->setWidgetResizable(true);
        scrollArea->setWidget(page);
        m_tabWidget->insertTab(i, scrollArea, icon, title);
    }
}

void ConfigWindow::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape) {
        close();
    }
}

void ConfigWindow::initErrorIndicator(QWidget* tab, QWidget* widget)
{
    auto* label = new QLabel(tab);
    auto* btnResolve = new QPushButton(tr("Resolve"), tab);
    auto* btnLayout = new QHBoxLayout();

    // Set up label
    label->setText(tr(
      "<b>Configuration file has errors. Resolve them before continuing.</b>"));
    label->setStyleSheet(QStringLiteral(":disabled { color: %1; }")
                           .arg(qApp->palette().color(QPalette::Text).name()));
    label->setVisible(ConfigHandler().hasError());

    // Set up "Show errors" button
    btnResolve->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    btnLayout->addWidget(btnResolve);
    btnResolve->setVisible(ConfigHandler().hasError());

    widget->setEnabled(!ConfigHandler().hasError());

    // Add label and button to the parent widget's layout
    auto* layout = static_cast<QBoxLayout*>(tab->layout());
    if (layout != nullptr) {
        layout->insertWidget(0, label);
        layout->insertLayout(1, btnLayout);
    } else {
        widget->layout()->addWidget(label);
        widget->layout()->addWidget(btnResolve);
    }

    // Sigslots
    connect(
      ConfigHandler::getInstance(), &ConfigHandler::error, widget, [=, this]() {
          widget->setEnabled(false);
          label->show();
          btnResolve->show();
      });
    connect(ConfigHandler::getInstance(),
            &ConfigHandler::errorResolved,
            widget,
            [=]() {
                widget->setEnabled(true);
                label->hide();
                btnResolve->hide();
            });
    connect(btnResolve, &QPushButton::clicked, this, [this]() {
        ConfigResolver().exec();
    });
}
