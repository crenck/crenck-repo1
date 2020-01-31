#include "P4AdminApplication.h"

#include "ActionStore.h"
#include "ActionUtil.h"
#include "AdminAboutBox.h"
#include "AdminCmd.h"
#include "AdminCommandHandler.h"
#include "AdminWindow.h"
#include "Application.h"
#include "CmdAction.h"
#include "CmdActionGroup.h"
#include "CommandIds.h"
#include "ConnectionPane.h"
#include "DebugLog.h"
#include "DialogUtil.h"
#include "GuiWindowManager.h"
#include "LanguageWidget.h"
#include "LogUtil.h"
#include "ObjectCommandGlobals.h"
#include "ObjectDumper.h"
#include "OpContext.h"
#include "Platform.h"
#include "PMessageBox.h"
#include "Prefs.h"
#include "SettingKeys.h"
#include "StyleSheetUtil.h"
#include "UIAdminPreferencesDialog.h"
#include "UIConnectionInfoDialog.h"
#include "XMLManager.h"
#include "XMLProperties.h"
#include "Workspace.h"
#include "WorkspaceId.h"
#include "WorkspaceManager.h"
#include "WorkspaceScopeName.h"

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QKeyEvent>
#include <QLayout>
#include <QLabel>
#include <QLibraryInfo>
#include <QLocale>
#include <QMenuBar>
#include <QPushButton>
#include <QResource>
#include <QStyle>
#include <QStyleFactory>
#include <QString>
#include <QTextEdit>
#include <QTextStream>
#include <QTime>
#include <QTimer>
#include <QTranslator>
#include <QWidget>

#if defined (Q_OS_OSX) || defined (Q_OS_WIN)
#define GUI_APPLICATION
#endif

#define DEFAULT_WINDOW_WIDTH 500

int globalMenuBarHeight()
{
    return 20;
}

// Do not call this often as GetAppPref is super expensive
static int refreshRate()
{
    return 60 * 1000 * Prefs::Value(Prefs::WS(QString(), p4v::prefs::RefreshRate), p4v::Preferences::Defaults()[p4v::prefs::RefreshRate].toInt());
}

static CmdAction *
NewGlobalAction( const StringID & id, const CmdTargetDesc & t,
    const QString & text = QString() )
{
    CmdAction * a = ActionStore::Global().createAction( id );
    a->setTargetDescription( t );
    a->setDefaultMenuText( text );
    return a;
};

static void
p4adminKeyMaker(const QString &workspaceScope, QString *key, QString *filename)
{
    if (workspaceScope.isEmpty())
    {
            *filename = "ApplicationSettings.xml";
    }
    else
    {
            *filename = "ConnectionSettings.xml";
            *key = workspaceScope + "/" + *key;
    }
}


