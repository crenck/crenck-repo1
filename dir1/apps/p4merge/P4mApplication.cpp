#include "AboutBox.h"
#include "ActionStore.h"
#include "ActionUtil.h"
#include "CharsetCombo.h"
#include "CharSetMap.h"
#include "CmdAction.h"
#include "CmdActionGroup.h"
#include "CommandIds.h"
#include "CompareDialog.h"
#include "Cp858Codec.h"
#include "DebugLog.h"
#include "DialogUtil.h"
#include "HelpLauncher.h"
#include "ImgMainWindow.h"
#include "LanguageWidget.h"
#include "LogUtil.h"
#include "MergeMainWindow.h"
#include "ObjectDumper.h"
#include "ObjectNameUtil.h"
#include "P4mApplication.h"
#include "P4mArgs.h"
#include "p4massert.h"
#include "P4mCommandGlobals.h"
#include "p4mdocument.h"
#include "p4mdocumentbuilder.h"
#include "P4mGlobals.h"
#include "P4mPreferencesDialog.h"
#include "P4mPrefs.h"
#include "Platform.h"
#include "PMessageBox.h"
#include "Prefs.h"
#include "SettingKeys.h"
#include "StringID.h"
#include "StyleSheetUtil.h"
#include "WebKitMainWindow.h"
#include "WinFlags.h"
#include "XMLManager.h"

#include "stdhdrs.h"
#include "diff.h"

#include <QCheckBox>
#include <QClipboard>
#include <QDebug>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QImageReader>
#include <QLabel>
#include <QLayout>
#include <QLibraryInfo>
#include <QMenuBar>
#include <QPushButton>
#include <QResource>
#include <QScrollArea>
#include <QStyleFactory>
#include <QTextEdit>
#include <QTextStream>
#include <QTime>
#include <QTimer>
#include <QTranslator>

qLogCategory( "P4mApplication" );

