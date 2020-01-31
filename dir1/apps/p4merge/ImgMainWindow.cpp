#include "ImgMainWindow.h"

#include "stdhdrs.h"
#include "error.h"
#include "diff.h"
#include "strbuf.h"
#include "filesys.h"
#include "i18napi.h"
#include "charcvt.h"

#include "ActionStore.h"
#include "CmdAction.h"
#include "CommandIds.h"
#include "DialogUtil.h"
#include "HelpLauncher.h"
#include "ImgDiffEngine.h"
#include "MergeMainWindow.h"
#include "P4mApplication.h"
#include "P4mCommandGlobals.h"
#include "p4mcorewidget.h"
#include "p4mdocumentbuilder.h"
#include "P4mGlobals.h"
#include "Platform.h"
#include "Prefs.h"
#include "Rule.h"
#include "SettingKeys.h"

#include <QComboBox>
#include <QCloseEvent>
#include <QDesktopWidget>
#include <QDockWidget>
#include <QFileInfo>
#include <QFileDialog>
#include <QFontDialog>
#include <QInputDialog>
#include <QIODevice>
#include <QLabel>
#include <QMessageBox>
#include <QMoveEvent>
#include <QMenuBar>
#include <QStatusBar>
#include <QTextCodec>
#include <QTextStream>
#include <QToolBar>
#include <QToolButton>
#include <QVariant>
#include <QToolTip>
#include <QTimer>
#include <QVBoxLayout>
#include <QDebug>

ImgMainWindow::ImgMainWindow( QWidget * parent )
        : QMainWindow( parent ), ResponderMainWindow( this )
{

if ( !Platform::IsMac() && ! Platform::IsWin() )
{
    setWindowIcon(QPixmap(":/p4mergeimage.png"));
}

    P4mPrefs::SetDefaultValue(p4v::prefs::diff::ImgCombinedDiff, false);

    // Load ImgDiffController.
    mDiffController = new ImgDiffController( this );

    setCentralWidget( mDiffController->centralWidget() );

    QMenuBar * mb = Platform::GlobalMenuBar( false );
    if ( !mb )
        P4mApplication::app()->initMenuBar( menuBar() );

    resizeToFitDesktop();
}

ImgMainWindow::~ImgMainWindow()
{
    delete mDiffController;
}



void
ImgMainWindow::customizeAction( const StringID & cmd, QAction * a )
{
    ResponderMainWindow::customizeAction( cmd, a );

    if ( cmd == Cmd_SideBySide )
    {
        QAction * ia = mDiffController->action(ImgDiffController::SideBySide);
        a->setEnabled ( ia->isEnabled() );
        a->setChecked ( ia->isChecked() );
        a->setText    ( ia->text() );
    }
    else if ( cmd == Cmd_Combined  )
    {
        QAction * ia = mDiffController->action(ImgDiffController::Overlay);
        a->setEnabled ( ia->isEnabled() );
        a->setChecked ( ia->isChecked() );
        a->setText    ( ia->text() );
    }
    else if ( cmd == Cmd_Link )
    {
        QAction * ia = mDiffController->action(ImgDiffController::Link);
        a->setEnabled ( ia->isEnabled() );
        a->setChecked ( ia->isChecked() );
        a->setText    ( ia->text() );
    }
    else if ( cmd == Cmd_FitToWindow )
    {
        QAction * ia = mDiffController->action(ImgDiffController::FitToWindow);
        a->setEnabled ( ia->isEnabled() );
        a->setChecked ( ia->isChecked() );
        a->setText    ( ia->text() );
    }
    else if ( cmd == Cmd_ActualSize )
    {
        QAction * ia = mDiffController->action(ImgDiffController::ActualSize);
        a->setEnabled   ( ia->isEnabled() );
        a->setChecked   ( ia->isChecked() );
        if ( !Platform::IsMac() )
            a->setIcon      ( ia->icon() );
        a->setText      ( ia->text() );
    }
    else if ( cmd == Cmd_Help )
    {
        a->setEnabled ( true );
    }
    else if ( cmd == Cmd_ZoomIn )
    {
        QAction * ia = mDiffController->action(ImgDiffController::ZoomIn);
        a->setEnabled   ( ia->isEnabled() );
        a->setShortcuts  ( ia->shortcuts() );
        if ( !Platform::IsMac() )
            a->setIcon      ( ia->icon() );
        a->setText      ( ia->text() );
    }
    else if ( cmd == Cmd_ZoomOut )
    {
        QAction * ia = mDiffController->action(ImgDiffController::ZoomOut);
        a->setEnabled   ( ia->isEnabled() );
        a->setShortcuts  ( ia->shortcuts() );
        if ( !Platform::IsMac() )
            a->setIcon      ( ia->icon() );
        a->setText      ( ia->text() );
    }
    else if ( cmd == Cmd_Highlight )
    {
        QAction * ia = mDiffController->action(ImgDiffController::Highlight);
        a->setEnabled ( ia->isEnabled() );
        a->setChecked ( ia->isChecked() );
        a->setText    ( ia->text() );
    }
    else if ( cmd == Cmd_RefreshDoc )
    {
        // Enable this once we have a refreshDoc implementation
        a->setEnabled ( false );
    }

}


void
ImgMainWindow::setSideBySideViewShown(const QVariant &)
{
        QAction * ia = mDiffController->action(ImgDiffController::SideBySide);
        ia->toggle();
}


void
ImgMainWindow::setUnifiedViewShown(const QVariant &)
{
        QAction * ia = mDiffController->action(ImgDiffController::Overlay);
        ia->toggle();
}


void
ImgMainWindow::linkImages(const QVariant &)
{
        QAction * ia = mDiffController->action(ImgDiffController::Link);
        ia->toggle();
}


