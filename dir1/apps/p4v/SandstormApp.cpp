/*
 *  SandstormApp is the QApplication for P4V and manages:
 *
 *      Command line arguments
 *      P4V startup - see init()
 *      Menu commands
 *      Menu/toolbar enable/disable states
 *      P4V workspaces
 *      handleCommand - our custom events
 *
 */
#include "SandstormApp.h"

#include "ActionStore.h"
#include "AutoQPointer.h"
#include "ClientPortCommandHandler.h"
#include "CmdAction.h"
#include "Command.h"
#include "CommandIds.h"
#include "ConnectionContainerEditor.h"
#include "ConnectionDef.h"
#include "ConnectionMap.h"
#include "ConnectionsMenu.h" // for ConfigConnectionDef
#include "Cp858Codec.h"
#include "customizetools.h"
#include "DebugLog.h"
#include "DialogUtil.h"
#include "DvcsConnectionUtil.h"
#include "EditorCaller.h"
#include "GuiConnectionChecker.h"
#include "GuiErrorPrompter.h"
#include "GuiFeatures.h"
#include "GuiFeaturesLoader.h"
#include "GuiWindowManager.h"
#include "HelpLauncher.h"
#include "ItemEditorFactory.h"
#include "LanguageWidget.h"
#include "LogHandler.h"
#include "LogUtil.h"
#include "MainWindow.h"
#include "MainWindowCommands.h"
#include "ObjClient.h"
#include "ObjDirectory.h"
#include "ObjectCommandGlobals.h"
#include "ObjectDumper.h"
#include "ObjectNameUtil.h"
#include "ObjectTreeItem.h"
#include "ObjStash.h"
#include "OpClientView.h"
#include "OpContext.h"
#include "OpFileSystem.h"
#include "OpP4Path.h"
#include "OpTagDict.h"
#include "P4PathEdit.h"
#include "P4VCommandHandler.h"
#include "P4VCommandLine.h"
#include "P4VConnections.h"
#include "P4VPreferences.h"
#include "P4VPreferencesDialog.h"
#include "Platform.h"
#include "PMessageBox.h"
#include "Prefs.h"
#include "RCFServer.h"
#include "SettingKeys.h"
#include "ShortCutCmds.h"
#include "ShortcutItemEditor.h"
#include "StyleSheetUtil.h"
#include "SWorkspace.h"
#include "SyncHint.h"
#include "UIAboutDialog.h"
#include "UIConfigurationDialog.h"
#include "UIConfigurationWizard.h"
#include "UISpecPickerWithType.h"
#include "UIWorkspace2.h"
#include "WinFlags.h"
#include "XMLManager.h"
#include "Workspace.h"

// import static-built image format libs
// THIS LOADING OF THE PLUGINS NO lONGER WORKS IN QT5..... NEEDS TO BE FIXED
/*
#ifdef QT_STATICPLUGIN
    // Built-in image file formats
    #include "imageformatplugins.h"

    #ifdef Q_OS_WIN
        Q_IMPORT_PLUGIN(libqtmaxfmt);
    #endif
        Q_IMPORT_PLUGIN(libqtmayafmt);
        Q_IMPORT_PLUGIN(libqtphotofmt);
        Q_IMPORT_PLUGIN(libqttgafmt);
#endif
*/
// END OF .........  NEEDS TO BE FIXED

#include <QAction>
#include <QApplication>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileOpenEvent>
#include <QFont>
#include <QFontInfo>
#include <QKeyEvent>
#include <QLibraryInfo>
#include <QMap>
#include <QMenu>
#include <QMenuBar>
#include <QPixmapCache>
#include <QResource>
#include <QShortcut>
#include <QStyleFactory>
#include <QString>
#include <QTextCodec>
#include <QTime>
#include <QTimer>
#include <QTranslator>

#include <iostream>

qLogCategory("SandstormApp");

/*** Application Data ***/

//-----------------------------------------------------------------------------
//  ShowCommandLineError
//-----------------------------------------------------------------------------

void
SandstormApp::showCommandLineError(const QString& msg)
{
    if (!Platform::IsUnix())
    {
        PMessageBox::critical(NULL, Platform::ApplicationName(), msg);
    }
    else
    {
        QTextStream stout(stdout, QIODevice::WriteOnly);
        stout << msg << "\n";
    }
}

//-----------------------------------------------------------------------------
//  ShowCommandLineInfo
//-----------------------------------------------------------------------------

void
SandstormApp::showCommandLineInfo(const QString& msg)
{
    if (!Platform::IsUnix())
    {
        PMessageBox::information(NULL, Platform::ApplicationName(), msg);
    }
    else
    {
        QTextStream stout(stdout, QIODevice::WriteOnly);
        stout << msg << "\n";
    }
}

//-----------------------------------------------------------------------------
//  processCommandLine
//-----------------------------------------------------------------------------

void
SandstormApp::processCommandLine(int argc, QStringList argv)
{
    if ( argc > 1 )
    {
        QString sep = " ";
        rLog(1) << "processCommandLine= " << argv.join(sep) << qLogEnd;
    }
    
    // The P4VCommandLine::parse function removes the argv[0] from the list
    // so we save it here so we can use it in the help output
    QString commandName = argv[0];
    
    mCommandLine->parse(argv);

    qApp->setStyle( QStyleFactory::create("fusion") );
    
    QString styleName = Prefs::Value(Prefs::FQ(p4v::prefs::Skin), p4v::Preferences::Defaults()[p4v::prefs::Skin].toString());
    QString filename = Platform::SkinsDir() + QDir::separator() + styleName + ".qss";
    QFile styleSheetFile(filename);
    if (styleSheetFile.exists())
    {
        styleSheetFile.open(QIODevice::ReadOnly | QIODevice::Text);
        QByteArray styleSheetData = styleSheetFile.readAll();
        QString styleSheet(styleSheetData.constData());
        qApp->setStyleSheet(styleSheet);
    }
    else
    {
        qApp->setStyleSheet(StyleSheetUtil::builtin());
    }

    qApp->setProperty("StyleSheetLoaded", QVariant(true));
 
    // for -faceless do not just run what the command line says
    if (mCommandLine->faceless())
    {
        mRCFServer.reset(new RCFServer(this));
        mRCFServer->init("127.0.0.1", 7999);
        return;
    }


    Gui::GRID_OPTION = mCommandLine->gridOption();

    switch(mCommandLine->mode())
    {
    case P4VCommandRunner::P4V:
        if(mCommandLine->consoleLogOption() == P4VCommandLine::Partial)
            useLog(false);
        else if(mCommandLine->consoleLogOption() == P4VCommandLine::All)
            useLog(true);
        if(mCommandLine->winHandle())
           Platform::SetExternalWindowHandle(mCommandLine->winHandle());
        return;
    case P4VCommandRunner::Error:
        showCommandLineError(mCommandLine->errorMessage());
        setQuitCode(1);
        return;
    case P4VCommandRunner::TestOnly:
        setQuitCode(0);
        return;
    case P4VCommandRunner::ShowHelp:
        {
         QString msg = tr(
            "Usage: %1 [options] [arg]\n"
            "\n"
            "Options:\n"
            "-p port   (Sets p4port)\n"
            "-u user   (Sets p4user)\n"
            "-c client   (Sets p4client)\n"
            "-log   (Logs p4 user actions to stdout)\n"
            "-logall   (Includes all p4 operations)\n"
            "-t tab   (Uses the tab argument to specify active tab. Tab options are "
            "files, history, pending, submitted, branches, labels, workspaces "
            "and jobs)\n"
            "-s filename   (Opens the workspace or depot tree to the specified "
            "file and selects the file)\n"
            "-V   (Displays the client version number)\n"
            "-h or -help   (Displays this message)\n").arg(commandName);


// Do not put #ifdef in the middle of tr() statements because lupdate will not pick up
// any strings that come after them.

#ifdef TEST
        msg += "    -testonly      run only the unit tests, then exit\n";
#endif
        showCommandLineInfo(msg);
        setQuitCode(0);
        return;
        }
    case P4VCommandRunner::OtherWindow:
    default :
        break;
    }
}