P4AdminApplication::P4AdminApplication(int &argc, char **argv)
: QtSingleApplication(argc, argv)
, Responder( this )
, mInRefreshData(false)
, mRefreshRate(0)
{
    mHelpLauncher = NULL;
    mAdminCommandHandler = NULL;
    mAdminWindow = NULL;
    setObjectName( "P4Admin Application" );

    Prefs::Path::SetWorkspaceKeyMaker(&p4adminKeyMaker);

/* This code segment should be replaced with a call to Platform::AppConfigDir() when available */
    QString configPath = QDir::home().absoluteFilePath( ".p4admin" );
    if ( Platform::IsMac() )
    {
        QDir tempDir = QDir::home();
        tempDir.cd( "Library" );
        tempDir.cd( "Preferences" );
        configPath = tempDir.absoluteFilePath( "com.perforce.p4admin" );

        qApp->setQuitOnLastWindowClosed( false );
    }
/* end platform code */

    // ensure the application's config/log dir exists
    QDir configDir(configPath);
    if (!configDir.exists())
        configDir.mkpath(configPath);

    // set this before the ::init call below, or you get extra config files (job036718)
    XMLManager::setConfigDir( configPath );

    // NOTE:  there is an order in which these have to be called
    // for translations and the application language to work:
    //
    // Platform::SetApplicationName
    // XMLManager::setConfigDir
    // LanguageWidget::LanguagePref() - needs preferences to be configured
    // installTranslator
    // loadGlobalActions
    //
    // For example, if loadGlobalActions is called before installTranslator
    // none of the menus will be translated.

    // Translation files
    QString lang = LanguageWidget::LanguagePref();
    QString translationDir = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
    QTranslator* translator = new QTranslator(0);
    translator->setObjectName( "p4admin translator" );
    if(translator->load( "p4admin_" + lang, translationDir ))
        qApp->installTranslator( translator );
    else
        delete translator;

    // Install Qt translations for context menus and File/Font picker dialogs
    QTranslator* qtTranslator = new QTranslator(0);
    if(qtTranslator->load( "qt_" + lang, translationDir  ) )
        qApp->installTranslator( qtTranslator );
    else
        delete qtTranslator;

    qApp->setStyle( QStyleFactory::create("fusion") );
    
    QString filename( Platform::GetResourcePath("p4vstyle.qss") );
    QFile styleSheetFile( filename );
    if (styleSheetFile.exists())
    {
        styleSheetFile.open( QIODevice::ReadOnly | QIODevice::Text );
        QByteArray styleSheetData = styleSheetFile.readAll();
        QString styleSheet( styleSheetData.constData() );
        
        qApp->setStyleSheet( styleSheet );
    }
    else
    {
        qApp->setStyleSheet( StyleSheetUtil::builtin() );
    }
    
    qApp->setProperty("StyleSheetLoaded", QVariant(true));
// End off EasterEgg

    if (!QResource::registerResource(Platform::ImageResourceFile()))
    {
        PMessageBox::critical(0, Platform::ApplicationName(),
            tr("Error initializing %1.\nUnable to load resource file.").arg(Platform::ApplicationName()));
        qWarning("error loading resource file");
    }

    // Needs to be called after the QTranslator is setup
    loadGlobalActions();
    // create single, global Mac menubar
    Platform::GlobalMenuBar();

    qApp->setStartDragDistance(12);
    ObjectDumper::InstallOnApplication();

    Gui::LogFormatter fmt;
    mAdminShellApp = new AdminShellApplication(configPath, fmt);
    Gui::Application::dApp = mAdminShellApp;
    mWorkspace = new Gui::Workspace(Gui::WorkspaceScopeName());
    mWorkspace->windowManager()->setApplicationIcon(mAdminShellApp->appIcon());
    mAdminShellApp->init();
    Gui::WorkspaceId wsId(objectName());
    Gui::WorkspaceManager::Instance()->insertWorkspace(wsId, mWorkspace);
    mAdminShellApp->loadGlobalActions();

    // Admin should always support streams depot creation
    Op::Context::SetStreamsFeatureEnabled(true);

    qLogConfigureAndMonitor(configPath + "/adminlog.config");
    initFileLogging();

    mAdminCommandHandler = new AdminCommandHandler();
    installEventFilter(this);

    // listen for refresh rate change
    Prefs::AddListener(Prefs::WS(QString(), p4v::prefs::RefreshRate),
                       this,
                       SLOT     (onPreferencesChanged()));

    AdminShellApplication* adminApp = static_cast<AdminShellApplication*>(Gui::Application::dApp);
    connect(adminApp, &AdminShellApplication::emitHandleMessage,
             this,    &P4AdminApplication::handleMessage);

    // kick off the initial timer
    startRefreshTimer(refreshRate());    // kick off the refresh process
}

P4AdminApplication::~P4AdminApplication()
{
    removeEventFilter(this);
    delete mHelpLauncher;
    delete mAdminCommandHandler;
    if (Platform::IsMac())
    {
        // AdminWindow has DeleteOnClose = false
        delete mAdminWindow;
        mAdminWindow = NULL;
    }
	
    delete mAdminShellApp;
    Gui::Application::dApp = 0;

    Prefs::Flush();
}

void
P4AdminApplication::handleMessage(const QString &message)
{
    if (mAdminWindow)
        mAdminWindow->handleApplicationMessage(message);
}

/*!
    \brief  Launch the AdminWindow using the parameters
    to build the initial configuration.  If none are
    provided, display the dialog to choose a connection.

    \return Status of start-up configuration; false == no config
*/
bool
P4AdminApplication::run()
{
    Op::Configuration config =
        Op::Configuration::CreateFromArgumentList( arguments() );

    bool restoreConnections = Prefs::Value(Prefs::FQ(p4admin::prefs::RestoreConnections), true);
    if (restoreConnections)
    {
        const QStringList priorConnections = Prefs::Value(Prefs::FQ(p4v::prefs::PriorConnectionList),
                                                          QStringList());
        if (priorConnections.isEmpty())
            restoreConnections = false;
        else if ( config.isEmpty() )
        {
            const QString lastSelected = Prefs::Value(Prefs::FQ(p4admin::prefs::SelectedConnection), QString());
            config = Op::Configuration::CreateFromAdminString(lastSelected);
        }
    }

    // In case some parameter values are not specified on cmd line,
    // fill in remaining settings from default configuration.
    if (!config.isEmpty())
    {
        Op::Configuration defaultConfig =
            Op::Configuration::DefaultConfiguration();
        if (config.user().isEmpty())
            config.setUser( defaultConfig.user() );
        if (config.port().isEmpty())
            config.setPort( defaultConfig.port() );
        if (config.charset().isEmpty()) // check enviro
            config.setCharset( defaultConfig.charset() );
        if (config.charset().isEmpty()) // use platform default
            config.setCharset( Platform::GetDefaultCharSet() );
    }
    else if (!restoreConnections)
    {
        // display dialog if nothing else is specified
        config = AdminWindow::OpenConfigurationDialog(NULL);
    }

    // global workspace is empty and disconnected
    mWorkspace->init(Op::Configuration(), true);
    // create the admin window set to the config
    createAdminWindow(config);
    return true;
}