// THIS LOADING OF THE PLUGINS NO lONGER WORKS IN QT5..... NEEDS TO BE FIXED
/*
// import static-built image format libs
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
// END OFF..... NEEDS TO BE FIXED

#if defined (Q_OS_OSX) || defined (Q_OS_WIN)
#define GUI_APPLICATION
#endif

//#define TEST_FAST_PLAIN_TEST
#ifdef TEST_FAST_PLAIN_TEST
#include <LightweightTextView3.h>
#endif

#define DEFAULT_WINDOW_WIDTH 500

int globalMenuBarHeight()
{
    int height = 20;

    return height;
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


P4mApplication::P4mApplication( int & argc, char ** argv )
        : QApplication( argc, argv ), Responder( this )
{

    QStringList args;
    for (int i = 0; i < argc; ++i)
        args.append(QString::fromLocal8Bit(argv[i]));

    mCompareFilesDialog = NULL;
    mHelpLauncher = NULL;
    mZeroArguments = false;
    mGridOption = Optimal;

    // the default value is 4
    qApp->setStartDragDistance(12);
    ObjectDumper::InstallOnApplication();
    Platform::SetApplicationName("P4Merge");
    Platform::SetApplication(Platform::P4Merge_App);

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

    QString configDir;

    if ( Platform::IsMac() )
    {
        QDir tempDir = QDir::home();
        tempDir.cd( "Library" );
        tempDir.cd( "Preferences" );
        configDir = tempDir.absoluteFilePath( "com.perforce.p4merge" );

        setQuitOnLastWindowClosed( false );
    }
    else
    {
        configDir = QDir::home().absoluteFilePath( ".p4merge" );
    }


#ifdef QT_DEBUG
    LogUtil::Configure(true,
                       configDir + "/p4mergelog.config",
                       configDir + "/p4mergelog.txt",
                       512 * 1024);
#else
    DebugLog::Enable(false);
#endif

    qLogToCategory("startup",1)
            << "---- application startup "
            << QTime::currentTime().toString()
            << " ----"
            << qLogEnd;

    XMLManager::setConfigDir( configDir );

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

    // Load translation files
    QString lang = LanguageWidget::LanguagePref();
    QString translationDir = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
    QTranslator* translator = new QTranslator(0);
    translator->setObjectName( "p4merge translator" );
    if(translator->load( "p4merge_" + lang, translationDir ))
        installTranslator( translator );
    else
        delete translator;

    // Install Qt translations for context menus and File/Font picker dialogs
    QTranslator* qtTranslator = new QTranslator(0);
    if(qtTranslator->load( "qt_" + lang, translationDir  ) )
        qApp->installTranslator( qtTranslator );
    else
        delete qtTranslator;

    if (!QResource::registerResource(Platform::ImageResourceFile()))
    {
        PMessageBox::critical(0,
                              Platform::ApplicationName(),
                              tr("Error initializing %1.\nUnable to load resource file.").arg(Platform::ApplicationName()));
        qWarning("error loading resource file");
    }

    loadGlobalActions();

    QMenuBar * mb = Platform::GlobalMenuBar();
    if ( mb )
        initMenuBar( mb );

    // register our defaults

    P4mPrefs::SetDefaultValue( MergePrefs::FONT,       Platform::DefaultFixedPitchFont());
    P4mPrefs::SetDefaultValue( MergePrefs::TABWIDTH,   Platform::IsUnix() ? 8 : 4 );
    P4mPrefs::SetDefaultValue( MergePrefs::CHARSET,    P4mPrefs::ToString( P4mPrefs::System ) );

    P4mPrefs::SetDefaultValue( MergePrefs::LINEENDING, P4mPrefs::ToString(
      Platform::IsWin() ?
        P4mPrefs::Windows :
        P4mPrefs::Unix ) );

    P4mPrefs::SetDefaultValue( MergePrefs::DIFFOPTION, P4mPrefs::ToString( P4mPrefs::DashL ) );
    P4mPrefs::SetDefaultValue( MergePrefs::INLINEDIFFSENABLED, true );
    P4mPrefs::SetDefaultValue( MergePrefs::TABINSERTSSPACES, false );
    P4mPrefs::SetDefaultValue( MergePrefs::SHOWTABSSPACES, false );

    setFont( Prefs::Value( Prefs::FQ( MergePrefs::APPFONT ), QFont() ) );

    new P4CP858Codec();
}

P4mApplication::~P4mApplication()
{
    delete mCompareFilesDialog;
    delete mHelpLauncher;
    Prefs::Flush();
}

bool
P4mApplication::processArguments(const QStringList& appArgs)
{
//#define SHOW_ARGS
#ifdef SHOW_ARGS
    QString str;
    for ( int i = 0; i < appArgs.size(); i++ )
    {
        str.append( QString("Arg %1: %2\n").arg(i).arg(appArgs.at(i) ));
    }

    PMessageBox::information( NULL, QString(), str );
#endif

   P4mArgs args;


// On the Mac the showCompareDialog will be callled later,
// once the application receives an "open application" event.
// If we call this now this dialog will always come up with the shim
// that's why we don't want call it here

    if ( appArgs.size() == 1 )
    {
        // Workaround for job045804
        //
        if ( !Platform::IsMac() )
            showCompareDialog();
        else
        {
            mZeroArguments = true;
            QTimer::singleShot(500, this, SLOT( scheduleShowCompareDialog() ));
        }
        return true;
    }

    args.parseArgs( appArgs );

    if ( args.helpRequested )
    {

#ifdef GUI_APPLICATION
        QDialog* dlg = new QDialog(0, Qt::WindowCloseButtonHint);
        QVBoxLayout* layout = new QVBoxLayout(dlg);
        layout->setContentsMargins(0,0,0,0);
        QScrollArea *scrollArea = new QScrollArea();
        QLabel* helpText = new QLabel(args.helpMessage());
        scrollArea->setWidget(helpText);
        layout->addWidget(scrollArea);
        dlg->setMinimumSize(500,520);
        dlg->setWindowTitle( tr("%1 Command-line Help").arg(Platform::ApplicationName()) );
        dlg->exec();
        delete dlg;
        return false;
#else
        QTextStream out(stdout);
        out << args.helpMessage();
        return false;
#endif
    }

    if ( args.versionRequested )
    {
#ifdef GUI_APPLICATION
        showAbout();
        return true;
#else
        QTextStream out(stdout);
        out << args.versionMessage();
        return false;
#endif
    }

    int exitValue = 0;

    if ( !args.checkFiles() )
        exitValue = -1;

    if ( args.gridOption == Guarded )
        mGridOption = Guarded;

    //
    // If we failed, then exit
    //
    if ( args.hasErrorMessage() )
    {
        QString msg = tr("Errors: %1\n\nUse 'p4merge -h' for more help.").arg(args.errorMessage());
#ifdef GUI_APPLICATION
        PMessageBox::information( NULL, tr("Error starting %1").arg(Platform::ApplicationName()), msg);
#else
        QTextStream err(stderr);
        err << endl << msg << endl;
#endif
        exitValue = -1;
    }

    //
    // If we failed, then exit
    //
    if ( exitValue != 0 )
        return false;


    QMainWindow * mw = createMainWindow( args.files[P4m::LEFT], args.files[P4m::RIGHT],args.files[P4m::BASE], &args );

    if ( mw )
    {
        mw->show();
        mw->activateWindow();
        mw->raise();
        return true;
    }
    else
        return false;

}

QString
P4mApplication::realApplicationDirPath()
{
    // normally, QApplication gets it right
    QString  appPath = qApp->applicationDirPath ();
    appPath = QDir::toNativeSeparators (appPath);

    return appPath;
}

void
P4mApplication::customizeAction( const StringID & cmd, QAction * a )
{
    if ( cmd == Cmd_Close )
        a->setEnabled( qApp->activeWindow() );
}


bool
P4mApplication::notify(QObject* obj, QEvent* e)
{
    ActionUtil::FilterUnicodeControlCharacterMenu( obj, e);
    return QApplication::notify(obj, e);
}

void
P4mApplication::scheduleShowCompareDialog()
{
    if (mZeroArguments)
        showCompareDialog();
}

void
P4mApplication::showCompareDialog()
{
    if ( !mCompareFilesDialog )
    {
        mCompareFilesDialog = new CompareDialog( NULL, WinFlags::NoParentFlags());
        mCompareFilesDialog->setObjectName( "CompareDialog" );
        connect( mCompareFilesDialog, SIGNAL(compareClicked()), this, SLOT(dialogCompared()) );
    }

    mCompareFilesDialog->show();
    mCompareFilesDialog->activateWindow();
    mCompareFilesDialog->raise();
}


void
P4mApplication::hideCompareFilesDialog()
{
    if ( !mCompareFilesDialog )
        return;

    mCompareFilesDialog->hide();
}

    // admin and merge are a little trickier since they don't have a single server
    // connection. For now, we're just going to go to online help.
HelpLauncher *
P4mApplication::helpWindow()
{
    if (!mHelpLauncher)
        mHelpLauncher = new HelpLauncher("p4merge");

    QString countryCode = Prefs::Value(Prefs::FQ(p4v::prefs::ApplicationLanguage), QString("en"));

    mHelpLauncher->setHelpLocation(kOnlineHelpPrefix);
    mHelpLauncher->setLocale(countryCode);

    return mHelpLauncher;
}


QMainWindow *
P4mApplication::createMainWindow( QString file1, QString file2, P4mArgs * args )
{
    return createMainWindow( file1, file2, QString(), args );
}

/*! \brief  Is this file one of the known ambiguous image types:
            files that might be text, might be image?

    See http://local.wasp.uwa.edu.au/~pbourke/dataformats/ppm/
    for PPM/PGM/PBM/PNM documentation.
*/
static bool
IsAmbiguousImageType( const QImageReader & reader )
{
    qLog(4) << "IsAmbiguousImageType " << reader.fileName()
            << " " << reader.format() << " " << reader.canRead()
            << qLogEnd;

    if ( !reader.canRead() ) return false;

    const QByteArray format = reader.format();

    return    format == "xbm"
           || format == "pbm"
           || format == "pgm"
           || format == "pnm"
           || format == "ppm"
           ;
}