//-----------------------------------------------------------------------------
//  static openLastWorkspaces
//-----------------------------------------------------------------------------


static bool
openLastWorkspaces(SandstormApp * app)
{
    bool wsOpened = false;
    QStringList lastWorkspaces = Prefs::Value(Prefs::FQ(p4v::prefs::OpenWorkspaces), QStringList());
    foreach(QString name, lastWorkspaces)
    {
        Op::Configuration configuration =
            Op::Configuration::CreateFromSerializedString( name );

        app->fillInDvcsInfo(configuration);

        bool opened = app->showWorkspace(configuration);

        if (opened)
            wsOpened = true;
    }

    if (Platform::IsMac()) {
        // fix for job020461
        // on the mac you can exit p4v while there is no workspace running.
        // in this case we do not want to bring up the connection dialog
        // unconditionally, we start up with the last connection instead.
        const QString serializedConnection = Prefs::Value(Prefs::FQ(p4v::prefs::LastConnection), QString());
        if (!wsOpened && !serializedConnection.isEmpty())
        {
            Op::Configuration configuration =
                Op::Configuration::CreateFromSerializedString( serializedConnection );
            return app->showWorkspace(configuration);
        }
    } // if (Platform::IsMac())

    return wsOpened;
}

//-----------------------------------------------------------------------------
/*** The Application Object ***/
//-----------------------------------------------------------------------------


SandstormApp * SandstormApp::sApp = NULL;

//-----------------------------------------------------------------------------
//  Create
//-----------------------------------------------------------------------------

bool
SandstormApp::Create()
{
    if (!QResource::registerResource(Platform::ImageResourceFile()))
    {
        PMessageBox::critical(0, tr("Helix Visual Client"),
            tr("Error initializing Helix Visual Client.\nUnable to load resource file."));
        qWarning("error loading resource file");
        return false;
    }

    QString configPath;
#ifdef Q_OS_OSX
    QDir tempDir = QDir::home();
    tempDir.cd( "Library" );
    tempDir.cd( "Preferences" );

    QFileInfo finfo(tempDir.absoluteFilePath("com.perforce.p4qt"));
    if (finfo.exists())
        configPath = tempDir.absoluteFilePath("com.perforce.p4qt");
    else
        configPath = tempDir.absoluteFilePath("com.perforce.p4v");
#else
    configPath = QDir::home().absoluteFilePath(".p4qt");
#endif

    XMLManager::setConfigDir(configPath);

    LogUtil::Configure(Prefs::Value(Prefs::FQ(p4v::prefs::LogToFile), p4v::Preferences::Defaults()[p4v::prefs::LogToFile].toBool()), configPath + "/log.config",
                       Prefs::Value(Prefs::FQ(p4v::prefs::LogFileName), configPath + "/log.txt"),
                       Prefs::Value(Prefs::FQ(p4v::prefs::LogFileSize), p4v::Preferences::Defaults()[p4v::prefs::LogFileSize].toInt()) * 1024);


    static Gui::LogFormatter formatter;
    dApp = sApp = new SandstormApp( configPath, formatter);
    if(sApp->quitNow())
        return true;

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

    // Install translations
    sApp->installTranslators();
    QStringList args = qApp->arguments();
    sApp->processCommandLine(args.count(), args);

        // let's see how this goes. For debugging, this is HUGELY helpful and I think may be a valuable
        // tool for even tech support to have. Historically this was only put in for debug versions,
        // but I want to see about having it in a release build as well
    ObjectDumper::InstallOnApplication();

    new P4CP858Codec();

    connect(qApp, SIGNAL(aboutToQuit()), sApp, SLOT(handleAboutToQuit()));

    return true;
}

void
SandstormApp::handleAboutToQuit()
{
    SandstormApp::Destroy();
}

//-----------------------------------------------------------------------------
//  Destroy
//-----------------------------------------------------------------------------

void
SandstormApp::Destroy()
{
    delete sApp;
    dApp = sApp = 0;
    Prefs::Flush();

    Gui::PropertyFeatures* features = Gui::PropertyFeatures::Instance();
    if (features)
        delete features;

    if (!QResource::unregisterResource(Platform::ImageResourceFile()))
    {
        qWarning("error unloading resource file");
    }
}

//-----------------------------------------------------------------------------
//  SandstormApp
//-----------------------------------------------------------------------------

SandstormApp::SandstormApp(const QString &configPath ,const Gui::LogFormatter& formatter)
: Gui::Application(configPath, formatter)
, mCommandHandler(0)
, mHelpLauncher(0)
, mCommandLine(0)
#ifdef Q_OS_OSX
, mAboutDialog()
#endif
, mConfigWizard(0)
, mShortcutCmds(0)
{
    // Object name is used for context sensitive help and/or
    // test automation so be careful if changing it.
    //
    setObjectName("P4V_APPLICATION");

                // use this when debugging win to V problems
    //PMessageBox::critical( 0x0, "P4V attach debugger", "attach debugger", "OK" );

                // assume it's going to be p4v for now; override this later if
                // we find a -cmd option
                // set before processCommandLine - which might change it
    Platform::SetApplication(Platform::P4V_App);
    setAppIcon(QPixmap(":/p4vimage.png"));

    mCommandLine = new P4VCommandLine(this);

    if(quitNow())
        return;

    Gui::Features features = Gui::FeaturesLoader(QString()).features();
    bool streamsAreEnabled = features.isFeatureVisible(Gui::Features::Streams);
    Op::Context::SetStreamsFeatureEnabled(streamsAreEnabled);

    bool unloadIsEnabled = features.isFeatureEnabled(Gui::Features::UnloadReload);
    Op::Context::SetUnloadFeatureEnabled(unloadIsEnabled);
}

//-----------------------------------------------------------------------------
//  ~SandstormApp
//-----------------------------------------------------------------------------

SandstormApp::~SandstormApp()
{
    qApp->removeEventFilter(this);

    if ( !Platform::RunningFromCommandLine() )
        Op::FileSystem::DeleteAllTemporaryFiles();

    delete mHelpLauncher;
    delete mShortcutCmds;
    delete mCommandHandler;
    QItemEditorFactory::setDefaultFactory( NULL );
}


//-----------------------------------------------------------------------------
//  init
//-----------------------------------------------------------------------------

