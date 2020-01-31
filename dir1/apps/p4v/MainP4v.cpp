#include "SandstormApp.h"

#include "MacTextEditEventFilter.h"
#include "Platform.h"
#include "Prefs.h"
#include "SettingKeys.h"

#include "global_object_manager.h"

#include <time.h>

#include <QApplication>
#include <QFont>
#include <QSysInfo>

int mainP4v( int argc, char **argv )
{
    srand (time (NULL));
    GlobalObjectManager gom;

        // fix Mac OS X 10.9 (mavericks) font issue
        // https://bugreports.qt-project.org/browse/QTBUG-32789

    QApplication app(argc, argv);

    // workaround for QTBUG-58069.. menus enabled at startup
    if (Platform::RunningUnityDesktop())
    {
        QApplication::setAttribute(Qt::AA_DontUseNativeMenuBar);
    }

    // this installs our Gui::SystemLocale as the system locale
    //Gui::SystemLocale locale;

    if (!SandstormApp::Create())
        return 1;
    if(SandstormApp::sApp->quitNow())
        return SandstormApp::sApp->quitCode();

    if ( Platform::IsMac() )
    {
        MacTextEditEventFilter::InstallOnApplication();
    }

#ifdef Q_OS_OSX
    // We need to shield P4V from the Restore state feature in Lion.
    if ( QSysInfo::MacintoshVersion > QSysInfo::MV_10_6 )
        system("defaults write com.perforce.p4v NSQuitAlwaysKeepsWindows -bool false");        
#endif

    Gui::Application::dApp->init();
    
    int result = app.exec();

	Prefs::Flush();

    return result;
}