/*! \brief  Break one or more QImageReader objects so that
            they can no longer read an image.
*/
static void
ForceText(
    QImageReader * img1,
    QImageReader * img2 = NULL,
    QImageReader * img3 = NULL )
{
    if ( img1 ) img1->setDevice( NULL );
    if ( img2 ) img2->setDevice( NULL );
    if ( img3 ) img3->setDevice( NULL );
}

/*! \brief  If you are trying to look at svg files in p4merge, we assume you want to look at them as xml/text files.
            otherwise, you would pick an other diff tool for that extention.
*/
static void
ForceTextIfSVG(
                QImageReader &  img1,
                QImageReader &  img2,
                QImageReader &  img3)
{
    if ( img1.canRead() )
    {
        const QByteArray format1 = img1.format();
        if (format1 == "svg")
            img1.setDevice( NULL );
    }

    if ( img2.canRead() )
    {
        const QByteArray format2 = img2.format();
        if (format2 == "svg")
            img2.setDevice( NULL );
    }

    if ( !img3.fileName().isEmpty() && img3.canRead() )
    {
        const QByteArray format3 = img3.format();
        if (format3 == "svg")
            img3.setDevice( NULL );
    }
}

/*! \brief  De-image-ify ambiguous image files that the user wants to see
            as text.

    XBM and PBM are image formats that look like text files, so text files often
    look like XBM and PBM files to QImageReader::canRead().

    See job{030484,job030484} for XBM and job{030798,job030798} for PBM bugs.
*/
static void
ForceTextIfNecc(
    QImageReader &  img1,
    QImageReader &  img2,
    QImageReader &  img3,
    const bool      haveFile3,
    bool &          cancel )
{
    cancel = false;

    // Any ambiguous files that will confuse our calling code?
    const bool ambi1 =              IsAmbiguousImageType( img1 );
    const bool ambi2 =              IsAmbiguousImageType( img2 );
    const bool ambi3 = haveFile3 && IsAmbiguousImageType( img3 );

    qLog(1) << "P4mApplication::ForceTextIfNecc() "
            << " " << img1.fileName()
            << " " << img2.fileName()
            << " " << img3.fileName()
            << " " << ambi1
            << " " << ambi2
            << " " << ambi3
            << " " << img1.format()
            << " " << img2.format()
            << " " << img3.format()
            << qLogEnd;

    // If nothing's ambiguous, our work is done.
    if ( !ambi1 && !ambi2 && !ambi3 ) return;

    if ( ambi1 && ambi2 && ambi3 )
    {
        qLog(2) << "Three-way ambiguous image always compares as text." << qLogEnd;
        ForceText( &img1, &img2, &img3 );
        return;
    }

    // Three-way with disambiguous image: prohibited (no image three-ways).
    // Three-way with no disambiguous images: three-way text.
    if ( haveFile3 )
    {
        const bool  otherImg1 = !ambi1 && img1.canRead();
        const bool  otherImg2 = !ambi2 && img2.canRead();
        const bool  otherImg3 = !ambi3 && img3.canRead();
        qLog(2) << "Three-way "
                << " ambi="      << ambi1 << ambi2 << ambi3
                << " otherImg=" << otherImg1 << otherImg2 << otherImg3
                << qLogEnd;

        if ( !otherImg1 && !otherImg2 && !otherImg3 )
        {
            qLog(2) << "No other image files. Compare as text." << qLogEnd;
            ForceText( &img1, &img2, &img3 );
        }
        else
        {
            qLog(2) << "Ambiguous and other image format files. Three-way prohibited." << qLogEnd;
        }
        return;
    }

    // Everything is two-way from here to end of function.

    // 0 1  One ambiguous file and one disambiguous image?
    // 1 0
    if (( ambi1 && !ambi2 ) || ( !ambi1 && ambi2 ))
    {
        // Compare as images if other is image.
        if (   ( !ambi1 && img1.canRead() )
            || ( !ambi2 && img2.canRead() ))
        {
            qLog(2) << "One ambiguous and one disambiguous image file: compare as image." << qLogEnd;
            return;
        }

        qLog(2) << "One ambiguous image and one text file: compare as text." << qLogEnd;
        ForceText( &img1, &img2 );
        return;
    }

        // 1 1  Two ambiguous image files = Ask the user what to do.
    if ( ambi1 && ambi2 )
    {
        qLog(2) << "Two ambiguous image files. Asking..." << qLogEnd;







            // Include filenames in dialog text.
        QString file1 = QFileInfo(img1.fileName()).fileName();
        QString file2 = QFileInfo(img2.fileName()).fileName();
        if (file1 == file2)
        {
                // Fall back to full paths when file names are identical.
            file1 = img1.fileName();
            file2 = img2.fileName();
        }

        DontAsk::ImageDiffDialog::Mode imageMode = DontAsk::ImageDiffDialog::Different;
        if (img1.format() == img2.format())
            imageMode = DontAsk::ImageDiffDialog::Same;

        DontAsk::ImageDiffDialog dlg(imageMode,
                                     file1,
                                     file2,
                                     QString::fromUtf8(img1.format()),
                                     QString::fromUtf8(img2.format()));
        dlg.exec();
        DontAsk::ImageDiffDialog::Request result = dlg.request();

        qLog(2) << "Result returned " << result << qLogEnd;

        switch (result)
        {
            case DontAsk::ImageDiffDialog::Cancelled:
                cancel = true;
                break;
            case DontAsk::ImageDiffDialog::Request_Image:
                break;
            case DontAsk::ImageDiffDialog::Request_Text:
                ForceText( &img1, &img2 );
                break;
            default :                       // Unexpected!
                break;
        }
        return;
    }
}

