#include "MergeMainWindow.h"

#include "stdhdrs.h"
#include "error.h"
#include "diff.h"
#include "strbuf.h"
#include "filesys.h"
#include "i18napi.h"
#include "charcvt.h"

#include "CharSetMap.h"
#include "CmdAction.h"
#include "CmdActionGroup.h"
#include "CommandIds.h"
#include "DialogUtil.h"
#include "DisplayLabel.h"
#include "NakedToolButton.h"
#include "ObjectNameUtil.h"
#include "P4mApplication.h"
#include "P4mCommandGlobals.h"
#include "p4mcorewidget.h"
#include "p4mdocumentbuilder.h"
#include "P4mGlobals.h"
#include "p4mmultimergewidget.h"
#include "Platform.h"
#include "Prefs.h"
#include "Rule.h"
#include "SettingKeys.h"
#include "WinFlags.h"

#include <QCloseEvent>
#include <QDesktopWidget>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QFileDialog>
#include <QFontDialog>
#include <QIODevice>
#include <QLabel>
#include <QMenuBar>
#include <QProgressDialog>
#include <QPushButton>
#include <QStatusBar>
#include <QTextCodec>
#include <QTextStream>
#include <QToolBar>
#include <QToolButton>
#include <QTimer>
#include <QVBoxLayout>

QString
MergeMainWindow::totalDiffStr()
{
    return MergeMainWindow::tr( "%1 diffs (%2)" );
}

QString
MergeMainWindow::fileFormatStr()
{
    return MergeMainWindow::tr( "File format ( Encoding: %1  Line endings: %2 )" );
}

QString
MergeMainWindow::diffFileFormatStr()
{
    return MergeMainWindow::tr( "Encoding: %1" );
}

QString
MergeMainWindow::tabSpacingStr()
{
    return MergeMainWindow::tr( "Tab spacing: %1" );
}

QString
MergeMainWindow::FileSizeWarningStr()
{
    return MergeMainWindow::tr( "The size of the files selected could cause "
                           "%1 to become temporarily unresponsive."
                           "\n\nDo you want to continue?" ).arg(Platform::ApplicationName());
}

QString
MergeMainWindow::messageFilename()
{
    QString filename;
    if ( cwb->isDiff() && cwb->editDiffIndex() != P4m::UNKNOWN )
    {
        filename = "\"" +  PathToName ( files[cwb->editDiffIndex()] ) + "\"";
    }
    else
        filename = tr( "the merge file" );
    return filename;
}

MergeMainWindow::MergeMainWindow( QWidget * parent )
        : QMainWindow( parent ), ResponderMainWindow( this ), mDocument( NULL ),
        mRequestedDocument(NULL), mDocumentCreationSuccessful( false ),
        mProgressDialog(NULL)
{
    P4mPrefs::SetDefaultValue(p4merge::prefs::ShowLineNumbers, false);
    P4mPrefs::SetDefaultValue(p4merge::prefs::ShowBase, true);
    P4mPrefs::SetDefaultValue(p4merge::prefs::DiffCombinedDiff, false);
        
    mLabels[P4m::LEFT] = tr("Left:");
    mLabels[P4m::RIGHT] = tr("Right:");
    mLabels[P4m::MERGE] = tr("Not saved.");

    saveFile = NULL;
    cwb = NULL;
    mMacToolBarAreaVisible = true;
    mEncoding           = P4mPrefs::ToCharset( Prefs::Value( Prefs::FQ( MergePrefs::CHARSET ), QString() ) );
    mTabWidth           = Prefs::Value( Prefs::FQ( MergePrefs::TABWIDTH ), 0 );
    mTabInsertsSpaces   = Prefs::Value( Prefs::FQ( MergePrefs::TABINSERTSSPACES ), false );
    mShowTabsSpaces     = Prefs::Value( Prefs::FQ( MergePrefs::SHOWTABSSPACES ), false );
    mDiffOption         = mRequestedDiffOption = P4mPrefs::ToDiffFlags( Prefs::Value( Prefs::FQ( MergePrefs::DIFFOPTION ), QString() ) );
    mGridOption         = 0;
    mLineEnding         = P4mPrefs::ToLineEnding( Prefs::Value( Prefs::FQ( MergePrefs::LINEENDING ), QString() ) );

    if ( !Platform::IsMac() && ! Platform::IsWin() )
    {
        setWindowIcon(QPixmap(":/p4mergeimage.png"));
    }

    mainWidget = new QFrame( this );
    setCentralWidget( mainWidget );

    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainWidget->setLayout( mainLayout );
    mainLayout->setMargin( 0 );

    tb = new QToolBar( this );
    tb->setWindowTitle( tr("P4MergeToolbar") );
    tb->setObjectName( tr("MainWindowToolbar") );

    mainLayout->setContentsMargins( 0, 0, 0, 0 );
    mainLayout->setSpacing( 0 );

    macStatusBar = new QStatusBar();
    mainLayout->addWidget( macStatusBar );
    macStatusBar->setSizeGripEnabled( false );

    diffStatsLbl  = new QLabel( getStatusBar() );
    tabSpacingLbl = new QLabel( getStatusBar() );
    fileFormatLbl    = new QLabel( getStatusBar() );

    // On the Mac we want to make the status line text
    // two point size smaller then the current size
    //
    if ( Platform::IsMac() )
    {
        QFontInfo fontinfo = diffStatsLbl->fontInfo();
        QFont labelfont = QFont(fontinfo.family(), fontinfo.pointSize(),
            fontinfo.weight());
        labelfont.setPointSize(labelfont.pointSize() - 2 );

        diffStatsLbl->setFont ( labelfont );
        fileFormatLbl->setFont( labelfont );
        tabSpacingLbl->setFont( labelfont );
        
        tb->setIconSize(QSize(18,18));
    }

    getStatusBar()->setContentsMargins( 4, 0, 4, 0 );
    getStatusBar()->addPermanentWidget( diffStatsLbl, 0 );
    getStatusBar()->addPermanentWidget(Rule::CreateVRule(this) );
    getStatusBar()->addPermanentWidget( tabSpacingLbl, 0 );
    getStatusBar()->addPermanentWidget(Rule::CreateVRule(this) );
    getStatusBar()->addPermanentWidget( fileFormatLbl, 1 );

    mainLayout->addWidget( Rule::CreateHRule(this) );

    QMenuBar * mb = Platform::GlobalMenuBar( false );
    if ( !mb )
        P4mApplication::app()->initMenuBar( menuBar() );

    loadToolbar( tb );
    tb->setMovable( !Platform::IsMac() );
    addToolBar( tb );

    // we keep this only because it will tell us if the toolbar is visible or not
    mToolbarAction = tb->toggleViewAction();

    connect( mToolbarAction, SIGNAL(triggered()), this, SLOT(toolBarToggled()) );

    updateStatusBar();

    resizeToFitDesktop();
}