void
SandstormApp::init()
{
    Gui::Application::init();

    QPixmapCache::setCacheLimit(3072);

    loadGlobalActions();
    Responder::SetLastResponder(this);

    mCommandHandler = new P4VCommandHandler();

    // Install custom editors for QueryDialogs, and Models and Trees
    QItemEditorFactory* itemFactory = new ItemEditorFactory();

    QItemEditorCreatorBase *pathEditorCreator = new QStandardItemEditorCreator<P4PathEdit>();
    itemFactory->registerEditor( (QVariant::Type)qMetaTypeId<OpP4Path>(), pathEditorCreator );

    QItemEditorCreatorBase *shortCutEditorCreator = new QStandardItemEditorCreator<CustomShortcutItemEditor>();
    itemFactory->registerEditor(QVariant::KeySequence, shortCutEditorCreator);

    QItemEditorFactory::setDefaultFactory( itemFactory );

    // create a global command cache
    ObjectCommandGlobals::DefineObjectCommands(NULL, &ActionStore::Global());
    ObjectCommandGlobals::DefineObjectVisualCommands(this, &ActionStore::Global());

    // set the current working directory global
    Gui::ConnectionsMenu::setStartupDirectory(QDir::toNativeSeparators(QDir::currentPath()));

    qApp->installEventFilter(this);

    // the default value is 4
    qApp->setStartDragDistance(12);

    // don't close on last window for RCF
    if (mCommandLine->faceless())
        qApp->setQuitOnLastWindowClosed(false);

    // connect so last open workspace(s) will be remembered
    connect(qApp,
            SIGNAL(lastWindowClosed()),
            this,
            SLOT(onLastWindowClosed()));
}

//-----------------------------------------------------------------------------
//  helpWindow
//-----------------------------------------------------------------------------

HelpLauncher*
SandstormApp::helpWindow()
{
    QString helpPrefix;
    
    Gui::Workspace* ws = getActiveWorkspace();
    bool locallyDeployed = false;

    QVariant v;
    if (ws)
        v = Gui::PropertyFeatures::Instance()->value(ws->scopeName(), Gui::Features::P4VHelp);
    if (!v.isNull())
    {
        helpPrefix = v.toString();
        if (helpPrefix == kLocalHelpProperty)
        {
            helpPrefix = determineLocalHelpPath();
            locallyDeployed = true;
        }
    }
        // if we have a ws, but the variant is null, it means we COULD have had a
        // property, but don't, so we should go out to the online help since we
        // know for sure that the admin has not set an alternate location
    else if (ws)
    {
        helpPrefix = kOnlineHelpPrefix;
    }
    else // if we don't have a property, point at online help
    {
        QString helpPreference = Prefs::Value(Prefs::FQ(p4v::prefs::HelpIndex), QString());
        if (!helpPreference.isEmpty())
        {
            QFileInfo fi(helpPreference + "index.html");
            if (fi.exists())
                helpPrefix = helpPreference;
            else
                helpPrefix = kOnlineHelpPrefix;
        }
        else
        {
            helpPrefix = kOnlineHelpPrefix;
        }
    }

    if (helpPrefix.isEmpty())
        return NULL;

    if (!mHelpLauncher)
    {
        mHelpLauncher = new HelpLauncher("p4v");
            // connect pref change for the application language to the help
            // launcher so that it updates real-time if you change your 
            // preferred language
    }

    QString countryCode = Prefs::Value(Prefs::FQ(p4v::prefs::ApplicationLanguage), QString("en"));
    mHelpLauncher->setHelpLocation(helpPrefix);
    mHelpLauncher->setLocale(countryCode);
    mHelpLauncher->setLocallyDeployed(locallyDeployed);
    return mHelpLauncher;
}


QString
SandstormApp::determineLocalHelpPath()
{
    QString helpPreference = Prefs::Value(Prefs::FQ(p4v::prefs::HelpIndex), QString());
    if (!helpPreference.isEmpty())
        return helpPreference;

    QString appName = Platform::ApplicationName();

    PMessageBox::warning(NULL,
                         tr("%1").arg(appName),
                         tr("Your administrator has deployed %1's help locally. Please locate the index.html.").arg(appName));

    QString helpDir = QFileDialog::getOpenFileName(NULL,
                                                   tr("%1 - Locate the index.html.").arg(appName),
                                                   Platform::ApplicationDirPath(),
                                                   "index.html");

    if (helpDir.isEmpty())
    {
        PMessageBox::warning(NULL,
                             tr("%1").arg(appName),
                             tr("Help files index not found."),
                             QMessageBox::Ok);
        return QString();
    }

    QFileInfo fi(helpDir);
    Q_ASSERT(fi.exists()); // we picked it, it should exist.
    helpDir = QString("%1/").arg(fi.absolutePath());

    Prefs::SetValue(Prefs::FQ(p4v::prefs::HelpIndex), helpDir);
    return helpDir;
}

//-----------------------------------------------------------------------------
//  loadGlobalActions
//-----------------------------------------------------------------------------

void
SandstormApp::loadGlobalActions()
{
    Gui::Application::loadGlobalActions();

    MainWindowCommands::DefineApplicationCommands(this, & ActionStore::Global());

    CmdAction * a = NULL;
    CmdTargetDesc APP_TARGET( this, CmdTargetDesc::HolderIsTheTarget );
    CmdTargetDesc GLOBAL_FOCUS_TARGET( NULL, CmdTargetDesc::FocusWidget );

    a = ActionStore::Global().createAction( Cmd_Prefs, GLOBAL_FOCUS_TARGET, tr("Preferences...") );
    if ( Platform::IsMac() )
    {
        a->setDefaultShortcut( tr("CTRL+,") );
        a->setActionScope(CmdAction::CAS_Private);
    }
    else
        a->setText( tr("Prefere&nces...") );
    a->setMenuRole( QAction::PreferencesRole );

    a = ActionStore::Global().createAction( Cmd_CheckForUpdate, GLOBAL_FOCUS_TARGET, tr("&Check for Updates") );
    if ( Platform::IsMac() )
        a->setActionScope(CmdAction::CAS_Private);
    a->setMenuRole( QAction::NoRole );


    a = ActionStore::Global().createAction( Cmd_About, APP_TARGET, tr("&About P4V") );
    a->setMenuRole( QAction::AboutRole );
    if ( Platform::IsMac() )
        a->setActionScope(CmdAction::CAS_Private);

    a = ActionStore::Global().createAction( Cmd_Help, APP_TARGET, tr("P4V &Help") );

    if ( Platform::IsMac() )
    {
        if ( beforeLeopard() )
            a->setDefaultShortcut( tr("CTRL+?") );
        else
        {
            /*
            // does not work, only recognizes the first item.
            QList<QKeySequence> shortcuts;
            shortcuts << Qt::Key_Question + Qt::CTRL + Qt::ALT ;
            shortcuts << Qt::Key_Slash + Qt::CTRL ;
            a->setShortcuts( shortcuts );
            */
            a->setDefaultShortcut( Qt::Key_Question + Qt::CTRL + Qt::ALT );
        }
    }
    else
        a->setDefaultShortcut( tr("F1") );
    a->setMenuRole( QAction::NoRole );

    if ( Platform::IsMac() )
    {
        a = ActionStore::Global().createAction( Cmd_Quit, APP_TARGET, tr("Quit") );
        a->setDefaultShortcut( tr("CTRL+Q") );
        a->setActionScope(CmdAction::CAS_Private);
    }
    else
    {
        a = ActionStore::Global().createAction( Cmd_Quit, APP_TARGET, tr("E&xit") );
    }
    a->setMenuRole( QAction::QuitRole );


    a = ActionStore::Global().createAction( Cmd_Cut,        GLOBAL_FOCUS_TARGET, tr("Cu&t") );
    a->setMenuRole( QAction::NoRole );
    a->setDefaultShortcut( tr("CTRL+X") );
    a->setActionScope(CmdAction::CAS_Private);

    a = ActionStore::Global().createAction( Cmd_CopyToFind,GLOBAL_FOCUS_TARGET, tr("Us&e Selection for Find") );
    a->setMenuRole( QAction::NoRole );
    if ( !Platform::IsMac() )
        a->setDefaultShortcut( tr("CTRL+E") );

    a = ActionStore::Global().createAction( Cmd_Paste,      GLOBAL_FOCUS_TARGET, tr("&Paste") );
    a->setMenuRole( QAction::NoRole );
    a->setDefaultShortcut( tr("CTRL+V") );
    a->setActionScope(CmdAction::CAS_Private);

    a = ActionStore::Global().createAction( Cmd_Undo, GLOBAL_FOCUS_TARGET, tr("&Undo") );
    a->setMenuRole( QAction::NoRole );
    a->setDefaultShortcut( tr("CTRL+Z") );
    if ( !Platform::IsMac() )
        a->setIcon( QIcon( ":/cmd_undo.png" ) );
    a->setToolTip( tr("Undo"));
    a->setActionScope(CmdAction::CAS_Private);

    a = ActionStore::Global().createAction( Cmd_Redo, GLOBAL_FOCUS_TARGET, tr("&Redo") );
    a->setMenuRole( QAction::NoRole );
    if ( !Platform::IsMac() )
        a->setIcon( QIcon( ":/cmd_redo.png" ) );
    a->setToolTip( tr("Redo"));
    a->setActionScope(CmdAction::CAS_Private);
    if ( Platform::IsMac() )
        a->setDefaultShortcut( tr("CTRL+SHIFT+Z") );
    else
        a->setDefaultShortcut( tr("CTRL+Y") );
    /*
    a = ActionStore::Global().createAction(Cmd_DvcsInit, GLOBAL_FOCUS_TARGET, tr("&Init local..."));
    a->setMenuRole(QAction::NoRole);
    if (!Platform::IsMac())
        a->setIcon(QIcon(":/cmd_init.png"));
    a->setToolTip(tr("Initialize new personal server"));
    a->setActionScope(CmdAction::CAS_Private);
    */
}