QMainWindow *
P4mApplication::createMainWindow( QString file1, QString file2, QString file3, P4mArgs * args )
{
    if (file3.isNull() && args)
    {
        QString urlPref = Prefs::Value( Prefs::FQ( MergePrefs::DOCXCOMPAREURL ), QString() );
        if (!args->compareUrl.isNull() || (!urlPref.isNull() && !urlPref.isEmpty()))
        {
            QFileInfo infoFile1(file1);
            QFileInfo infoFile2(file2);
            QString extFile1 = infoFile1.suffix();
            QString extFile2 = infoFile2.suffix();
            if (extFile1 == "docx" && extFile2 == "docx")
            {
                WebKitMainWindow * mw = new WebKitMainWindow();
                mw->setAttribute( Qt::WA_DeleteOnClose );
                mw->loadFiles( file1, file2, *args );
                return mw;
            }
        }
    }

    QImageReader img1( file1 );
    QImageReader img2( file2 );
    QImageReader img3( file3 );

    bool    cancel = false;
    ForceTextIfNecc( img1, img2, img3, !file3.isNull(), cancel );
    if (cancel) return NULL;

    ForceTextIfSVG( img1, img2, img3 );

    if ( !file3.isNull() && ( img3.canRead() || img1.canRead() ||  img2.canRead() ) )
    {
            PMessageBox::warning( NULL, Platform::ApplicationName(), tr("Image files can not be merged") , QMessageBox::Ok, QMessageBox::NoButton );
            return NULL;
    }

    if ( ( file3.isNull() && ( img1.canRead() && !img2.canRead() ) ) || ( !img1.canRead() && img2.canRead() ) )
    {
            PMessageBox::warning( NULL, Platform::ApplicationName(), tr("The formats of the two files are incompatible and can not be diffed") , QMessageBox::Ok, QMessageBox::NoButton );
            return NULL;
    }

    if ( img1.canRead() && img2.canRead() )
    {

        QByteArray fmt1 = img1.format();
        QByteArray fmt2 = img2.format();

        ImgMainWindow * mw = new ImgMainWindow( );
        mw->setAttribute( Qt::WA_DeleteOnClose );

        QString err = mw->loadFiles( file1, file2 );

        if ( args )
            mw->overrideOptions( *args );

        if ( !err.isNull() )
        {
            delete mw;
            mw = NULL;

            if ( err == "FilesTooBig" )
                return mw;

            // Because of magic numbers at the beginning of these files, canRead can return
            // true, but read returned false because they are not actual image files.
            // Most likely these are text files so we just do a text diff in this case.
            if ( fmt1 == "pbm" || fmt2 == "pbm" ||
                 fmt1 == "pgm" || fmt2 == "pgm" ||
                 fmt1 == "ppm" || fmt2 == "ppm")
                goto textdiff;

            // We had trouble reading the images so ask the user if they want to do a text diff
            QString msg = err;
            msg.append( tr("Would you like to diff the files as a textual diff?\n") );
            bool ok = QMessageBox::Yes ==
                PMessageBox::warning( NULL, Platform::ApplicationName(), msg, QMessageBox::Yes|QMessageBox::No, QMessageBox::No );
            if ( ok )
                goto textdiff;
        }
        return mw;
    }

textdiff:
    MergeMainWindow * mw = new MergeMainWindow( );
    mw->setAttribute( Qt::WA_DeleteOnClose );

    //  Override options here

    if ( args )
        mw->overrideOptions( *args );

    P4m::ReadErrors err = mw->loadFiles( file1, file2, file3 );

    if ( err != P4m::NoError )
    {
        mw->close();
        mw = NULL;
        if ( err == P4m::FileWarnBig || err == P4m::CancelLoad)
            return mw;

        if ( err != P4m::CharSetIsWrong && err != P4m::SystemCharSetOk )
            PMessageBox::warning( NULL, Platform::ApplicationName(), tr("Error opening files"), QMessageBox::Ok, QMessageBox::NoButton );

    }
    return mw;

}


void
P4mApplication::dialogCompared()
{
    // Check to see if paths are valid.
    //
    QString files[3] = { mCompareFilesDialog->basePath(),
                    mCompareFilesDialog->leftPath(),
                    mCompareFilesDialog->rightPath()
                    };
    int i = 0;
    while ( i<3 )
    {
        files[i] = files[i].trimmed();
        if ( !files[i].isEmpty() && !QFileInfo(files[i]).isFile() )
            break;
        i++;
    }

    // If we failed, show a dialog
    //
    if ( i < 3 )
    {
        QString type;
        switch (i)
        {
            case 0: type = tr("Base");  break;
            case 1: type = tr("1st");  break;
            case 2: type = tr("2nd"); break;
        }

        QString message = tr("%1 file not found.\n'%2' is not a valid file.",
                             "%1 is \"Base\", \"1st\", or \"2nd\". %2 is filename.")
                          .arg(type, files[i]);

        PMessageBox::warning( mCompareFilesDialog, Platform::ApplicationName(), message );
        return;
    }

    QMainWindow * mw = createMainWindow( files[P4m::LEFT], files[P4m::RIGHT], files[P4m::BASE] );
    if ( mw )
    {
        mw->show();
        mw->activateWindow();
        mw->raise();
    }
    mCompareFilesDialog->hide();
}