MergeMainWindow::~MergeMainWindow()
{
    delete saveFile;
}

void
MergeMainWindow::customizeAction( const StringID & cmd, QAction * a )
{
    ResponderMainWindow::customizeAction( cmd, a );

    if ( !cwb || !cwb->document() )
    {
        a->setEnabled(false);
        return;
    }

    if ( cmd == Cmd_ToggleToolBar )
    {
        a->setChecked( toolbarVisible() );
    }

    P4m::Player player = (P4m::Player)a->data().toInt();

    if (!cwb)
    {
        a->setEnabled( false );
        return;
    }

    else if ( cmd == Cmd_LineNumbers )
    {
        a->setEnabled( !cwb->isDiff() || (cwb->isDiff() && !cwb->isUnifiedView()) );
        a->setChecked( lineNumbersVisible() );
    }
    else if ( cmd == Cmd_ShowInlineDiffs )
    {
        a->setChecked( cwb->inlineDiffsEnabled());
    }
    else if ( cmd == Cmd_Save  )
    {
        a->setEnabled( needsSaving() );
    }
    else if ( cmd == Cmd_SaveAs )
    {
        a->setEnabled( cwb->isEditingDiff() || !cwb->isDiff() );
    }
    else if ( cmd == Cmd_Font )
    {
        a->setEnabled( true );
    }
    else if ( cmd == Cmd_TabWidth )
    {
        a->setEnabled( true );
    }
    else if ( cmd == Cmd_Help )
    {
        a->setEnabled( true );
    }
    else if ( cmd == Cmd_LineEndings )
    {
        a->setEnabled( !cwb->isDiff() );
        a->setChecked( mLineEnding == a->data().toInt() );
    }
    else if ( cmd == Cmd_DiffOption )
    {
        a->setChecked( mDiffOption == a->data().toInt() );
        if ( cwb->isDiff() )
            if ( !(QFileInfo(files[P4m::LEFT]).exists()) || !(QFileInfo (files[P4m::RIGHT]).exists()) )
                a->setEnabled( false );
    }
    else if ( cmd == Cmd_Encoding )
    {
        a->setChecked( mEncoding == a->data().toInt() );
        if ( cwb->isDiff() )
            if ( !(QFileInfo(files[P4m::LEFT]).exists()) || !(QFileInfo (files[P4m::RIGHT]).exists()) )
                a->setEnabled( false );
    }
    else if ( cmd == Cmd_EditDiff )
    {
        a->setEnabled( cwb->isDiff() && !cwb->isUnifiedView() &&
                       QFileInfo( files[player] ).isWritable() && QFileInfo( files[player] ).exists() ) ;
        a->setChecked( cwb->editDiffIndex() == player );
    }
    else if ( cmd == Cmd_RefreshDoc )
    {
        if ( cwb->isDiff() )
            if ( !(QFileInfo(files[P4m::LEFT]).exists()) || !(QFileInfo (files[P4m::RIGHT]).exists()) )
                a->setEnabled( false );
    }
}

void
MergeMainWindow::updateCaption()
{
    if ( !cwb )
        return;

    QString caption;
    QString appName = tr( "Perforce %1" ).arg(Platform::ApplicationName());

                            //If p4merge has been passed labels for
                            //the file names, use these instead of the actual
                            //temp file names being passed in.
                            //This will eliminate having file#1.txt as the
                            //caption
    if ( cwb->isDiff() )
    {
        const QString l = PathToName ( !names[P4m::LEFT].isEmpty()
                                       ? names[P4m::LEFT]
                                       : files[P4m::LEFT] );
        const QString r = PathToName ( !names[P4m::RIGHT].isEmpty()
                                       ? names[P4m::RIGHT]
                                       : files[P4m::RIGHT] );

        if ( Platform::IsMac() )
            caption = tr( "%1 and %2",
                          "Diff window title on Mac OS X. %1=left filename, %2 = right filename." )
                      .arg ( l, r );
        else
            caption = tr( "%1 and %2 - %3",
                          "Diff window title on Windows and Linux. %1=left filename, %2 = right filename."
                          " %3=application name \"Perforce %3\"" )
                      .arg ( l, r, appName );
    }

    else if ( saveFile )
    {
        QString filename;

        if ( names[P4m::MERGE].isEmpty() )
        {
            // It's merge and there is a saved file name.
            // call it the save file name.
            //
            filename = saveFile->fileName();
            cwb->setLabelText( P4m::MERGE, saveFile->fileName() );
        }
        else
        {
            // Its a merge and there is a saved file name
            // but it's being called from P4V, so use the
            // target/theirs file name for the window title
            filename = PathToName( files[P4m::RIGHT] );
        }

        if ( Platform::IsMac() )
            caption = filename;
        else
            caption += tr( "%1 - %2",
                           "Save window title on Windows and Linux. %1=filename."
                           " %2=application name \"Perforce %3\"" )
                       .arg ( filename, appName, appName );
    }
    else
    {
        // There is no saved results file name to display
        // in the window title yet
        caption = appName;
    }

  if ( mGridOption == P4mPrefs::Guarded )
      caption += " - GUARDED";

    setWindowTitle( caption );
}