bool
SandstormApp::loadInstallerConfig(Op::Configuration& config)
{
    // job041895 - look for the installer file that specifies the new default config
    const QString installerOverride = Gui::Application::dApp->getConfigDir().filePath("NextConnectionSettings.cfg");
    if ( QFileInfo(installerOverride).exists() )
    {
        // default is the most recent connection
        QString serializedConnection = Prefs::Value(Prefs::FQ(p4v::prefs::LastConnection), QString());
        config = Op::Configuration::CreateFromSerializedString( serializedConnection );

        // load P4PORT/P4USER/P4WORKSPACE values
        QFile file(installerOverride);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            while (!file.atEnd())
            {
                QString nvp = QString(file.readLine()).simplified();
                // remove CRLF
                QStringList pair = nvp.split('=');
                if (pair.length() != 2)
                    continue;
                const QString name = pair[0];
                const QString val  = pair[1];
                if (name.isEmpty())
                    continue;

                if (name == "P4PORT")
                    config.setPort(val);
                else if (name == "P4USER")
                    config.setUser(val);
                else if (name == "P4CLIENT")
                    config.setClient(val);
            }
        }
        file.close();
        QFile::remove( installerOverride );
        return true;
    }

    return false;
}

void
SandstormApp::delayedInit()
{
    bool success = false;

    Gui::Application::delayedInit ();

    Op::Configuration config = mCommandLine->config();

    // running as RCF?  return, messages come in via another route
    if (mCommandLine->faceless())
        return;

    if (!mCommandLine->config().isEmpty())
    {
        success = showWorkspace(config);
    }
    else
    {
        if (loadInstallerConfig(config))
        {
            success = startNewWorkspace( config );
        }
        else
        {
            using namespace Gui;
            connection::StartupOption startUp = static_cast<connection::StartupOption>(Prefs::Value(Prefs::FQ(p4v::prefs::StartupOption), static_cast<int>(p4v::Preferences::Defaults()[p4v::prefs::StartupOption].value<connection::StartupOption>())));
            if (connection::Last == startUp)
            {
                success = openLastWorkspaces (this);
            }
            else if (connection::Environment == startUp)
            {
                Op::Configuration dconfig = Op::Configuration::DefaultConfiguration();

                fillInDvcsInfo(dconfig);

                success = showWorkspace (dconfig);
            }
            else
            {
                // The dialog will come up because the value of
                // success is still false.
            }

            // Last chance at trying to get a workspace
            if (!success )
                success = startNewWorkspace();
        }
    }
    setDelayedInitStarting(false);

    if (!success)
        if (!isTopLevelWidgetOpen())
        {
            if (mCommandLine->usingEnviroArgs())
            {
                PMessageBox::critical(NULL,
                                      Platform::ApplicationName(),
                                      tr("Something went wrong trying to start P4V with those parameters."));
                                      
            }
            qApp->quit();
        }
}

// so far, this is called from in-points that bypass the connection dialog

void
SandstormApp::fillInDvcsInfo(Op::Configuration &config)
{
    // Don't want to introduce new weirdness at ScopeName and ConnectionMap level
    // by checking a clean-port string via a substring check. Use the full port.
    // Also need the working directory for a correct workspace init!
    if (Platform::RunningInP4V())
    {
        // This will set the config.port to the fullPort, if found.
        DvcsConnectionUtil dvcs;
        dvcs.findFullPort(config);

        // won't destroy the config's full port
        // This just establishes the working dir for dvcs
        dvcs.processConfig(config, config.port());
    }
}

//-----------------------------------------------------------------------------
//  handleFileOpenEvent
//-----------------------------------------------------------------------------

void
SandstormApp::handleFileOpenEvent(QFileOpenEvent * e)
{
    // try to open the file as a configuration file
    Op::Configuration config = Op::Configuration::DefaultConfiguration(e->file());
    if (!config.isEmpty())
        showWorkspace(config);

    e->accept();
}

//-----------------------------------------------------------------------------
//  customizeAction
//-----------------------------------------------------------------------------

void
SandstormApp::customizeAction( const StringID & cmd, QAction * a )
{
    if ( cmd == Cmd_Close )
        a->setEnabled( qApp->activeWindow() );
    else if (cmd == Cmd_Minimize)
        a->setEnabled( qApp->activeWindow() );
    else if (cmd == Cmd_Maximize)
        a->setEnabled( qApp->activeWindow() );
    else if (cmd == Cmd_BringToFront)
        a->setEnabled( Platform::IsMac() );
    else if (cmd == Cmd_AddConnection)
        a->setEnabled( getActiveWorkspace() );
    else if (cmd == Cmd_ReloadDialog)
    {
        Gui::Workspace* ws = getActiveWorkspace();
        if (!ws)
        {
            a->setEnabled(false);
            return;
        }
        Op::ServerProtocol protocol = ws->stash()->context()->serverProtocol();
        Gui::Features features = ws->features();
        a->setEnabled(features.isFeatureEnabled(Gui::Features::UnloadReload) &&
                      protocol.isUnloadCapable());
        CmdAction* c = static_cast<CmdAction*>(a);
        if (c->data().toInt() == (int)Obj::STREAM_TYPE && (!protocol.isTaskStreamCapable() ||
            !features.isFeatureEnabled(Gui::Features::Streams)))
            a->setEnabled(false);
    }
}