void
P4mApplication::loadGlobalActions()
{

    CmdAction * a = NULL;
    CmdTargetDesc APP_TARGET( qApp, CmdTargetDesc::HolderIsTheTarget );
    CmdTargetDesc GLOBAL_FOCUS_TARGET( qApp, CmdTargetDesc::FocusWidget );

    a = NewGlobalAction( Cmd_Open, APP_TARGET, tr("&New Merge or Diff...") );
    a->setShortcut( tr("CTRL+N") );
    a->setMenuRole( QAction::NoRole);

    a = NewGlobalAction( Cmd_TestTextEdit, APP_TARGET, tr("Test FastPlainTextEdit") );
    a->setMenuRole( QAction::NoRole);

    a = NewGlobalAction( Cmd_Close, APP_TARGET, tr("&Close") );
    a->setMenuRole( QAction::NoRole);

    if ( Platform::IsWin() )
    {
        QList<QKeySequence> winShortcuts;
        winShortcuts << tr("CTRL+W") << tr("Esc");
        a->setShortcuts( winShortcuts  );
    } else
        a->setShortcut( tr("CTRL+W") );

    a = NewGlobalAction( Cmd_Prefs, APP_TARGET, tr("Pre&ferences...") );
    a->setMenuRole( QAction::PreferencesRole );

    a = NewGlobalAction( Cmd_About, APP_TARGET, tr("&About %1").arg(Platform::ApplicationName()) );
    a->setMenuRole( QAction::AboutRole );

    a = NewGlobalAction( Cmd_Help, APP_TARGET, tr("%1 Help").arg(Platform::ApplicationName()) );
    a->setMenuRole( QAction::NoRole);

    if ( Platform::IsMac() )
    {
        if ( beforeLeopard() )
            a->setShortcut( tr("CTRL+?") );
        else
            a->setShortcut( Qt::Key_Question + Qt::CTRL + Qt::ALT );
    }
    else
    {
        a->setDefaultMenuText( tr("%1 &Help").arg(Platform::ApplicationName()) );
        a->setShortcut( tr("F1") );
    }

    a = NewGlobalAction( Cmd_Quit, APP_TARGET, tr("E&xit") );
    a->setMenuRole( QAction::QuitRole );

    a = NewGlobalAction( Cmd_Cut,        GLOBAL_FOCUS_TARGET, tr("Cu&t") );
    a->setShortcut( tr("CTRL+X") );
    a->setMenuRole( QAction::NoRole);

    a = NewGlobalAction( Cmd_Copy,       GLOBAL_FOCUS_TARGET, tr("&Copy") );
    a->setShortcut( tr("CTRL+C") );
    a->setMenuRole( QAction::NoRole);

    if (QApplication::clipboard()->supportsFindBuffer())
    {
        a = NewGlobalAction( Cmd_CopyToFind, GLOBAL_FOCUS_TARGET, tr("Us&e Selection for Find") );
        a->setShortcut( tr("CTRL+E") );
        a->setMenuRole( QAction::NoRole);
    }

    a = NewGlobalAction( Cmd_Paste,      GLOBAL_FOCUS_TARGET, tr("&Paste") );
    a->setShortcut( tr("CTRL+V") );
    a->setMenuRole( QAction::NoRole);

    a = NewGlobalAction( Cmd_Clear,      GLOBAL_FOCUS_TARGET, tr("Clear") );
    a->setMenuRole( QAction::NoRole);

    a = NewGlobalAction( Cmd_SelectAll, GLOBAL_FOCUS_TARGET, tr("Select &All") );
    a->setShortcut( tr("CTRL+A") );
    a->setMenuRole( QAction::NoRole);

    a = NewGlobalAction( Cmd_ToggleToolBar,      GLOBAL_FOCUS_TARGET, tr("&Toolbar") );

    a = NewGlobalAction( Cmd_Save, GLOBAL_FOCUS_TARGET, tr("&Save") );
    a->setShortcut( tr("CTRL+S") );
    a->setMenuRole( QAction::NoRole);
    a->setIcon( QIcon( ":/cmd_save.png" ) );
    ActionUtil::createToolTip(a);

    a = NewGlobalAction( Cmd_SaveAs, GLOBAL_FOCUS_TARGET, tr("Save &As...") );
    a->setMenuRole( QAction::NoRole);

    a = NewGlobalAction( Cmd_Undo, GLOBAL_FOCUS_TARGET, tr("&Undo") );
    a->setShortcut( tr("CTRL+Z") );
    a->setMenuRole( QAction::NoRole);
    a->setIcon( QIcon( ":/cmd_undo.png" ) );
    ActionUtil::createToolTip(a);

    a = NewGlobalAction( Cmd_Redo, GLOBAL_FOCUS_TARGET, tr("Redo") );
    a->setMenuRole( QAction::NoRole);
    a->setIcon( QIcon( ":/cmd_redo.png" ) );
    if ( Platform::IsMac() )
    {
        a->setShortcut( tr("CTRL+SHIFT+Z") );
    }
    else
    {
        a->setDefaultMenuText( tr("&Redo") );
        a->setShortcut( tr("CTRL+Y") );
    }
    ActionUtil::createToolTip(a);

    a = NewGlobalAction( Cmd_SideBySide, GLOBAL_FOCUS_TARGET, tr("&Double Pane Diff Layout") );
    a->setMenuRole( QAction::NoRole);

    a = NewGlobalAction( Cmd_Combined,     GLOBAL_FOCUS_TARGET, tr("&Single Pane Diff Layout") );
    a->setMenuRole( QAction::NoRole);


    a = NewGlobalAction( Cmd_Font,      GLOBAL_FOCUS_TARGET, tr("&Font...") );
    a->setMenuRole( QAction::NoRole);

    a = NewGlobalAction( Cmd_TabWidth, GLOBAL_FOCUS_TARGET, tr("T&ab and Space Options...") );
    a->setMenuRole( QAction::NoRole);


    a = NewGlobalAction( Cmd_ZoomIn, GLOBAL_FOCUS_TARGET, tr("Zoom In") );
    a->setMenuRole( QAction::NoRole);

    a = NewGlobalAction( Cmd_ZoomOut,     GLOBAL_FOCUS_TARGET, tr("Zoom Out") );
    a->setMenuRole( QAction::NoRole);

    a = NewGlobalAction( Cmd_FitToWindow, GLOBAL_FOCUS_TARGET, tr("Fit to Window") );
    a->setMenuRole( QAction::NoRole);

    a = NewGlobalAction( Cmd_ActualSize,     GLOBAL_FOCUS_TARGET, tr("Actual Size") );
    a->setMenuRole( QAction::NoRole);


    a = NewGlobalAction( Cmd_Link, GLOBAL_FOCUS_TARGET, tr("Link Images") );
    a->setMenuRole( QAction::NoRole);

    a = NewGlobalAction( Cmd_Highlight,     GLOBAL_FOCUS_TARGET, tr("Highlight Differences") );
    a->setMenuRole( QAction::NoRole);


    CmdActionGroup * ag = ActionStore::Global().createActionGroup( Cmd_LineEndings );
    ag->setTargetDescription( GLOBAL_FOCUS_TARGET );
    for ( int i = P4mPrefs::Unix; i < P4mPrefs::LineEndingCount; i++ )
    {
        a = ag->addActionWithData( i );
        a->setDefaultMenuText( P4mPrefs::LineEndingDescription( (P4mPrefs::LineEnding)i ) );
    }

    ag = ActionStore::Global().createActionGroup( Cmd_Encoding );
    ag->setTargetDescription( GLOBAL_FOCUS_TARGET );
    for ( int i = P4mPrefs::System; i < P4mPrefs::Count; i++ )
    {
        a = ag->addActionWithData( i );
        a->setDefaultMenuText( P4mPrefs::CharSetDescription( (P4mPrefs::Charset)i ) );
        a->setMenuRole( QAction::NoRole);
    }


    ag = ActionStore::Global().createActionGroup( Cmd_DiffOption );
    ag->setTargetDescription( GLOBAL_FOCUS_TARGET );
    for ( int i = P4mPrefs::Line; i < P4mPrefs::DiffFlagCount; i++ )
    {
        a = ag->addActionWithData( i );
        a->setDefaultMenuText( P4mPrefs::DiffOptionDescription( i ) );
        a->setMenuRole( QAction::NoRole);
    }

    a = NewGlobalAction( Cmd_ShowBase,    GLOBAL_FOCUS_TARGET, tr("&Base File") );
    a->setMenuRole( QAction::NoRole);


    ag = ActionStore::Global().createActionGroup( Cmd_EditDiff );
    ag->setTargetDescription( GLOBAL_FOCUS_TARGET );
    a = ag->addActionWithData( P4m::LEFT );
    a->setDefaultMenuText( tr("&Edit Left Pane") );
    a->setIcon( QIcon( ":/edit_left.png" ) );
    a->setToolTip(tr("Edit file in left pane"));
    a->setIconVisibleInMenu( false );

    a = ag->addActionWithData( P4m::RIGHT );
    a->setMenuRole( QAction::NoRole);
    a->setDefaultMenuText( tr("E&dit Right Pane") );
    a->setIcon( QIcon( ":/edit_right.png" ) );
    a->setToolTip(tr("Edit file in right pane"));
    a->setIconVisibleInMenu( false );

    a = NewGlobalAction( Cmd_ShowInlineDiffs,    GLOBAL_FOCUS_TARGET, tr("&Inline Differences") );
    a->setShortcut( tr("CTRL+I") );
    a->setMenuRole( QAction::NoRole);
    a->setIcon( QIcon( ":/inline_icon.png" ) );
    a->setToolTip(tr("Show inline differences"));
    ActionUtil::createToolTip(a);

    a = NewGlobalAction( Cmd_LineNumbers, GLOBAL_FOCUS_TARGET, tr("&Line Numbers") );
    a->setShortcut( tr("CTRL+L") );
    a->setMenuRole( QAction::NoRole);
    a->setIcon( QIcon(":/line_numbers.png") );
    a->setToolTip(tr("Show line numbers"));
    ActionUtil::createToolTip(a);

    a = NewGlobalAction( Cmd_PrevDiff, GLOBAL_FOCUS_TARGET, tr("&Previous Diff") );
    a->setMenuRole( QAction::NoRole);
    if ( Platform::IsWin() )
    {
        QList<QKeySequence> winShortcuts;
        winShortcuts << tr("CTRL+1") << tr("F7");
        a->setShortcuts( winShortcuts  );
    }
    else
        a->setShortcut( tr("CTRL+1") );
    a->setIcon( QIcon(":/arrow_teal_left.png") );
    a->setMenuRole( QAction::NoRole);
    ActionUtil::createToolTip(a);

    a = NewGlobalAction( Cmd_NextDiff, GLOBAL_FOCUS_TARGET, tr("&Next Diff") );
    if ( Platform::IsWin() )
    {
        QList<QKeySequence> winShortcuts;
        winShortcuts << tr("CTRL+2") << tr("F8");
        a->setShortcuts( winShortcuts  );
    }
    else
        a->setShortcut( tr("CTRL+2") );
    a->setMenuRole( QAction::NoRole);
    a->setIcon( QIcon(":/arrow_teal_right.png") );
    ActionUtil::createToolTip(a);

    a = NewGlobalAction( Cmd_PrevConflict, GLOBAL_FOCUS_TARGET, tr("P&revious Conflict") );
    if ( Platform::IsWin() )
    {
        QList<QKeySequence> winShortcuts;
        winShortcuts << tr("CTRL+3") << tr("F9");
        a->setShortcuts( winShortcuts  );
    }
    else
        a->setShortcut( tr("CTRL+3") );
    a->setMenuRole( QAction::NoRole);
    a->setIcon( QIcon(":/arrow_red_left.png") );
    ActionUtil::createToolTip(a);

    a = NewGlobalAction( Cmd_NextConflict, GLOBAL_FOCUS_TARGET, tr("Ne&xt Conflict") );
    if ( Platform::IsWin() )
    {
        QList<QKeySequence> winShortcuts;
        winShortcuts << tr("CTRL+4") << tr("F10");
        a->setShortcuts( winShortcuts  );
    }
    else
        a->setShortcut( tr("CTRL+4") );
    a->setMenuRole( QAction::NoRole);
    a->setIcon( QIcon(":/arrow_red_right.png") );
    ActionUtil::createToolTip(a);

    a = NewGlobalAction( Cmd_Find, GLOBAL_FOCUS_TARGET, tr("&Find...") );
    a->setShortcut( tr("CTRL+F") );
    a->setMenuRole( QAction::NoRole);
    a->setIcon( QIcon(":/find_icon.png") );
    ActionUtil::createToolTip(a);

    a = NewGlobalAction( Cmd_GotoDialog, GLOBAL_FOCUS_TARGET, tr("&Go To Line Number...") );
    a->setShortcut( tr("CTRL+G") );
    a->setMenuRole( QAction::NoRole);
    a->setIcon( QIcon(":/goto_icon.png") );
    a->setToolTip(tr("Go to line number"));
    ActionUtil::createToolTip(a);

    a = NewGlobalAction( Cmd_RefreshDoc, GLOBAL_FOCUS_TARGET, tr("&Refresh All") );
    a->setShortcut( Platform::IsMac() ? tr("CTRL+ALT+R") : tr("F5") );
    a->setMenuRole( QAction::NoRole);
    a->setIcon( QIcon(":/refresh_icon.png") );
    ActionUtil::createToolTip(a);

}