bool
MergeMainWindow::chooseSaveFile()
{
    QString s;
    if ( !saveFile )
    {
        // If we have no saved file location,
        // default to the location of the yours file
        QFileInfo fi (files[2]);
        s = fi.absolutePath();
    }
        // If we running from P4V, names[P4m::LEFT] will not be empty
        // so we hide the Temp directory that P4V uses behind
        // the scenes and default the save dir to the users
        // Home directory instead
        //
    else if ( names[P4m::LEFT].isEmpty() )
        s = saveFile->absolutePath();
    else
        s = QDir::homePath();

    s = QFileDialog::getSaveFileName( this, tr("Save File As"), s, QString());

    if ( !s.isNull() )
        setSaveFile( s );
    return !s.isNull();
}

void
MergeMainWindow::updateStatusBar()
{
    if ( !cwb )
        return;

    diffStatsLbl->setText( totalDiffStr().arg( cwb->numDiffs() ).arg( P4mPrefs::DiffOptionDescription(mDiffOption, P4mPrefs::StatusBarText) ) );

    if ( cwb->isDiff() )
        fileFormatLbl->setText( diffFileFormatStr().arg( P4mPrefs::CharSetDescription(mEncoding, P4mPrefs::StatusBarText) ) );
    else
        fileFormatLbl->setText( fileFormatStr().arg( P4mPrefs::CharSetDescription(mEncoding, P4mPrefs::StatusBarText) )
                           .arg( P4mPrefs::LineEndingDescription(mLineEnding, P4mPrefs::StatusBarText) ) );

    QString tabText = tabSpacingStr();
    if ( mTabInsertsSpaces )
        tabText = tr( "%1 (Insert spaces)",
                      "%1=\"Tab spacing: %1\"" )
                  .arg( tabText );
    tabSpacingLbl->setText( tabText.arg( mTabWidth ) );

}


void
MergeMainWindow::toggleToolbar( const QVariant & )
{
    setToolbarVisible( !toolbarVisible() );
}


bool
MergeMainWindow::needsSaving()
{
    if ( !cwb || !cwb->document() )
        return false;

    if ( cwb->isDiff() && ( !cwb->isEditingDiff() ) )
        return false;

    if ( !saveFile )
        return true;

    return cwb->document()->isDirty();
}

bool
MergeMainWindow::requestSave( bool * wasSaved )
{
    if ( wasSaved != NULL )
        *wasSaved = false;
    if ( !cwb || !cwb->document() )
        return true;

    if ( !needsSaving() )
        return true;

    PMessageBox mb( QMessageBox::Question,
                    Platform::ApplicationName(),
                    tr("Do you want to save your changes to %1?").arg( messageFilename() ) ,
                    QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                    this );
    mb.setWindowModality( Qt::WindowModal );
    mb.setWindowFlags( WinFlags::SheetFlags() );

    mb.button( QMessageBox::Save )->setText( tr("&Save") );
    mb.button( QMessageBox::Discard )->setText( tr("&Don't Save") );

    switch ( mb.exec() )
    {
        case QMessageBox::Save:
            if ( save(QVariant()) )
            {
                if ( wasSaved )
                    *wasSaved = true;
                return true;
            }
            break;
        case QMessageBox::Discard:
            return true;
            break;
        case QMessageBox::Cancel:
        default: // just for sanity
            return false;
            break;
    }
    return false;
}

void
MergeMainWindow::setSaveFile( QString s )
{
    if ( !s.isEmpty() )
    {
        if (!saveFile) saveFile = new QFileInfo();

        files[P4m::MERGE] = s;
        saveFile->setFile( s );
    }
    updateCaption();
}

bool
MergeMainWindow::saveDocument( const QString & path )
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly))
        return false;

    QTextStream ts( &f );

    // By default, QTextCodec::codecForLocale() is used for reading and writing QTextStreams
    if ( mEncoding != P4mPrefs::System )
    {
        QString charset = P4mPrefs::ToP4CharSet( mEncoding );
        CharSetMap::CharSetInfo csi = CharSetMap::Lookup( charset );

        //QTextStream does not write a BOM by default, but you can enable this by calling setGenerateByteOrderMark(true).
        if ( csi.hasBom )
            ts.setGenerateByteOrderMark( true );

        QTextCodec * tc = QTextCodec::codecForName( csi.qtName.toLatin1() );

        if ( !tc )
            tc = QTextCodec::codecForLocale();

        ts.setCodec( tc );
    }

    cwb->document()->save( ts, mLineEnding );
    cwb->document()->clearDirty();
    cwb->document()->dontAbsorbNextCommand();
    f.close();
    return true;
}

bool
MergeMainWindow::checkDocument( int inP4CharSet, int *outP4CharSet )
{

    P4m::ReadErrors tooLong = P4m::NoError;
    bool isdiff = false;

    if ( files[0].isNull() )
        isdiff = true;

    *outP4CharSet = inP4CharSet;
    int alternateCharSet = inP4CharSet;

    tooLong = P4mDocumentBuilder::CanProcessFiles( files[0], files[1], files[2], inP4CharSet );

#ifdef USE_LIGHTWEIGHT_WIDGET
        if ( tooLong == P4m::FileWarnBig )
        {
            //For the lw textedit widget:
            //if combined size is less then 10MB = no dialog
            //if combined size greater then 10MB = warn user application may
            //not respond (make it a preference eventually)

            bool dontShow = DontAsk::ReadDontAskPref(DontAsk::FileSizeDialog::kPrefKey, false).toBool();
            if (dontShow)
                goto doit;

            DontAsk::FileSizeDialog dlg(FileSizeWarningStr(), this);
            dlg.exec();

            if (dlg.request() == DontAsk::FileSizeDialog::Request_DontShowFile)
                return false;
        }
#endif

doit:
    if ( tooLong == P4m::CharSetIsWrong  || tooLong == P4m::SystemCharSetOk )
    {
        if ( tooLong == P4m::SystemCharSetOk )
            alternateCharSet = CharSetApi::NOCONV;
        // Display dialog with appropriate default selection
        //
        CharSetConfirmationDialog * dlg = new CharSetConfirmationDialog( inP4CharSet, alternateCharSet, isdiff );
        if ( dlg->exec() == QDialog::Rejected )
        {
            delete dlg;
            return false;
        }

        *outP4CharSet = dlg->getNewCharSet();
        delete dlg;
    }

    return true;
}