/*!
    \brief  Reads file logging prefs, and enables/disables accordingly

    \returns true if logging is enabled, and false if not
*/
void
P4AdminApplication::initFileLogging()
{
    const QString logFile = Gui::Application::dApp->getDefaultLogFileName();
    LogUtil::Configure(Prefs::Value(Prefs::FQ(p4v::prefs::LogToFile), p4v::Preferences::Defaults()[p4v::prefs::LogToFile].toBool()),
                       QString(),   // No config file.
                       Prefs::Value(Prefs::FQ(p4v::prefs::LogFileName), logFile),
                       Prefs::Value(Prefs::FQ(p4v::prefs::LogFileSize), p4v::Preferences::Defaults()[p4v::prefs::LogFileSize].toInt()) * 1024);
}

void
P4AdminApplication::setPrefHelpLocation(QString helpdir)
{
    Prefs::SetValue(Prefs::FQ(p4v::prefs::HelpIndex), helpdir);
}

QString
P4AdminApplication::realApplicationDirPath()
{
    // normally, QApplication gets it right
    QString  appPath = qApp->applicationDirPath ();
    appPath = QDir::toNativeSeparators (appPath);

    return appPath;
}

    // admin and merge are a little trickier since they don't have a single server
    // connection. For now, we're just going to go to online help.
HelpLauncher *
P4AdminApplication::helpWindow()
{
    if (!mHelpLauncher)
        mHelpLauncher = new HelpLauncher("p4admin");

    QString countryCode = Prefs::Value(Prefs::FQ(p4v::prefs::ApplicationLanguage), QString("en"));

    mHelpLauncher->setHelpLocation(kOnlineHelpPrefix);
    mHelpLauncher->setLocale(countryCode);

    return mHelpLauncher;
}

void
P4AdminApplication::startRefreshTimer(int rate)
{
    // cancel existing refresh timers
    emit stopRefreshTimer();

    // no refresh when <= 0
    if (rate <= 0)
        return;

    // rate is in ms, 1 minute minimum
    if (rate < 60000) // no less than one minute
        rate = 60000;

    // build a single short timer
    QTimer* alertTimer = new QTimer(this);
    alertTimer->setSingleShot(true);
    connect(alertTimer, &QTimer::timeout,
            this,       &P4AdminApplication::handleRefreshData);
    // make so we can cancel, or we get leftover refreshes
    connect(this,       &P4AdminApplication::stopRefreshTimer,
            alertTimer, &QTimer::stop);
    alertTimer->start(rate);
}

void
P4AdminApplication::handleRefreshData()
{
    int rate = refreshRate();
    if (rate <= 0)
        return;
    mInRefreshData = true;
    refreshAll(QVariant("NotApplets"));
    startRefreshTimer(rate);
    mInRefreshData = false;
}

/*!
    \brief  Custom event filter to catch context-sensitive help key
*/
bool
P4AdminApplication::eventFilter(QObject* obj, QEvent* e)
{
    // check for FocusEvents, etc.
    if (mAdminShellApp->eventFilter(obj, e))
        return true;

    if (e->type() == QEvent::KeyPress) {
        if (((QKeyEvent *)e)->key() == Qt::Key_Help)
        {
            HelpLauncher* helper = helpWindow();
            if ( !helper )
                return true;    // to avoid recursion

            // This returns false if it fails to find a page
            // in which case we try again on its parent.
            return helper->showHelp(obj);
        }
    }
    return false;
}

void
P4AdminApplication::onPreferencesChanged()
{
    // check for server refresh time change
    if (!mInRefreshData)
    {
        // check if the refresh rate changed
        int rate = refreshRate();
        if (rate != mRefreshRate)
        {
            mRefreshRate = rate;
            startRefreshTimer(rate);
        }
    }
}