void
P4mApplication::initMenuBar( QMenuBar * mb )
{
    QMenu * menu;
    Targetable * a;
    CmdAction * ta = NULL;

        // On the Mac only, we don't want icons on the menu bar.
        // And, Cmd_LineNumbers and Cmd_ShowInlineDiffs are a special case.
        // In both cases, we don't want the icon to show in the menu bar.
        // So we make a copy of it that has no icon and use that for the menu bar.
        //
        // clone does not copy the shortcuts so we have
        // to set them again
        //
#define ADD_TO_MENU(x) a = ActionStore::Global().find( x ); \
if ( a ) \
{ \
    if ( ( (ta = dynamic_cast<CmdAction*>(a)) && Platform::IsMac() ) || ( x == Cmd_LineNumbers ) || \
         ( x == Cmd_ShowInlineDiffs ) ) \
    { \
        CmdAction *na = ta->clone(); \
        na->setIcon( QIcon() ); \
        na->setShortcuts( ta->shortcuts() ); \
        na->validate(); \
        menu->addAction( na ); \
    } \
    else \
        a->addToWidget( menu ); \
}

    // Character encodings menu
    //
    menu = new QMenu( mb );

    ADD_TO_MENU( Cmd_Encoding );

    QMenu * charEncodingsMenu = menu;
    charEncodingsMenu->setTitle( tr("Character &Encoding") );
    // Line endings menu
    //
    menu = new QMenu( mb );
    ADD_TO_MENU( Cmd_LineEndings );

    QMenu * lineEndingsMenu = menu;
    lineEndingsMenu->setTitle( tr("&Line Endings") );

    // Diff Option menu
    //
    menu = new QMenu( mb );
    ADD_TO_MENU( Cmd_DiffOption );

    QMenu * diffOptionMenu = menu;
    diffOptionMenu->setTitle( tr("Comparison &Method") );

    // Show/Hide menu
    //
    menu = new QMenu( mb );
    ADD_TO_MENU( Cmd_ShowBase );
    ADD_TO_MENU( Cmd_ToggleToolBar );
    ADD_TO_MENU( Cmd_LineNumbers );
    ADD_TO_MENU( Cmd_ShowInlineDiffs );

    QMenu * showHideMenu = menu;
    menu->setTitle( tr("Show/H&ide") );

    //
    // File Menu
    //
    menu = new QMenu( mb );
    menu->setTitle( tr("&File") );
    ADD_TO_MENU( Cmd_Open );
    menu->addSeparator();
    ADD_TO_MENU( Cmd_Close );
    ADD_TO_MENU( Cmd_Save );
    ADD_TO_MENU( Cmd_SaveAs );
    menu->addSeparator();
    menu->addMenu( charEncodingsMenu );
    menu->addMenu( lineEndingsMenu );
    menu->addSeparator();
    menu->addMenu( diffOptionMenu );
    menu->addSeparator();
    ADD_TO_MENU( Cmd_Quit );
    mb->addMenu( menu );

    //
    // Edit Menu
    //
    menu = new QMenu( mb );
    menu->setTitle( tr("&Edit") );
    ADD_TO_MENU( Cmd_Undo );
    ADD_TO_MENU( Cmd_Redo );
    menu->addSeparator();
    ADD_TO_MENU( Cmd_Cut );
    ADD_TO_MENU( Cmd_Copy );
    ADD_TO_MENU( Cmd_Paste );
    menu->addSeparator();
    ADD_TO_MENU( Cmd_SelectAll );
    menu->addSeparator();
    CmdActionGroup *ag = (CmdActionGroup*)ActionStore::Global().find(Cmd_EditDiff);
    ag->addToWidget( menu );
    menu->addSeparator();
    ADD_TO_MENU( Cmd_Prefs );
    mb->addMenu( menu );

    //
    // View Menu
    //
    menu = new QMenu( mb );
    menu->setTitle( tr("&View") );
    ADD_TO_MENU( Cmd_SideBySide );
    ADD_TO_MENU( Cmd_Combined );
    menu->addSeparator();
    ADD_TO_MENU( Cmd_Font );
    ADD_TO_MENU( Cmd_TabWidth );
    menu->addSeparator();
    ADD_TO_MENU( Cmd_ZoomIn );
    ADD_TO_MENU( Cmd_ZoomOut );
    ADD_TO_MENU( Cmd_FitToWindow );
    ADD_TO_MENU( Cmd_ActualSize );
    menu->addSeparator();
    ADD_TO_MENU( Cmd_Link );
    ADD_TO_MENU( Cmd_Highlight );
    menu->addSeparator();
    menu->addMenu( showHideMenu );
    menu->addSeparator();
    ADD_TO_MENU( Cmd_RefreshDoc );

    mb->addMenu( menu );

    //
    // Find Menu
    //
    menu = new QMenu( mb );
    menu->setTitle( tr("&Search") );
    ADD_TO_MENU( Cmd_PrevDiff );
    ADD_TO_MENU( Cmd_NextDiff );
    menu->addSeparator();
    ADD_TO_MENU( Cmd_PrevConflict );
    ADD_TO_MENU( Cmd_NextConflict );
    menu->addSeparator();
    ADD_TO_MENU( Cmd_Find );
    ADD_TO_MENU( Cmd_CopyToFind );
    ADD_TO_MENU( Cmd_GotoDialog );

    mb->addMenu( menu );

    //
    // Help Menu
    //
    menu = new QMenu( mb );
    menu->setTitle( tr("&Help") );
#ifdef TEST_FAST_PLAIN_TEST
    ADD_TO_MENU( Cmd_TestTextEdit );
#endif
    ADD_TO_MENU( Cmd_Help );
    menu->addSeparator();
    ADD_TO_MENU( Cmd_About );
    mb->addMenu( menu );

#undef ADD_TO_MENU
}