//-----------------------------------------------------------------------------
//  reopenApplication
//-----------------------------------------------------------------------------

/*! \brief React to a click on out Dock icon.

    If we have at least one window, do nothing. The regular application
    activation sequence will bring it to front, which is enough feedback.
    If we have NO windows, then there won't be enough feedback that we're
    activated, so put the connection dialog in the user's face as feedback.

    \see PlatformMac::RegisterEventHandlers();
*/
void
SandstormApp::reopenApplication()
{
    // If any window is visible, we've nothing more to do.
    QWidgetList ww = qApp->topLevelWidgets();
    foreach (QWidget * w, qApp->topLevelWidgets())
        if (w && w->isVisible()) return;

    // Show a blank document as feedback.
    // Connection dialog is as close to a blank document as we get.
    startNewWorkspace();
}

//-----------------------------------------------------------------------------
//  eventFilter
//-----------------------------------------------------------------------------
bool
SandstormApp::eventFilter(QObject* obj, QEvent* e)
{
    static const bool kPassToObj      = false;
    static const bool kDoNotPassToObj = true;

    if(Gui::Application::eventFilter(obj, e))
        return kDoNotPassToObj;

    switch (e->type()) {

        case QEvent::ApplicationActivate :
        {
        if (Platform::IsMac() && !delayedInitStarting())
                reopenApplication();
        }
        break;

        case QEvent::FileOpen:
            handleFileOpenEvent( (QFileOpenEvent*)e );
        break;
        case QEvent::KeyPress: {
            if (((QKeyEvent *)e)->key() == Qt::Key_Help)
            {
                // No help files were found tell app we handled the condition
                // to avoid recursion
                HelpLauncher* hLauncher = helpWindow();
                if ( !hLauncher )
                    return kDoNotPassToObj;
                // This returns false if it fails to find a page
                // in which case we try again on its parent.
                return hLauncher->showHelp(obj);
            }
        }
        break;

        case QEvent::Show :
            {
                //#define CHECK_OBJECT_NAMES
                #if defined(QT_DEBUG) && defined(CHECK_OBJECT_NAMES)
                {
                    // Seek and report on QWidgets that lack required
                    // objectName() values. Dump ENTIRE view hierarchy
                    // any time you find an offending widget: gives more
                    // context and increases chance that programmer can
                    // find the offending code.
                    using namespace ObjectNameUtil;
                    QWidget * w = dynamic_cast<QWidget *>(obj);
                    if (!w)
                        break;
                    if (IsTopLevel(w) && !HierarchyHasSquishableNames(w))
                        DumpHierarchy(w);
                }
                #endif // QT_DEBUG && CHECK_OBJECT_NAMES
            }
            break;

        case QEvent::Wheel :
            {
                // Mac standard is to ignore scroll wheel events in many controls.
                if (Platform::IsMac())
                {
                    const QStringList kProhibited = QStringList()
                        << "QComboBox"
                        << "QTabBar";
                    foreach (const QString & s, kProhibited)
                    {
                        if (obj->inherits(qPrintable(s)))
                            return kDoNotPassToObj;
                    }
                }
            }
            break;

        default:
            break;
    }
    return kPassToObj;
}

//-----------------------------------------------------------------------------
//  startNewWorkspace
//-----------------------------------------------------------------------------
bool
SandstormApp::startNewWorkspace(const Op::Configuration& cfg /* = Op::Configuration() */)
{
    Gui::P4VConnectionMaps cmaps;
    cmaps.load();

         // repeat until they succeed in opening a workspace or give up
    Op::Configuration config = cfg;
    while (true)
    {
        QPointer<QWidget> pLastActiveWindow = qApp->activeWindow();
            // config the Configuration Dialog
        AutoQPointer<UIConfigurationDialog> dlg(new UIConfigurationDialog(pLastActiveWindow));

        dlg->setShowStartupOption(true);
        setAppIcon(dlg);
        dlg->init(config);

            // run the dialog
        if (dlg->exec() != QDialog::Accepted || !dlg)
        {
            return false;
        }
            // save startup option to prefs
        using namespace Gui;
        if (dlg->startupDialog())
            Prefs::SetValue(Prefs::FQ(p4v::prefs::StartupOption), connection::Dialog);
        else
            Prefs::SetValue(Prefs::FQ(p4v::prefs::StartupOption), connection::Last);

            // try to open the selected workspace
            // if it fails, go back and show dialog again
        config = dlg->configuration();


        // Keep everything from the dlg object, because while we are trying to open the workspace
        // the active window could be closed and the dlg will be destroyed
        bool syncWorkspace = dlg->syncWorkspace();
        Op::Configuration lastConnection = dlg->lastConnection();

        Gui::Workspace* ws = loadWorkspace(config, dlg->disconnected());
        if (!ws)
            continue;

        // before connecting to command handlers, set dvcs status;
        // connection info should be available from loadWorkspace() call.
        setIsDvcs(ws);

        connectWorkspaceToRunner(ws);

            // it worked, so save this as last connection pref
        Prefs::SetValue(Prefs::FQ(p4v::prefs::LastConnection),
                        lastConnection.serializedString());

        // Hack: Make sure the active window is not the UIConfigurationDialog that is going to be destructed
        if (qApp->activeWindow() == dlg)
            qApp->setActiveWindow(pLastActiveWindow);

        if (syncWorkspace)
            Gui::CommandHandler::Handle(Gui::Command(Cmd_SyncHead), ws);

        return true;
    }
}

//-----------------------------------------------------------------------------
//  runConnectionWizard
//-----------------------------------------------------------------------------

bool
SandstormApp::runConnectionWizard(bool runforthefirsttime)
{
    Op::Configuration config;

    mConfigWizard = new UIConfigurationWizard( NULL,
        WinFlags::WindowFlags() );

    if (Platform::IsMac())
    {
        Qt::WindowFlags f = WinFlags::CloseOnlyFlags() |
                Qt::WindowMaximizeButtonHint;
        f &= ~Qt::Dialog; // turns off the flag
        mConfigWizard->setWindowFlags( f );
    }

    mConfigWizard->init(config, runforthefirsttime);

    bool retvalue = false;
    if (mConfigWizard->exec() == QDialog::Accepted)
    {
        if (!showWorkspace(mConfigWizard->configuration()))
        {
            // return runConnectionWizard(false);
            retvalue = false;
        }
        else
        {
            Gui::Workspace* ws = findWorkspace(
                    mConfigWizard->configuration());

            QString rootdir = ws->stash()->rootDirectoryPath();
            QFileInfo finfo(rootdir);
            if (!finfo.exists())
            {
                QDir pdir(QDir::root().absolutePath());
                pdir.mkpath(rootdir);
            }

            Prefs::SetValue(Prefs::FQ(p4v::prefs::LastConnection),
                            mConfigWizard->configuration().serializedString());
            retvalue = true;
        }

        // open the workspace tab in the mainwindow
        Gui::Workspace* ws = findWorkspace( mConfigWizard->configuration());
        if (ws)
        {
            UIWorkspace2 * workspace = dynamic_cast<UIWorkspace2*>(ws->windowManager()->mainWorkspaceWindow());
            if (workspace)
                workspace->showTab(tabs::WorkspacePage);
        }
    }
    else
        retvalue = false;

    delete mConfigWizard;
    mConfigWizard = NULL;

    return retvalue;
}