void
P4AdminApplication::loadGlobalActions()
{
    CmdAction * a = NULL;
    CmdTargetDesc APP_TARGET( qApp, CmdTargetDesc::HolderIsTheTarget );
    CmdTargetDesc GLOBAL_FOCUS_TARGET( qApp, CmdTargetDesc::FocusWidget );

    a = NewGlobalAction( Cmd_AdminOpenConnection, APP_TARGET, tr("&Open Connection...") );
    a->setShortcut( QKeySequence(QKeySequence::Open) );
    a->setMenuRole( QAction::NoRole );

    a = NewGlobalAction( Cmd_AdminAbout, APP_TARGET, tr("&About %1").arg(Platform::ApplicationName()) );
    a->setMenuRole( QAction::AboutRole );

    a = NewGlobalAction( Cmd_Prefs, APP_TARGET, tr("Pre&ferences...") );
    a->setMenuRole( QAction::PreferencesRole );

    a = NewGlobalAction( Cmd_Quit, APP_TARGET, tr("E&xit") );
    a->setMenuRole( QAction::QuitRole );

    a = NewGlobalAction( Cmd_Help, APP_TARGET, tr("%1 &Help").arg(Platform::ApplicationName()) );
    if ( Platform::IsMac() )
    {
        if ( beforeLeopard() )
            a->setShortcut( QKeySequence(Qt::CTRL + Qt::Key_Question) );
        else
            a->setShortcut( QKeySequence(Qt::ALT + Qt::CTRL + Qt::Key_Question) );
        a->setMenuRole( QAction::NoRole );
    }
    else
    {
        a->setShortcut( QKeySequence(Qt::Key_F1) );
        a->setMenuRole( QAction::NoRole );
    }

    if (QApplication::clipboard()->supportsFindBuffer())
    {
        a = NewGlobalAction( Cmd_CopyToFind, GLOBAL_FOCUS_TARGET,
            tr("Us&e Selection for Find") );
        a->setShortcut( QKeySequence(Qt::CTRL + Qt::Key_E) );
    }

    a = NewGlobalAction( Cmd_Cut, GLOBAL_FOCUS_TARGET, tr("Cu&t") );
    a->setShortcut( QKeySequence(QKeySequence::Cut) );
    a->setMenuRole( QAction::NoRole );

    a = NewGlobalAction( Cmd_Paste, GLOBAL_FOCUS_TARGET, tr("&Paste") );
    a->setShortcut( QKeySequence(QKeySequence::Paste) );
    a->setMenuRole( QAction::NoRole );

    a = NewGlobalAction( Cmd_Clear, GLOBAL_FOCUS_TARGET, tr("Clear") );
    a->setMenuRole( QAction::NoRole );

    a = NewGlobalAction( Cmd_Undo, GLOBAL_FOCUS_TARGET, tr("&Undo") );
    a->setShortcut( QKeySequence(QKeySequence::Undo) );
    a->setMenuRole( QAction::NoRole );
    ActionUtil::createToolTip(a);

    a = NewGlobalAction( Cmd_Redo, GLOBAL_FOCUS_TARGET, tr("&Redo") );
    a->setShortcut( QKeySequence(QKeySequence::Redo) );
    a->setMenuRole( QAction::NoRole );
    ActionUtil::createToolTip(a);

    // Object Commands shared with P4V
    ObjectCommandGlobals::DefineObjectCommands( this, &ActionStore::Global() );
    ObjectCommandGlobals::DefineObjectVisualCommands( this, &ActionStore::Global() );

    // MainWindowCommands from P4V
    a = NewGlobalAction( Cmd_AddConnection, GLOBAL_FOCUS_TARGET,
        tr("&Add Favorite Connection...") );
    a = NewGlobalAction( Cmd_ManageConnections, GLOBAL_FOCUS_TARGET,
        tr("&Manage Favorites...") );
    a = NewGlobalAction( Cmd_CloseConnection, GLOBAL_FOCUS_TARGET,
        tr("&Close Connection") );
    a = NewGlobalAction( Cmd_Reconnect, GLOBAL_FOCUS_TARGET,
        tr("Reconnec&t") );
    a = NewGlobalAction( Cmd_AdminRefreshConnection, GLOBAL_FOCUS_TARGET,
        tr("Refresh") );
    a = NewGlobalAction( Cmd_Customize, GLOBAL_FOCUS_TARGET,
        tr("&Manage Custom Tools... ") );
    a = NewGlobalAction( Cmd_ShowInfo, APP_TARGET, tr("&System Info") );
    a->setShortcut( QKeySequence(Qt::CTRL+Qt::Key_I) );
}