bool
MergeMainWindow::makeDocumentThreaded()
{

    if ( !mProgressDialog )
    {
        mProgressDialog = new QProgressDialog( this );
        mProgressDialog->setMinimumDuration( 0 );
        mProgressDialog->setLabelText(tr("Loading files..."));
        mProgressDialog->setRange(0,0);

        mProgressDialog->setWindowModality(isVisible() ? Qt::WindowModal
                                                       : Qt::ApplicationModal);
        if ( !Platform::IsMac() )
        {
            mProgressDialog->setWindowTitle(Platform::ApplicationName());
            if (isVisible())
            {
                mProgressDialog->setWindowModality(Qt::WindowModal);
                mProgressDialog->setWindowFlags(WinFlags::SheetFlags());
            }
            else
            {
                mProgressDialog->setWindowFlags(WinFlags::CloseOnlyFlags());
            }
        }
        DialogUtil::HideSizeGrip(*mProgressDialog, QSize(250, 100));
    }
    QTimer::singleShot(5000, this, SLOT(showProgressDialog()));

    DiffFlags f;

    f.sequence = (DiffFlags::Sequence)prefDiffFlagsToP4DiffFlags( mRequestedDiffOption );
    f.grid = (DiffFlags::Grid)prefGridFlagsToP4GridFlags( mGridOption );

    P4mDocumentBuilder * builder = NULL;

    if ( files[P4m::BASE].isEmpty() )
        builder = new P4mDocumentBuilder( files[P4m::LEFT], files[P4m::RIGHT],
            P4mPrefs::ToP4CharSet( mEncoding ),
            P4mDocumentBuilder::LineEndingLocal, f );
    else
        builder = new P4mDocumentBuilder( files[P4m::BASE], files[P4m::LEFT], files[P4m::RIGHT],
            P4mPrefs::ToP4CharSet( mEncoding ),
            P4mDocumentBuilder::LineEndingLocal, f );

    connect( mProgressDialog,   SIGNAL(canceled()),  builder, SLOT(stop()) );

    builder->setFlags( f );
    mRequestedDocument = new P4mDocument( builder->documentType() );

    // Make sure commands are re-enabled when the operation completes
    CmdAction::ValidateAllOnSignal( mRequestedDocument, SIGNAL(finishedLoading()) );
    CmdAction::ValidateAllOnSignal( mRequestedDocument, SIGNAL(canceled()) );

    connect( mRequestedDocument, SIGNAL(finishedLoading()), this, SLOT(commitRequestedDocument()) );
    connect( mRequestedDocument, SIGNAL(canceled()), this, SLOT(canceled()) );
    mRequestedDocument->load( builder );

    if ( mProgressDialog )
    {
        mProgressDialog->close();
        delete mProgressDialog;
        mProgressDialog = NULL; // required so timer callback doesnt redisplay dialog
    }
    delete builder;

    return mDocumentCreationSuccessful;
}

void
MergeMainWindow::showProgressDialog()
{
    if ( mProgressDialog )
        mProgressDialog->show();
}
void
MergeMainWindow::canceled()
{
    mDocumentCreationSuccessful = false;
}

bool
MergeMainWindow::save(const QVariant &)
{

    // Check for any unresolved conflicts and alert user
    //
    DiffIterator i( *cwb->document() );
    while ( i.hasNext() )
    {
        P4mNodeGroup * ng = i.next();

        if ( !ng->isResolved() )
        {
            QString dialogMsg = tr("One or more conflicts have not been resolved.\n\n");
            dialogMsg = dialogMsg + tr("Do you still want to save your changes to %1?").arg( messageFilename() );

            PMessageBox mb( QMessageBox::Question,
                            Platform::ApplicationName(),
                            dialogMsg ,
                            QMessageBox::Save | QMessageBox::Cancel,
                            this );
            mb.setWindowModality( Qt::WindowModal );
            mb.setWindowFlags( WinFlags::SheetFlags() );

            mb.button( QMessageBox::Save )->setText( tr("&Save") );

            switch ( mb.exec() )
            {
                case QMessageBox::Save:
                    break;
                case QMessageBox::Cancel:
                default: // just for sanity
                    return false;
                    break;
            }
            break;
        }
    }

    if (!saveFile && !chooseSaveFile())
        return false;

    bool result = saveDocument( saveFile->absoluteFilePath() );

    if ( !result )
        PMessageBox::critical(
            this,
            tr("'%1' not saved").arg(windowTitle()),
            tr("Could not save file!"));

    if ( result && cwb && cwb->isEditingDiff() )
    {
        reloadDocument();
        cwb->document()->clearDirty();
        cwb->document()->clearUndoHistory();
    }

    return result;
}


bool
MergeMainWindow::saveAs(const QVariant &)
{
    if (!chooseSaveFile())
        return false;

    bool result = saveDocument( saveFile->absoluteFilePath() );

    if ( !result )
        PMessageBox::critical(
            this,
            tr("'%1' not saved").arg(windowTitle()),
            tr("Could not save file!"));
    return result;
}

void
MergeMainWindow::chooseFont(const QVariant &)
{
    bool ok;

    QFont f = QFontDialog::getFont( &ok, cwb->font(), this, tr( "Select Font" ), QFontDialog::DontUseNativeDialog );

    if ( !ok )
        return;

    f.setUnderline( false );
    f.setStrikeOut( false );
    cwb->setFont( f );
}

