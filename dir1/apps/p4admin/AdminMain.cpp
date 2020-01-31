/*
 *  main entry for standalone P4Admin application
 */
#include "global_object_manager.h"
#include "MiniQApp.h"
#include "OpConfiguration.h"
#include "P4AdminApplication.h"
#include "Platform.h"

int main( int argc, char ** argv )
{
    GlobalObjectManager gom;

    Platform::SetApplicationName("P4Admin");
    Platform::SetApplication(Platform::P4Admin_App);

    // workaround for QTBUG-58069.. menus enabled at startup
    if (Platform::RunningUnityDesktop())
    {
        QApplication::setAttribute(Qt::AA_DontUseNativeMenuBar);
    }

            // fix Mac OS X 10.9 (mavericks) font issue
        // https://bugreports.qt-project.org/browse/QTBUG-32789

#ifdef Q_OS_OSX
    if ( QSysInfo::MacintoshVersion > QSysInfo::MV_10_8 )
        QFont::insertSubstitution(".Lucida Grande UI", "Lucida Grande");
#endif

    int i = 0;
    // see if a custom style has been passed in
    // and honor it if it has
    bool hasCustomStyleSet = false;
    while ( i < argc )
    {
        if (QByteArray(argv[i]) == "-V")
        {
            MiniQApp app(argc, argv);
            app.showVersionInfo();
            return 1;
        }
        else if (QByteArray(argv[i]) == "-h")
        {
            MiniQApp app(argc, argv);
            app.showUsageInfo();
            return 1;
        }
        else if (QByteArray(argv[i]) == "-style")
        {
            hasCustomStyleSet = true;
        }
        else if (argv[i][0] == '-')
        {
            // make sure it's an acceptable argument for Op::Configuration (pucC)
            if (strchr("pucC", argv[i][1]) == NULL)
            {
                MiniQApp app(argc, argv);
                app.showParamError(i);
                return -1;
            }
        }
        ++i;
    }

    P4AdminApplication app(argc, argv);

    if (app.isRunning())
    {
        Op::Configuration config = Op::Configuration::CreateFromArgumentList(app.arguments());
        if (!config.isEmpty())
            app.sendMessage( config.adminString() );
        return 0;
    }
/*
    if (!hasCustomStyleSet)
        app.setStyle(CreatePerforceStyleFromBuiltIn( app.style() ));
*/
#ifdef Q_OS_OSX
    // We need to shield p4admin from the Restore state feature in Lion.
    if ( QSysInfo::MacintoshVersion > QSysInfo::MV_10_6 )
        system("defaults write com.perforce.p4admin NSQuitAlwaysKeepsWindows -bool false");        
#endif
    
    // Launch Admin with the command-line parameters
    if ( !app.run() )
        return 1;   // no config selected

    return app.exec();
}