void
P4AdminApplication::customizeAction( const StringID & cmd, QAction * a )
{
    if ( cmd == Cmd_Close  )
    {
        a->setEnabled( qApp->activeWindow() );
        return;
    }

    if ( cmd == Cmd_Maximize || cmd == Cmd_Minimize )
    {
        a->setEnabled( qApp->activeWindow() );
        return;
    }

    if ( cmd == Cmd_RefreshAll )
    {
        a->setEnabled( mAdminWindow && mAdminWindow->isVisible() );
        return;
    }

    if (cmd == Cmd_AdminOpenConnection)
    {
        a->setEnabled(true);
        return;
    }
}

bool
P4AdminApplication::allWindowsClosed()
{
    // This checks to see if any widgets are left.

    foreach( QWidget* widget, qApp->topLevelWidgets() )
    {
        if ( widget->testAttribute( Qt::WA_QuitOnClose ) && !widget->isHidden() )
            return false;
    }
    // If there are no visible widgets, we can quit
    //
    return true;
}


/// \brief Responder slot for Cmd_AdminOpenConnection
void
P4AdminApplication::adminOpenConnection( const QVariant & )
{
    Op::Configuration config = AdminWindow::OpenConfigurationDialog(NULL);
    if (!config.isEmpty())
    {
        if (mWorkspace->windowManager()->showWindow(winKey::admin::MainWindow))
        {
            handleMessage(config.adminString());
        }
        else
        {
            // is this code even run any more?
            Q_ASSERT(0);
            mWorkspace->init(config);
            createAdminWindow(config);
        }
    }
}

void
P4AdminApplication::showPrefs(const QVariant &)
{
    Qt::WindowFlags fl =    WinFlags::CloseOnlyFlags();
    UIAdminPreferencesDialog* prefsDialog = new UIAdminPreferencesDialog(NULL, fl);
    prefsDialog->setAttribute(Qt::WA_DeleteOnClose, true);
    prefsDialog->setWindowModality(Qt::ApplicationModal);
    prefsDialog->exec();
}

void
P4AdminApplication::showHelp(const QVariant &)
{
    QKeyEvent ev(QEvent::KeyPress, Qt::Key_Help, Qt::NoModifier);
    QObject* w = Gui::Application::dApp->getLastFocus();
    if (w)
        qApp->sendEvent(w, &ev);
    // If we don't have a last focus, i.e. all windows are closed
    // on the Mac, we show default page
    else
        qApp->sendEvent(this, &ev);
}

void
P4AdminApplication::showInfo(const QVariant &)
{
    UIConnectionInfoDialog* connectioninfo = new UIConnectionInfoDialog(
        mAdminWindow->currentWorkspace(), qApp->activeWindow());
    connectioninfo->setWindowFlags( WinFlags::CloseOnlyFlags() );
    connectioninfo->setWindowModality(Qt::NonModal);
    connectioninfo->setAttribute( Qt::WA_DeleteOnClose );
    connectioninfo->show();
    connectioninfo->raise();
}
void
P4AdminApplication::close(const QVariant &)
{
    if (qApp->activeWindow())
        qApp->activeWindow()->close();
}

/// Cmd_Maximize_WINDOW
void
P4AdminApplication::maximizeWindow(const QVariant &v)
{
    mAdminShellApp->maximizeWindow(v);
}

/// Cmd_Minimize_WINDOW
void
P4AdminApplication::minimizeWindow(const QVariant &v)
{
    mAdminShellApp->minimizeWindow(v);
}

void
P4AdminApplication::quit(const QVariant &)
{
    qApp->closeAllWindows();

    if ( allWindowsClosed() )
        QApplication::quit();
}


void
P4AdminApplication::adminAbout(const QVariant &data)
{
    if (mAdminWindow)
        mAdminWindow->adminAbout(data);
}

/// \brief Responder slot for Cmd_RefreshAll
void
P4AdminApplication::refreshAll( const QVariant &data )
{
    if (mAdminWindow)
        mAdminWindow->refreshAll(data);
}

void
P4AdminApplication::createAdminWindow(const Op::Configuration& config)
{
    Gui::WindowManager* winMgr = mWorkspace->windowManager();
    if (winMgr->showWindow(winKey::admin::MainWindow))
        return;

    AdminWindow* win = mAdminWindow = new AdminWindow
        (
        NULL,
        Qt::Window,
        mWorkspace,
        false
        );
    win->setAttribute(Qt::WA_DeleteOnClose, !Platform::IsMac());
    win->setObjectName( winKey::admin::MainWindow );
    winMgr->setWorkspaceParent(win);
    winMgr->registerAndShowWindow( winKey::admin::MainWindow, win, true );

    win->init(config);  // delayed after show as this call restores connections
}