void
MergeMainWindow::chooseTabWidth(const QVariant &)
{
    TabOptionsDialog * dlg = new TabOptionsDialog(this, WinFlags::SheetFlags() | Qt::MSWindowsFixedSizeDialogHint );
    dlg->setTabWidth(mTabWidth);
    dlg->setTabInsertsSpaces(mTabInsertsSpaces);
    dlg->setShowTabsSpaces(mShowTabsSpaces);

    if (dlg->exec() == QDialog::Accepted)
    {
        mTabWidth = dlg->tabWidth();
        cwb->setTabWidth( mTabWidth );
        mTabInsertsSpaces = dlg->tabInsertsSpaces();
        cwb->setTabInsertsSpaces ( mTabInsertsSpaces );
        mShowTabsSpaces = dlg->showTabsSpaces();
        cwb->setShowTabsSpaces ( mShowTabsSpaces );
    }
    delete dlg;
    updateStatusBar();

}

void
MergeMainWindow::setDiffOption( const QVariant & v )
{
    SaveNotificationDialog * dlg = NULL;

    int option = v.toInt();

    if ( option == mDiffOption )
        return;

    if ( !cwb )
        return;

    if ( !cwb->document()->canUndo() )
        goto doit;

    dlg = new SaveNotificationDialog( tr("change comparison method"), messageFilename() , this );
    if ( dlg->exec() == QMessageBox::No )
    {
        delete dlg;
        return;
    }
    delete dlg;

doit:
    mRequestedDiffOption = option;

    reloadDocument();
    if ( cwb )
        cwb->document()->makeDirty();
}

void
MergeMainWindow::setCustomGridOption( int option )
{
    Q_UNUSED(option);
/*  This is commented out until we officially suppor the -dg guarded grid
//  option in the gui

    if ( option == mPrefs.gridOption() )
        return;

    if ( !cwb || !cwb->document()->canUndo() )
        goto doit;

    if ( MessageBox::warning(
            this,
            tr("'%1' not saved").arg(windowTitle()),
            tr("Changing the diff option will remove "
              "all edits from the document and start "
              "from scratch.\nDo you want to remove "
              "all edits and change the diff option?"),
            QMessageBox::Yes,
            QMessageBox::No ) == QMessageBox::No )
    {
        return;
    }

doit:
    // We are now setting our own grid option, not receiving it from the global prefs.
    //
    */
//  disconnect( &P4mApplication::app()->prefs(),
//              SIGNAL(gridOptionChanged(int)),
//              &mPrefs, SLOT(setGridOption(int)) );
//  mPrefs.setGridOption( option );
}

// void
// MergeMainWindow::prefsGridOptionChanged( int /*le*/ )
// {
//  /* This code is commented out until we officially support the -dg guarded grid option
//  if ( !cwb )
//      return;
//
//  P4mDocument * d = makeDocument();
//
//  if (!d)
//  {
//      PMessageBox::warning( this, tr("Warning"),
//              tr("Error opening one of the files."),
//              QMessageBox::Ok, QMessageBox::NoButton );
//      this->close( NULL );
//      return;
//  }
//
//  // Start in an automerge state.
//  if (!d->isDiffDocument())
//  {
//      d->autoMerge();
//      d->clearUndoHistory();
//  }
//
//  setDocument( d ); */
//
//  updateStatusBar();
// }
//

void
MergeMainWindow::setDocument( P4mDocument * d )
{
    if ( cwb && cwb->document() != d )
            cwb->setDocument( d );

    if ( d == mDocument )
        return;


    mDocument = d;

    // Make sure we do extra validating when the document runs commands.
    // because the auto-validation doesn't always catch these two.

    CmdAction * a = mToolbarStore.findAction( Cmd_Undo );
    connect( d, SIGNAL( undoStateChanged(bool) ),
             a, SLOT  ( validate() ) );
    a = mToolbarStore.findAction( Cmd_Redo );
    connect( d, SIGNAL( redoStateChanged(bool) ),
             a, SLOT  ( validate() ) );
    a = mToolbarStore.findAction( Cmd_Save );
    connect( d, SIGNAL( dirtyChanged(bool) ),
             a, SLOT  ( validate() ) );
}


P4mDocument *
MergeMainWindow::document() const
{
    return mDocument;
}

void
MergeMainWindow::commitRequestedDocument()
{
Q_ASSERT( mRequestedDocument );

    bool isdiff = mRequestedDocument->isDiffDocument();
    // If this is the first time running
    bool firsttime = !cwb->document();

    if ( !isdiff )
    {
        // Start in an automerge state.
        mRequestedDocument->autoMerge();
        if ( firsttime )
        {
            mRequestedDocument->makeDirty();
            mRequestedDocument->clearUndoHistory();
        }
    }
    else if ( cwb->isEditingDiff() )
    {
        // At this point we have edited and saved a new
        // left or right file. So we want to clear the
        // undo history and re-select same index so
        // we can re-display the new results.
        // After we re-select we clear the dirty flag
        // so if the user just selects the other file
        // without making edits they don't get a prompt
        // asking if they want to save
        //
        mRequestedDocument->clearUndoHistory();
        mRequestedDocument->autoSelect( cwb->editDiffIndex() );
        mRequestedDocument->clearDirty();
    }

    mDiffOption = mRequestedDiffOption;

    setDocument( mRequestedDocument );

    cwb->mergePane()->goToNextDiff( QVariant() );

    mRequestedDocument->clearUndoHistory();

    updateStatusBar();

    mDocumentCreationSuccessful = true;

    mRequestedDocument = NULL;
}

void
MergeMainWindow::setEncoding( P4mPrefs::Charset cs )
{
    SaveNotificationDialog * dlg = NULL;

    if ( cs == mEncoding )
        return;

        mEncoding = cs;

    if ( !cwb )
        return;

    if ( !cwb->document()->canUndo() )
        goto doit;

    dlg = new SaveNotificationDialog( tr("change character encoding"), messageFilename() , this );
    if ( dlg->exec() == QMessageBox::No )
    {
        delete dlg;
        return;
    }
    delete dlg;

doit:
    reloadDocument();
    if ( cwb )
        cwb->document()->makeDirty();
}