/*** Workspace Functions ***/
//-----------------------------------------------------------------------------
//  createWorkspace
//-----------------------------------------------------------------------------

Gui::Workspace*
SandstormApp::createWorkspace(const Gui::WorkspaceScopeName &scopeName)
{
    Gui::Workspace* ws =  new SWorkspace(scopeName, NULL);
    connect(ws, SIGNAL(logItem(const Op::LogDesc&)),
            this, SLOT(handleLog(const Op::LogDesc&)));
    ws->windowManager()->setApplicationIcon(appIcon());

    return ws;
}

//-----------------------------------------------------------------------------
//  showWorkspace
//
//  disconnected: if server is down can start in disconnected mode
//-----------------------------------------------------------------------------


bool
SandstormApp::showWorkspace(const Op::Configuration & constConfig, bool disconnected)
{
    Gui::Workspace* ws = loadWorkspace(constConfig, disconnected);

    if (ws)
    {
        // before connecting to command handlers, set dvcs status;
        // connection info should be available from loadWorkspace() call.
        setIsDvcs(ws);
        connectWorkspaceToRunner(ws);
    }

    return (ws != NULL);
}

void
SandstormApp::setIsDvcs(Gui::Workspace* ws)
{
    Obj::Stash* stash = ws->stash();
    if (!stash)
        return;

    // get dvcs string from context info sniffer
    Op::TagDict clientTags = stash->context()->info();
    bool isDvcs = false;
    if (clientTags.count() > 0)
    {
        QString serverRoot = clientTags.value("serverRoot");
        if (serverRoot.contains(".p4root"))
            isDvcs = true;
    }

    // on startup always defaults to false in ConnectionState
    if (!isDvcs)
        return;

    // now, write it back to the existing (active) config;
    // Note that this field isn't used by the active state,
    // so we can change it without dropping the connection.
    stash->context()->setIsDvcs(isDvcs);
}

void
SandstormApp::connectWorkspaceToRunner(Gui::Workspace* ws)
{
    Q_ASSERT(ws);
    P4VCommandRunner* runner = &mCommandLine->runner();
    connect( runner, SIGNAL(error(const QString&)), SLOT(onCommandError(const QString&)) );
    connect( runner, SIGNAL(info(const QString&)),  SLOT(onCommandInfo(const QString&)) );
    runner->start(ws);
}

Gui::Workspace*
SandstormApp::loadWorkspace(const Op::Configuration & constConfig, bool disconnected)
{
    Gui::Workspace*       ws = NULL;
    bool                    workspaceChanged = false;
    Op::ConnectionStatus    status           = Op::CS_Normal;
    Op::Configuration       config = constConfig;

                    // charset for the connection if applicable
    if (config.charset().isEmpty())
        Gui::P4VConnectionMaps::SetCharset (&config);

                    // there is a map of cached workspaces for optimization
                    // for switching workspaces.
                    // is it already in the map of cached workspaces?
    if ((ws = findWorkspace (config, disconnected)))
        status = ws->stash()->context()->status();

                    // ** if it is not in the cached map of workspaces
                    // then the status will still be set to Normal (default)
                    //
                    // no workspace in the cache -> status will be normal
                    // workspace is in cache but it's not valid
                    // valid workspace and status is ok or busy trying to be good
    if ((!ws || !ws->isValid()) && status != Op::CS_Unavailable)
        return NULL;

                    // continue setting up for switching to ws in cache
    if (config.client() == ws->configuration().client())
    {
                    // exact match found, noop if active
    }
    else if ((ws->isValid () || status == Op::CS_Unavailable) &&
             !ws->stash()->context()->isShutdown())
    {
                    // the workspace has been opened, change the client
        ws->changeWorkspace (config.client(),
                             Gui::Workspace::NoCheckRunningOperations);

        Gui::CommandHandler::Handle (Gui::Command (Cmd_MainWindow),ws);

        if (mConfigWizard)
            workspaceChanged = true;
        else
            return ws;
    }
    else
    {
        return NULL;
    }

                    // setup workspace sync options from the config wizard
    if (mConfigWizard)
    {
        if (!mConfigWizard->syncingFiles())
        {
            ws->syncHint()->setSyncAction(Gui::SyncHint::DoNotSync);
        }
        else
        {
            ws->syncHint()->setSyncAction (Gui::SyncHint::SyncFiles);
            ws->syncHint()->setSyncVector (mConfigWizard->objectVector ());
            ws->syncHint()->setNumFiles   (mConfigWizard->mNumOfFiles);
        }

        if (workspaceChanged)
            return ws;
    }

    return ws;
}

//-----------------------------------------------------------------------------
//  installTranslators
//-----------------------------------------------------------------------------

void
SandstormApp::installTranslators()
{
    QString lang = LanguageWidget::LanguagePref();

    QString translationDir = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
    // Install P4V translations
    QTranslator*  translator = new QTranslator(sApp);
    const QString filename   = "p4v_" + lang;
    const bool    loaded     = translator->load( filename, translationDir );
    rLog(1) << "Translation loaded=" << loaded
            << " " << translationDir << " " << filename << qLogEnd;
    if(loaded)
        qApp->installTranslator( translator );
    else
        delete translator;

    // Install Qt translations for context menus and File/Font picker dialogs
    QTranslator* qtTranslator = new QTranslator(sApp);
    if(qtTranslator->load( "qt_" + lang, translationDir  ) )
        qApp->installTranslator( qtTranslator );
    else
        delete qtTranslator;

}

//-----------------------------------------------------------------------------
//  showLoginStatus
//-----------------------------------------------------------------------------

void
SandstormApp::showLoginStatus(Obj::Stash* stash, const QString& info)
{
    if (mCommandLine->mode() == P4VCommandRunner::P4V)
        Gui::Application::showLoginStatus(stash, info);
}


//-----------------------------------------------------------------------------
//  showAbout
//-----------------------------------------------------------------------------

void
SandstormApp::showAbout(const QVariant &)
{
    QString imageName(":/p4vimage.png");

#ifdef Q_OS_OSX
    if (mAboutDialog)
    {
        mAboutDialog->raise();
    }
    else
    {
    // Non-modal widget for the Mac
    //
        mAboutDialog = new UIAboutDialog(imageName, NULL);
        mAboutDialog->setWindowFlags( WinFlags::CloseOnlyFlags() );

        mAboutDialog->setAttribute( Qt::WA_DeleteOnClose );
        mAboutDialog->setWindowModality(Qt::NonModal);
        mAboutDialog->show();
        mAboutDialog->activateWindow();
        mAboutDialog->raise();
    }
#else
    UIAboutDialog* about = new UIAboutDialog(imageName, qApp->activeWindow());
    //about->setWindowFlags( WinFlags::ApplicationModalFlags() );
    about->setWindowFlags ( Qt::Dialog | Qt::WindowTitleHint |
        Qt::WindowCloseButtonHint );
    about->setAttribute(Qt::WA_DeleteOnClose);
    about->setWindowModality(Qt::ApplicationModal);

    StringID command = mCommandLine->startCommand();
    if (command == Cmd_Tree)
        about->setWindowTitle(tr("Perforce Revision Graph"));
    else if (command == Cmd_Annotate)
        about->setWindowTitle(tr("Perforce Time-lapse View"));
    else if (command == Cmd_RevisionHistory)
        about->setWindowTitle(tr("Perforce Revision History"));
    else if (command == Cmd_MainWindow)
        about->setWindowTitle(tr("About P4V"));

    about->show();
        about->setMaximumSize( about->size() );
#endif
}