bool
P4mApplication::allWindowsClosed()
{
    // This checks to see if any widgets are left.

    foreach( QWidget* widget, topLevelWidgets() )
    {
        if ( widget->testAttribute( Qt::WA_QuitOnClose ) && !widget->isHidden() )
            return false;
    }
    // If there are no visible widgets, we can quit
    //
    return true;
}

void
P4mApplication::showAbout(const QVariant &)
{
    if (!mAboutBox)
    {
        mAboutBox = new AboutBox(qApp->activeWindow());
        mAboutBox->setObjectName( "AboutBox" );

        if ( Platform::IsMac() )
        {
            mAboutBox->setWindowFlags( WinFlags::CloseOnlyFlags() );
            mAboutBox->setWindowModality( Qt::NonModal );
        }
        else
        {
            mAboutBox->setWindowFlags ( Qt::Dialog | Qt::WindowTitleHint |
            Qt::WindowCloseButtonHint );
            mAboutBox->setWindowModality( Qt::ApplicationModal );
        }
        mAboutBox->setAttribute( Qt::WA_DeleteOnClose );
    }

    mAboutBox->show();
    mAboutBox->setMaximumSize( mAboutBox->size() );

    mAboutBox->activateWindow();
    mAboutBox->raise();

}

void
P4mApplication::showPrefs(const QVariant &)
{
    Qt::WindowFlags f =     WinFlags::ApplicationModalFlags() |
            Qt::WindowCloseButtonHint;
    P4mPreferencesDialog* prefsDialog = new P4mPreferencesDialog(
            qApp->activeWindow(), f);
    prefsDialog->setWindowModality(Qt::ApplicationModal);
    prefsDialog->setAttribute(Qt::WA_DeleteOnClose, true);
    prefsDialog->exec();
}