void
MergeMainWindow::loadToolbar( QToolBar * tb )
{
    CmdAction * a = NULL;

#define ADD_BUTTON(x) a = ActionStore::Global().findAction( x ); \
if ( a ) \
{ \
    a = a->cloneWithParent( this );\
    CmdTargetDesc desc = a->targetDescription();\
    desc.focusHolder = this;\
    a->setTargetDescription( desc );\
    tb->addAction( a ); \
    mToolbarStore.addTargetable( a ); \
}
    ADD_BUTTON( Cmd_RefreshDoc);
    ADD_BUTTON( Cmd_Save );
    tb->addSeparator();
    CmdActionGroup *ag = (CmdActionGroup*)ActionStore::Global().find(Cmd_EditDiff);
    ag->addToWidget( tb );
    tb->addSeparator();
    ADD_BUTTON( Cmd_Undo );
    ADD_BUTTON( Cmd_Redo );
    tb->addSeparator();
    ADD_BUTTON( Cmd_PrevDiff );
    ADD_BUTTON( Cmd_NextDiff );
    ADD_BUTTON( Cmd_PrevConflict );
    ADD_BUTTON( Cmd_NextConflict );
    tb->addSeparator();
    ADD_BUTTON( Cmd_Find );
    ADD_BUTTON( Cmd_GotoDialog );
    tb->addSeparator();
    ADD_BUTTON( Cmd_LineNumbers );
    ADD_BUTTON( Cmd_ShowInlineDiffs );

    tb->addSeparator();

    QToolButton * button = new NakedToolButton();
    tb->addWidget( button );
    button->setIcon( QIcon( ":/comparison_method.png") );
    button->setToolTip( tr("Comparison method") );
    QMenu * popup = new QMenu( button );

    {
        Targetable * t = ActionStore::Global().find( Cmd_DiffOption );

        if (t)
            t->addToWidget( popup );
    }

    button->setMenu( popup );
    button->setPopupMode( QToolButton::InstantPopup );
    button->setAutoRaise(true);

#undef ADD_BUTTON
}

bool
MergeMainWindow::toolbarVisible() const
{
    if ( Platform::IsMac() )
        return mMacToolBarAreaVisible;

    return mToolbarAction->isChecked();
}

void
MergeMainWindow::setToolbarVisible( bool b )
{
    if ( Platform::IsMac() )
    {
        if ( mMacToolBarAreaVisible == b )
            return;

        // This toggles the mac toolbar area

        QToolBarChangeEvent te( true );
        QApplication::sendEvent( this, &te );

        return;
    }

    if ( b != toolbarVisible() )
        mToolbarAction->trigger();
}

void
MergeMainWindow::closeEvent( QCloseEvent * e )
{
    if ( requestSave() && cwb )
    {
        // MergeMainWindow

        Prefs::SetValue(Prefs::FQ(p4merge::prefs::ShowLineNumbers), cwb->lineNumbersVisible());

        // last window closed wins
        Prefs::SetValue(Prefs::FQ( MergePrefs::INLINEDIFFSENABLED ), cwb->inlineDiffsEnabled() );

        // Specifics

        if ( cwb->isDiff() )
        {
            Prefs::SetValue(Prefs::FQ(p4merge::prefs::DiffCombinedDiff), cwb->isUnifiedView() );
            Prefs::SetValue(Prefs::FQ(p4merge::prefs::DiffWindowState), saveState());
            Prefs::SetValue(Prefs::FQ(p4merge::prefs::DiffWindowGeometry), saveGeometry() );
        }
        else
        {
            Prefs::SetValue(Prefs::FQ(p4merge::prefs::ShowBase), cwb->isBaseVisible() );
            Prefs::SetValue(Prefs::FQ(p4merge::prefs::MergeWindowState), saveState());
            Prefs::SetValue(Prefs::FQ(p4merge::prefs::MergeWindowGeometry), saveGeometry() );
        }

        e->accept();
        QMainWindow::closeEvent( e );
        return;
    }

    e->ignore();
}


QStatusBar *
MergeMainWindow::getStatusBar() const
{
    return macStatusBar;//->statusBarDelegate();
}

void
MergeMainWindow::close( const QVariant & )
{
    QMainWindow::close();
}


void
MergeMainWindow::setEncoding( const QVariant & data )
{
    // get the encoding from the action
    // and set it
    int p4CharSet = P4mPrefs::ToP4CharSetInt( (P4mPrefs::Charset)data.toInt() );
    if ( checkDocument( p4CharSet, &p4CharSet ) )
        setEncoding( P4mPrefs::FromP4CharSet( p4CharSet ) );
}


void
MergeMainWindow::setUnifiedViewShown( const QVariant & )
{
    if ( !cwb )
        return;

    cwb->setUnifiedView( true );
};


void
MergeMainWindow::setEditDiff( const QVariant & data )
{
    P4m::Player player = (P4m::Player)data.toInt();

    bool wasSaved;
    if ( !requestSave( &wasSaved ) )
        return;

    setSaveFile( files[player] );
    //only reload document if file was saved/changed
    if ( wasSaved )
        reloadDocument();
    if ( cwb->editDiffIndex() == player )
        cwb->setEditDiffIndex( P4m::UNKNOWN );
    else
        cwb->setEditDiffIndex( player );
}


bool
MergeMainWindow::lineNumbersVisible()
{
    return cwb->lineNumbersVisible();
}