//-----------------------------------------------------------------------------
//  showHelp
//-----------------------------------------------------------------------------

void
SandstormApp::showHelp(const QVariant &)
{
    // The focus widget has to send the event to support
    // context sensitive help
    QKeyEvent ev(QEvent::KeyPress, Qt::Key_Help, Qt::NoModifier);
    if (getLastFocus())
        qApp->sendEvent(getLastFocus(), &ev);
    // If we don't have a last focus, i.e. all windows are closed
    // on the Mac, we default to the "Getting Started with Perforce" page
    else
        qApp->sendEvent(this, &ev);
}

//-----------------------------------------------------------------------------
//  showInfo - calls UIWorspace2 showInfo
//-----------------------------------------------------------------------------

void
SandstormApp::showInfo(const QVariant & var)
{
    Gui::Workspace* ws = getActiveWorkspace();
    if (ws)
    {
        UIWorkspace2 * workspace = dynamic_cast<UIWorkspace2*>(ws->windowManager()->mainWorkspaceWindow());
        if (workspace)
            workspace->showInfo(var);
    }
}

//-----------------------------------------------------------------------------
//  showWhatsNew - calls UIWorspace2 showWhatsNew
//-----------------------------------------------------------------------------

void
SandstormApp::showWhatsNew(const QVariant & var)
{
    Gui::Workspace* ws = getActiveWorkspace();
    if (ws)
    {
        UIWorkspace2 * workspace = dynamic_cast<UIWorkspace2*>(ws->windowManager()->mainWorkspaceWindow());
        if (workspace)
            workspace->showWhatsNew(var);
    }
}

//-----------------------------------------------------------------------------
//  showGotoDialog
//-----------------------------------------------------------------------------

void
SandstormApp::showGotoDialog(const QVariant & v)
{
    Gui::Workspace* ws = getActiveWorkspace();
    if (ws)
    {
        Gui::CommandHandler::Handle(Gui::Command(Cmd_GotoDialog), ws, v);
    }
}

void
SandstormApp::reloadDialog(const QVariant & v)
{
    Obj::ObjectType type = static_cast<Obj::ObjectType>(v.toInt());
    UISpecPickerWithType::CreateReloadDialog(
            UISpecList::ObjFormToFormType(type),
            getActiveWorkspace());
}