void
P4mApplication::showHelp(const QVariant &)
{
   helpWindow()->showHelp(qApp->activeWindow());
}

void
P4mApplication::openDocument(const QVariant &)
{
    showCompareDialog();
}


void
P4mApplication::close(const QVariant &)
{
    if (qApp->activeWindow())
        qApp->activeWindow()->close();
}


void
P4mApplication::quit(const QVariant &)
{
    closeAllWindows();

    if ( allWindowsClosed() )
        QApplication::quit();
}


void
P4mApplication::testLightWeightText(const QVariant &)
{
#ifdef TEST_FAST_PLAIN_TEST
    LightweightTextView3::RunTest();
#endif
}

CharSetConfirmationDialog::CharSetConfirmationDialog( int p4charSet, int alternateCharSet, bool isDiff, QWidget * parent )
    : QDialog( parent, WinFlags::ApplicationModalFlags() )
{
    setModal( true );
    setWindowTitle(Platform::ApplicationName());

    newCharSet = p4charSet;
    origCharSet = p4charSet;

    CharSetMap::CharSetInfo info = CharSetMap::Lookup( p4charSet );
    CharSetMap::CharSetInfo infoNew = CharSetMap::Lookup( alternateCharSet );

    QLabel * mainlabel = new QLabel( tr("%2 has encountered a problem "
        "%1 the files selected.").arg( isDiff ? tr("diffing") : tr("merging")).arg(Platform::ApplicationName()));

    QLabel * line1 = new QLabel( tr("One or more of the the files is either:"));
    QLabel * line2 = new QLabel( tr("of a type not supported by %1").arg(Platform::ApplicationName()));
    QLabel * line3 = new QLabel( tr("a text file that may not display correctly using the encoding"));
    QLabel * enc   = new QLabel( tr("%1").arg( info.displayName ));

    QLabel * questionLabel = new QLabel( isDiff
                                ? tr("Do you want to proceed with a text diff of these files?\n")
                                : tr("Do you want to proceed with a text merge of these files?\n") );

    QCheckBox * cb = new QCheckBox( tr("&Use character encoding:"), this );
    cb->setChecked( false );

    combo = new CharsetCombo(this, P4MERGE);
    combo->setCharset(infoNew.p4Name);
    combo->setDisabled( true );

    QPushButton * noButton = new QPushButton( tr("&No"), this );
    QPushButton * yesButton = new QPushButton( tr("&Yes"), this );
    noButton->setDefault( true );

    QDialogButtonBox* bbox = new QDialogButtonBox(this);
    bbox->addButton(noButton, QDialogButtonBox::RejectRole);
    bbox->addButton(yesButton, QDialogButtonBox::AcceptRole);
    ObjectNameUtil::ApplyStandardButtonNames(bbox);

    QVBoxLayout * l = new QVBoxLayout( this );
    l->setSizeConstraint( QLayout::SetFixedSize );

    l->addWidget( mainlabel );
    l->addItem( new QSpacerItem( 0,5 ) );

    QGridLayout * gl = new QGridLayout();
    gl->setSpacing(3);

    gl->addItem( new QSpacerItem( 10,0 ), 0,0 );
    gl->addWidget( line1, 0,1, 1,2 );
    gl->addWidget( new QLabel("*"), 1,1 );
    gl->addWidget( line2, 1,2 );
    gl->addWidget( new QLabel("*"), 2,1 );
    gl->addWidget( line3, 2,2 );
    gl->addWidget( enc, 3,2 );

    QHBoxLayout * hl = new QHBoxLayout();
    hl->addWidget(cb);
    hl->addWidget(combo);

    gl->addLayout( hl, 4,2 );
    l->addLayout( gl );

    l->addItem( new QSpacerItem( 0,8 ) );
    l->addWidget(questionLabel);
    l->addWidget( bbox );

    connect( combo, SIGNAL(activated(int)), this, SLOT(charSetChanged()) );
    connect( cb, SIGNAL(stateChanged(int)), this, SLOT(continueWithOrigCharSet(int)) );
    connect( yesButton, SIGNAL(clicked()), this, SLOT( accept()) );
    connect( noButton, SIGNAL(clicked()), this, SLOT(reject()) );
}

void
CharSetConfirmationDialog::charSetChanged()
{
    newCharSet = CharSetMap::Lookup( combo->currentCharset() ).p4id;
}


void
CharSetConfirmationDialog::continueWithOrigCharSet( int state )
{
    if ( state == Qt::Checked )
    {
        combo->setDisabled( false );
        newCharSet = CharSetMap::Lookup( combo->currentCharset() ).p4id;
    }else
    {
        combo->setDisabled( true );
        newCharSet = CharSetMap::Lookup( origCharSet ).p4id;
    }

}


int
CharSetConfirmationDialog::getNewCharSet( ) const
{
    return newCharSet;
}