P4m::ReadErrors
MergeMainWindow::loadFiles( const QString & leftPath, const QString & rightPath, const QString & basePath )
{
    DiffFlags f;
    int p4charset = P4mPrefs::ToP4CharSetInt( mEncoding );

    bool isdiff = false;

    if ( basePath.isNull() )
        isdiff = true;

    if (isdiff)
        setObjectName("SETTINGS_DIFF_FILES");
    else
        setObjectName("SETTINGS_MERGE_FILES");

    files[P4m::BASE] = basePath;
    files[P4m::LEFT] = leftPath;
    files[P4m::RIGHT] = rightPath;

    P4m::ReadErrors e = P4mDocumentBuilder::CanProcessFiles( basePath ,leftPath, rightPath, p4charset );

#ifdef USE_LIGHTWEIGHT_WIDGET
        if ( e == P4m::FileWarnBig )
        {
                //For the lw textedit widget:
                //if combined size is less then 10MB = no dialog
                //if combined size greater then 10MB = warn user application may
                //not respond (make it a preference eventually)

            bool dontShow = DontAsk::ReadDontAskPref(DontAsk::FileSizeDialog::kPrefKey, false).toBool();
            if (dontShow)
                goto doit;

            DontAsk::FileSizeDialog dlg(FileSizeWarningStr(), this);
            dlg.exec();

            if (dlg.request() == DontAsk::FileSizeDialog::Request_DontShowFile)
                    return e;
        }
#endif

doit:
        int alternateCharSet = p4charset;

        if ( e == P4m::CharSetIsWrong  || e == P4m::SystemCharSetOk )
        {
            if ( e == P4m::SystemCharSetOk )
                alternateCharSet = CharSetApi::NOCONV;
            // Display dialog with appropriate default selection
            //
            CharSetConfirmationDialog * dlg = new CharSetConfirmationDialog( p4charset, alternateCharSet, isdiff );
            if ( dlg->exec() == QDialog::Rejected )
            {
                delete dlg;
                return e;
            }

            p4charset = dlg->getNewCharSet();
            delete dlg;
            setEncoding( P4mPrefs::FromP4CharSet( p4charset ) );
        }

    if ( !cwb )
    {

        cwb = new P4mCoreWidget( NULL, isdiff , false, mainWidget );

        mainWidget->layout()->addWidget( cwb );
        setDelegate( cwb );

        cwb->setLabelText( P4m::BASE,  names[P4m::BASE].isNull()   ? basePath    : names[P4m::BASE] );
        cwb->setLabelText( P4m::LEFT,  names[P4m::LEFT].isNull()   ? leftPath   : names[P4m::LEFT] );
        cwb->setLabelText( P4m::RIGHT, names[P4m::RIGHT].isNull()  ? rightPath  : names[P4m::RIGHT] );


        if ( !names[P4m::MERGE].isNull() )
            cwb->setLabelText( P4m::MERGE, names[P4m::MERGE] );
        else if ( !files[P4m::MERGE].isEmpty() )
            cwb->setLabelText( P4m::MERGE, files[P4m::MERGE] );
        else
            cwb->setLabelText( P4m::MERGE, mLabels[P4m::MERGE] );

        if ( !mLabels[P4m::LEFT].isNull() )
            cwb->setTitleText(P4m::LEFT, mLabels[P4m::LEFT] );

        if ( !mLabels[P4m::RIGHT].isNull() )
            cwb->setTitleText(P4m::RIGHT, mLabels[P4m::RIGHT] );

        bool restoredGeometry;
        if ( cwb->isDiff() )
        {
            cwb->setUnifiedView( Prefs::Value(Prefs::FQ(p4merge::prefs::DiffCombinedDiff), false) );
            restoreState(Prefs::Value(Prefs::FQ(p4merge::prefs::DiffWindowState), QByteArray()));
            restoredGeometry = restoreGeometry(Prefs::Value(Prefs::FQ(p4merge::prefs::DiffWindowGeometry), QByteArray()));
        }
        else
        {
            cwb->setBaseVisible( Prefs::Value(Prefs::FQ(p4merge::prefs::ShowBase), true) );
            restoreState(Prefs::Value(Prefs::FQ(p4merge::prefs::MergeWindowState), QByteArray()));
            restoredGeometry = restoreGeometry(Prefs::Value(Prefs::FQ(p4merge::prefs::MergeWindowGeometry), QByteArray()));
        }
        if (!restoredGeometry)
        {
            QRect r = QDesktopWidget().availableGeometry( this );
            resize( 400, 800 );
            move( r.topLeft() );
        }

        cwb->setTabWidth( mTabWidth );
        cwb->setTabInsertsSpaces( mTabInsertsSpaces );
        cwb->setShowTabsSpaces( mShowTabsSpaces );

        setLineNumbersVisible( Prefs::Value(Prefs::FQ(p4merge::prefs::ShowLineNumbers), false) );

        // The window state restores the toolbar state as well.
        // If the toolbar is hidden, we need

        if ( tb->isHidden() )
            toolBarToggled();
    }

    updateCaption();

    updateStatusBar();

    bool documentCreationSucessful = makeDocumentThreaded();

    if ( documentCreationSucessful )
        return P4m::NoError;
    else
        return P4m::CancelLoad;

}


void
MergeMainWindow::overrideOptions( P4mArgs args )
{
    if ( args.tabWidth >= 0 )       setPrefTabWidth( args.tabWidth );
    if ( args.wsOption >= 0 )       setPrefDiffOption( args.wsOption );
    if ( args.gridOption >= 0 )     setPrefGridOption( args.gridOption );
    if ( args.lineEnding >= 0 )     setPrefLineEndings( (P4mPrefs::LineEnding)args.lineEnding );
    if ( !args.charSet.isNull() )   setPrefEncoding( P4mPrefs::FromP4CharSet(args.charSet) );

    if ( !args.leftLabel.isNull() )  setLabel( P4m::LEFT, args.leftLabel );
    if ( !args.rightLabel.isNull() ) setLabel( P4m::RIGHT, args.rightLabel );


    for ( int i = 0; i < 4; i++ )
    {
        if ( !args.names[i].isNull() )
            setName( (P4m::Player)i, args.names[i] );
    }

    if ( !args.files[P4m::MERGE].isNull() )
        setSaveFile( args.files[P4m::MERGE] );
}