bool
SandstormApp::isTopLevelWidgetOpen()
{
    foreach (QWidget* w, QApplication::topLevelWidgets())
    {
        if (w->testAttribute(Qt::WA_QuitOnClose) && !w->isHidden())
            return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
//  quit
//-----------------------------------------------------------------------------

void
SandstormApp::quit(const QVariant &)
{
    saveOpenWorkspacesList();

    // close all workspace windows without resaving open workspaces list
    disconnect(qApp,
            SIGNAL(lastWindowClosed()),
            this,
            SLOT(onLastWindowClosed()));
    qApp->closeAllWindows();
    connect(qApp,
            SIGNAL(lastWindowClosed()),
            this,
            SLOT(onLastWindowClosed()));

        // unfortunately, QApplication::closeAllWindows() doesn't say
        // if it succeeded, so we need to test if any windows rejected close
    if (!isTopLevelWidgetOpen())
    {
        qApp->quit();
    }
}

void
SandstormApp::onLastWindowClosed()
{
        // this slot gets called whenever a window closes.
        // except when closing all windows due to quit request
    saveOpenWorkspacesList();
}

//-----------------------------------------------------------------------------
//  addConnection
//-----------------------------------------------------------------------------

void
SandstormApp::addConnection(const QVariant &)
{
    ConnectionEditorCaller caller( qApp->activeWindow(), "ConnectionEditor", true );
    caller.doCall(getActiveWorkspace()->stash()->configuration());
}

//-----------------------------------------------------------------------------
//  manageConnection
//-----------------------------------------------------------------------------

void
SandstormApp::manageConnection(const QVariant &)
{
    ConnectionContainerEditor *ced =
            new ConnectionContainerEditor(NULL,
                                          qApp->activeWindow(),
                                          WinFlags::ApplicationModalFlags() | WinFlags::CloseOnlyFlags());
    ced->setWindowModality(Qt::ApplicationModal);
    ced->exec();
}

//-----------------------------------------------------------------------------
//  openConnection
//-----------------------------------------------------------------------------

void
SandstormApp::openConnection(const QVariant &)
{
    startNewWorkspace();
}

//-----------------------------------------------------------------------------
//  connectionWizard
//-----------------------------------------------------------------------------

void
SandstormApp::connectionWizard(const QVariant &)
{
    runConnectionWizard(false);
}

//-----------------------------------------------------------------------------
//  newUser
//-----------------------------------------------------------------------------

void
SandstormApp::newUser(const QVariant &)
{
    startNewWorkspace();
}

//-----------------------------------------------------------------------------
//  customize
//-----------------------------------------------------------------------------

void
SandstormApp::customize(const QVariant &)
{
    QDir dir = QDir::home();
    Gui::Workspace* ws = getActiveWorkspace();
    if (ws)
    {
        Obj::Stash* stash = ws->stash();
        if (stash->isClientValid())
            dir = stash->rootDirectoryPath();
    }
    CustomizeTools * win =
        new CustomizeTools(dir, qApp->activeWindow(),
                winKey::CustomTools,
                WinFlags::ApplicationModalFlags() );
    win->setWindowModality(Qt::ApplicationModal);

    if (win->exec() == QDialog::Accepted)
    {
        if (ws)
        {
            UIWorkspace2 * workspace = dynamic_cast<UIWorkspace2*>(ws->windowManager()->mainWorkspaceWindow());
            if (workspace)
                workspace->updateCustomToolsShortcuts();
        }
    }
}

//------------------------------------------------------------------------------
//  API for handling connections
//------------------------------------------------------------------------------

void
SandstormApp::onConfigurationChanged(const Op::Configuration & newC,
const Op::Configuration & oldC)
{
    Q_UNUSED(newC);
    Q_UNUSED(oldC);
    mCheckConnectDone = true;
}

// There are some small waits in config change that we need to heed
void
SandstormApp::setConfigurationAndWait(Obj::Stash* stash,
                                      const Op::Configuration& config)
{
    mCheckConnectDone = false;
    connect(stash, SIGNAL(configurationChanged(const Op::Configuration &,
                                               const Op::Configuration &)),
            this, SLOT(onConfigurationChanged(const Op::Configuration &,
                                              const Op::Configuration &)));

    stash->setConfiguration(config);

    // most of the above is synchronous: this will wait for a very short time
    while (!mCheckConnectDone)
        QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents, 250);

    // stash pointer may go stale so don't keep this connection hanging around
    // reconfiguration may also happen elsewhere, and we don't want it coming here
    disconnect(stash, SIGNAL(configurationChanged(const Op::Configuration &,
                                                  const Op::Configuration &)),
               this, SLOT(onConfigurationChanged(const Op::Configuration &,
                                                 const Op::Configuration &)));

    // This will wait but not block until tasks are done
    while (!stash->context()->wait(100))
        ;
}

//------------------------------------------------------------------------------
//  openConnectionHandler
//  This appears to be very similar to showWorkspace
//------------------------------------------------------------------------------

void
SandstormApp::openConnectionHandler(const Op::Configuration & config_)
{
    Op::Configuration config = config_;

    // charset from the connection index.
    Gui::P4VConnectionMaps::SetCharset(&config);

    // XXX Check with Caedmon to see if we can replace
    // this block with startConnectionChecker (config)
    //
    //
    // setup a temporary stash so that we can check the
    // connection.  This mainly sets up the error prompter
    // for error feedback.
    Obj::Stash stash;
    stash.setCheckConnectionAfterConfigChange(false);
    stash.context()->setErrorPrompter(new Gui::ErrorPrompter(0, &stash));
    setConfigurationAndWait(&stash, config);

    // is the connection we are about to use valid
    Op::CheckConnectionStatus status = stash.checkConnection();

    checkStatusAndShow(config, status);
}

bool
SandstormApp::checkStatusAndShow(const Op::Configuration& config,
                                 const Op::CheckConnectionStatus status)
{
                        // the status of this connection is good, show workspace
                        // and make sure that the connection is still good
    if (status == Op::CCS_Ok)
    {
        showWorkspace (config);
        return true;
    }
    else if (status == Op::CCS_ClientIsUnloaded)
    {
        QString msg = tr ("Unable to connect to the server %1 as user '%2'\n\n").arg(config.port(), config.user());
        msg.append(tr("Client '%1' has been unloaded, and must be reloaded to be used.").arg(config.client()));
        PMessageBox::information(qApp->activeWindow(), Platform::ApplicationName(), msg);
        return false;
    }
    else if (status == Op::CCS_ClientStreamIsUnloaded)
    {
        QString msg = tr ("Unable to connect to the server %1 as user '%2'\n\n").arg(config.port(), config.user());
        msg.append(tr("You must reload the stream associated with client '%1' in order to use it.").arg(config.client()));
        PMessageBox::information(qApp->activeWindow(), Platform::ApplicationName(), msg);
        return false;
    }

    const bool knownConnection =
            Gui::P4VConnectionMaps::IsConfigurationMapped(config);
    if (knownConnection && promptConnectNoConnection(config))
    {
        showWorkspace(config, true);
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
//  promptConnectNoConnection
//------------------------------------------------------------------------------

bool
SandstormApp::promptConnectNoConnection (const Op::Configuration & config)
{

    QMessageBox::StandardButton connectToServer;
                // Good Connection configuration, prompt the user to run
                // without a connection
    QString port    = config.port ();
    QString user    = config.user ();
    QString appName = Platform::ApplicationName ();

    QString question = tr ("Unable to connect to the server %1 as user '%2'\n\n").arg(port, user);
    question.append   (tr ("Would you like to run %1 without a connection?").arg(appName));

    connectToServer = PMessageBox::warning (qApp->activeWindow (),
                                            appName,
                                            question,
                                            QMessageBox::Yes | QMessageBox::No,
                                            QMessageBox::Yes);

    if (connectToServer == QMessageBox::Yes)
        return true;

    return false;
}

//------------------------------------------------------------------------------
//  startConnectionChecker
//------------------------------------------------------------------------------

void
SandstormApp::startConnectionChecker (Op::Configuration & config)
{
    Gui::ConnectionChecker * checker;

    checker = new Gui::ConnectionChecker (config, this);

    connect (checker, SIGNAL (done                    (Op::CheckConnectionStatus)),
             this,    SLOT   (openConnectionCheckDone (Op::CheckConnectionStatus)));
    checker->start ();
}

//------------------------------------------------------------------------------
//  openConnectionCheckDone
//------------------------------------------------------------------------------

void
SandstormApp::openConnectionCheckDone (Op::CheckConnectionStatus result)
{
    Gui::ConnectionChecker * checker;

    checker = dynamic_cast<Gui::ConnectionChecker*>(sender());
    Q_ASSERT (checker);

    if (!checker)
        return;

    Op::Configuration config = checker->configuration ();

                    // show workspace if possible
    switch (result)
    {
        case Op::CCS_NoSuchClient:
        case Op::CCS_ClientLockedHost:
        case Op::CCS_ClientNoValidRoot:
        case Op::CCS_ClientIsUnloaded:
        {
            config.setClient (QString());
        }
        case Op::CCS_Ok:
        {
                            // delay the showWorkspace() call for later,
                            // or we run into Op:Schedule done() and wait() deadlocking
            QTimer::singleShot (10, this, SLOT (showWorkspace ()));
            break;
        }
        default:
            break;
    }
}

void
SandstormApp::setupShortcuts ()
{
    if (mShortcutCmds)
        return;
                        // Build hotkey shortcuts for connections
    QStringList cmdList;
    cmdList << SHCT_FAVCONNECTION_1
            << SHCT_FAVCONNECTION_2
            << SHCT_FAVCONNECTION_3
            << SHCT_FAVCONNECTION_4
            << SHCT_FAVCONNECTION_5
            << SHCT_FAVCONNECTION_6
            << SHCT_FAVCONNECTION_7
            << SHCT_FAVCONNECTION_8
            << SHCT_FAVCONNECTION_9;

    mShortcutCmds = new ShortCutCmds (this);
    P4VConnections connections;
    ConnectionModel * model = connections.model (this);

    mShortcutCmds->buildShortcuts (cmdList, model, NULL);
}

void
SandstormApp::handleShortCut (const QString & key)
{
    ConnectionDef * def;

    if (!mShortcutCmds)
        return;

    if ((def = (ConnectionDef *) (mShortcutCmds->definition (key))))
    {
        Op::Configuration config = Gui::ConnectionsMenu::ConfigConnectionDef (*def);

        // see comments:
        fillInDvcsInfo(config);

        openConnectionHandler (config);
    }
}

ShortCutCmds *
SandstormApp::shortCutCmds ()
{
    return mShortcutCmds;
}

bool
SandstormApp::runCommand(const QString& cmd, QString& error, QObject* cb)
{
    P4VCommandRunner    runner;
    bool ok = ClientPortCommandHandler::parseJsonCommand(cmd, runner, error);

    if (!ok)
        return false;

    ClientPortCommandHandler* handler = new ClientPortCommandHandler(runner);

    if (cb)
    {
        connect(handler, SIGNAL(status(const QString&)),
                cb,      SLOT(status(const QString&)));
        connect(handler, SIGNAL(destroyed()),
                cb,      SLOT(done()));
    }

    return ok;
}

void
SandstormApp::onCommandError(const QString& msg)
{
    showCommandLineError(msg);
}

void
SandstormApp::onCommandInfo(const QString& msg)
{
    showCommandLineInfo(msg);
}