void
ImgMainWindow::fitToWindow(const QVariant &)
{
        QAction * ia = mDiffController->action(ImgDiffController::FitToWindow);
        ia->toggle();
}


void
ImgMainWindow::actualSize(const QVariant &)
{
        QAction * ia = mDiffController->action(ImgDiffController::ActualSize);
        ia->trigger();
}


void
ImgMainWindow::zoomIn(const QVariant &)
{
        QAction * ia = mDiffController->action(ImgDiffController::ZoomIn);
        ia->trigger();
}


void
ImgMainWindow::zoomOut(const QVariant &)
{
        QAction * ia = mDiffController->action(ImgDiffController::ZoomOut);
        ia->trigger();
}


void
ImgMainWindow::setHighlightDiffs(const QVariant &)
{
        QAction * ia = mDiffController->action(ImgDiffController::Highlight);
        ia->toggle();
}


void
ImgMainWindow::showHelp(const QVariant &)
{

    setObjectName("SETTINGS_DIFF_IMAGES");

    P4mApplication::app()->helpWindow()->showHelp(this);

}


void
ImgMainWindow::refreshDoc( const QVariant & )
{

}

void
ImgMainWindow::closeEvent( QCloseEvent * e )
{

        // ImgMainWindow

        Prefs::SetValue(Prefs::FQ(p4v::prefs::diff::ImgWindowState), saveState());
        Prefs::SetValue(Prefs::FQ(p4v::prefs::diff::ImgWindowGeometry), saveGeometry());

        QAction * ia = mDiffController->action(ImgDiffController::Overlay);

        Prefs::SetValue(Prefs::FQ(p4v::prefs::diff::ImgCombinedDiff),  ia->isChecked() );

        // Specifics

        QMainWindow::closeEvent( e );
}


void
ImgMainWindow::close( const QVariant & )
{
    QMainWindow::close();
}


void
ImgMainWindow::updateCaption()
{

    QString caption;

                            //If p4merge has been passed labels for
                            //the file names, use these instead of the actual
                            //temp file names being passed in.
                            //This will eliminate having file#1.txt as the
                            //caption

    if ( Platform::IsMac() )
    {
        caption = tr( "Perforce %1" ).arg(Platform::ApplicationName());
    }
    else
    {
        const QString l = MergeMainWindow::PathToName ( !names[P4m::LEFT].isEmpty()
                                                   ? names[P4m::LEFT]
                                                   : files[P4m::LEFT] );

        const QString r = MergeMainWindow::PathToName ( !names[P4m::RIGHT].isEmpty()
                                                   ? names[P4m::RIGHT]
                                                   : files[P4m::RIGHT] );

        caption = tr ( "%1 and %2  - Perforce %3",
                       "Image window title. %1 is file on left, %2 is file on right" )
                  .arg( l, r, Platform::ApplicationName() );
    }

    setWindowTitle( caption );
}


QString
ImgMainWindow::loadFiles( const QString & leftPath, const QString & rightPath )
{

    files[P4m::LEFT] = leftPath;
    files[P4m::RIGHT] = rightPath;

    QFile lf(leftPath);
    QFile rf(rightPath);
    qint64 ts = lf.size() + rf.size();

    if ( ts > IMG_MAX_SIZE )
    {
        bool dontShow = DontAsk::ReadDontAskPref(DontAsk::FileSizeDialog::kPrefKey, false).toBool();
        if (dontShow)
            goto doit;

        DontAsk::FileSizeDialog dlg(MergeMainWindow::FileSizeWarningStr(), this);
        dlg.exec();

        if (dlg.request() == DontAsk::FileSizeDialog::Request_DontShowFile)
            return "FilesTooBig";
    }

doit:

    if ( !mDiffController->load( leftPath, rightPath ) )
        return mDiffController->error();

    bool combined = Prefs::Value(Prefs::FQ(p4v::prefs::diff::ImgCombinedDiff), false);
    if ( combined )
    {
        QAction * ia = mDiffController->action(ImgDiffController::Overlay);
        ia->setChecked( true );
    } else
    {
        QAction * ia = mDiffController->action(ImgDiffController::SideBySide);
        ia->setChecked( true );
    }

    if (restoreState(Prefs::Value(Prefs::FQ(p4v::prefs::diff::ImgWindowState), QByteArray())))
    {
        mDiffController->dockWidget()->show();
    }
    else
    {
        addDockWidget( Qt::LeftDockWidgetArea,
                mDiffController->dockWidget() );
    }

    if (!restoreGeometry(Prefs::Value(Prefs::FQ(p4v::prefs::diff::ImgWindowGeometry), QByteArray())))
    {
        QRect r = QDesktopWidget().availableGeometry( this );
        resize( 400, 800 );
        move( r.topLeft() );
    }

    updateCaption();

    return QString();

}


void
ImgMainWindow::overrideOptions( P4mArgs args )
{
    // Override initial setting with command line options here

        if ( mDiffController && !args.names[P4m::LEFT].isNull() )
        {
                names[P4m::LEFT] = args.names[P4m::LEFT];
                mDiffController->diffEngine()->setName( ImgDiffEngine::Image1 , names[P4m::LEFT] );

        }

        if ( mDiffController && !args.names[P4m::RIGHT].isNull() )
        {
                names[P4m::RIGHT] = args.names[P4m::RIGHT];
                mDiffController->diffEngine()->setName( ImgDiffEngine::Image2, names[P4m::RIGHT] );
        }

    updateCaption();

}

/** resizeToFitDesktop
    \brief  Resize the window to fit within the bounds of the indicated screen.
*/
void
ImgMainWindow::resizeToFitDesktop()
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