void
MergeMainWindow::setName ( P4m::Player index, const QString & name )
{
    if ( cwb )
        cwb->setLabelText(index, names[index] );

    names[index] = name;
}

void
MergeMainWindow::setLabel ( P4m::Player index, const QString & label )
{
    if ( cwb )
        cwb->setTitleText(index, mLabels[index] );

    mLabels[index] = label;
}


QString
MergeMainWindow::PathToName( const QString & path )
{
    // Try to make a name from the path
    //
    QFileInfo fi(path);
    QString fName = fi.fileName();

    if ( fName.isEmpty() )
        return path;

    return fName;
}


void
MergeMainWindow::toggleLineNumbersShown ( const QVariant & )
{
    setLineNumbersVisible( !lineNumbersVisible() );
}

void
MergeMainWindow::setLineNumbersVisible( bool vis )
{
    cwb->setLineNumbersVisible(vis);
}


void
MergeMainWindow::setLineEndings( const QVariant & data )
{
    mLineEnding = (P4mPrefs::LineEnding)data.toInt();
    if ( cwb )
        cwb->document()->makeDirty();
    updateStatusBar();
}

void
MergeMainWindow::setGridOption( const QVariant & data )
{
    // get the grid option from the action
    // and set it
    setCustomGridOption( data.toInt() );
}

void
MergeMainWindow::refreshDoc( const QVariant & )
{
    SaveNotificationDialog * dlg = NULL;

    if ( !cwb || !cwb->document()->canUndo() )
        goto doit;

    dlg = new SaveNotificationDialog( tr("refresh"), messageFilename() , this );
    if ( dlg->exec() == QMessageBox::No )
    {
        delete dlg;
        return;
    }
    delete dlg;

doit:
    reloadDocument();
}


void MergeMainWindow::reloadDocument()
{
    if ( !cwb )
        return;

    makeDocumentThreaded();
}


/** resizeToFitDesktop
    \brief  Resize the window to fit within the bounds of the indicated screen.
*/
void
MergeMainWindow::resizeToFitDesktop()
{
// Ensure the window isn't bigger than its screen.
    QDesktopWidget *desk = qApp->desktop();
    QRect   deskRect = desk->screenGeometry(this);
    QRect   frameRect = frameGeometry();
    int     frameW = frameRect.width() - width();
    int     frameH = frameRect.height() - height();
    if (frameRect.width() > deskRect.width())
        resize(deskRect.width() - frameW, height());
    if (frameRect.height() > deskRect.height())
        resize(width(), deskRect.height() - frameH);
}


void
MergeMainWindow::toolBarToggled()
{

    if ( ! Platform::IsMac() )
        return;

    // we are a mac and the user did NOT use the mac-native toolbar button to
    // hide the toolbar, but rather, chose the Qt-menu (wish they hadn't)
    // This slot tracks that and makes sure the mac-toolbar is hidden as well.

    setToolbarVisible( mToolbarAction->isChecked() );
}


bool
MergeMainWindow::event( QEvent * e )
{
    bool handled;
    if ( e->type() == QEvent::ToolBarChange )
    {
        mMacToolBarAreaVisible = !mMacToolBarAreaVisible;

        // This means that the MacToolbar has been toggled
        // We need to make sure that the Qt toolbar also is hidden
        // so we trigger it to sync them

        if ( mMacToolBarAreaVisible != mToolbarAction->isChecked() )
            mToolbarAction->trigger();
    }

    handled = QMainWindow::event( e );
    return handled;
}

SaveNotificationDialog::SaveNotificationDialog( const QString & action, const QString & filename, QWidget * parent )
    : PMessageBox( parent )
{
    setText( tr( "Are you sure you want to %1?\n"
                 "This will cause your changes to %2 to be lost.").arg( action ).arg( filename ) );

    setIcon( QMessageBox::Warning );
    setWindowTitle( Platform::ApplicationName() );
    setStandardButtons( QMessageBox::Yes | QMessageBox::No );
    setWindowModality( Qt::WindowModal );
    setWindowFlags(WinFlags::SheetFlags());
    // do this last because setWindowModality messes up the tab order
    setDefaultButton( QMessageBox::No );
}


TabOptionsDialog::TabOptionsDialog(  QWidget * parent, Qt::WindowFlags f )
    : QDialog( parent, f )
{
    setWindowTitle( tr("Select Tab and Space Options") );
    setObjectName("TabOptionsDialog");
    setWindowModality( Qt::WindowModal );
    setWindowFlags(f);

    mTabWidth = new QSpinBox( this );
    mTabWidth->setRange(1, 99);
    QLabel * tabWidthLabel = new DisplayLabel( this, tr("&Tab spacing:") + "   " );
    tabWidthLabel->setBuddy( mTabWidth );
    mTabInsertsSpaces = new QCheckBox( tr("&Insert spaces instead of tabs") );
    mShowTabsSpaces = new QCheckBox( tr("&Show tabs and spaces") );

    QVBoxLayout * vbl = new QVBoxLayout(this);
    QHBoxLayout * hbl = new QHBoxLayout();
    vbl->insertLayout( 0, hbl );
    hbl->addWidget(tabWidthLabel);
    hbl->addWidget(mTabWidth);
    vbl->addWidget( mTabInsertsSpaces );
    vbl->addWidget( mShowTabsSpaces );

    QPushButton * okButton = new QPushButton( tr("OK"), this );
    QPushButton * cancelButton = new QPushButton( tr("Cancel"), this );
    okButton->setDefault( true );
    connect( okButton,    SIGNAL(clicked()), this, SLOT(accept()) );
    connect( cancelButton,SIGNAL(clicked()), this, SLOT(reject()) );

    QDialogButtonBox* bbox = new QDialogButtonBox(this);
    bbox->addButton( cancelButton , QDialogButtonBox::RejectRole );
    bbox->addButton( okButton, QDialogButtonBox::AcceptRole );
    ObjectNameUtil::ApplyStandardButtonNames(bbox);

    vbl->addSpacing(8);
    vbl->addWidget( bbox );

}

