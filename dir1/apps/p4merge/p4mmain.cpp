#if defined (Q_OS_OSX) || defined (Q_OS_WIN)
#define GUI_APPLICATION
#endif

#include "P4mApplication.h"
#include "Platform.h"
#include "MacTextEditEventFilter.h"

#include "global_object_manager.h"

#include <QByteArray>


#ifdef Q_OS_OSX
#include "P4mAppleEventHandler.h"
#endif



int p4mMain( int argc, char ** argv )
{
    GlobalObjectManager gom;

    // workaround for QTBUG-58069.. menus enabled at startup
    if (Platform::RunningUnityDesktop())
    {
        QApplication::setAttribute(Qt::AA_DontUseNativeMenuBar);
    }

    // see if a custom style has been passed in
    // and honor it if it has

    bool hasCustomStyleSet = false;
    int i = 0;
    while ( i < argc )
    {
        if ( QByteArray(argv[i++]) == "-style" )
            hasCustomStyleSet = true;
    }

            // fix Mac OS X 10.9 (mavericks) font issue
        // https://bugreports.qt-project.org/browse/QTBUG-32789

#ifdef Q_OS_OSX
    if ( QSysInfo::MacintoshVersion > QSysInfo::MV_10_8 )
        QFont::insertSubstitution(".Lucida Grande UI", "Lucida Grande");
#endif
    P4mApplication a( argc, argv );
    QStringList args;
    for(int i = 0; i < argc; ++i)
        args.append(QString::fromLocal8Bit(argv[i]));

    if ( !a.processArguments(args) )
        exit(EXIT_SUCCESS);

#ifdef Q_OS_OSX
    P4mAppleEventHandler h( a );
#endif

    MacTextEditEventFilter::InstallOnApplication();
/*
    if ( hasCustomStyleSet == false )
        a.setStyle( CreatePerforceStyleFromBuiltIn( a.style() ) );
*/    
#ifdef Q_OS_OSX
    // We need to shield p4merge from the Restore state feature in Lion.
    if ( QSysInfo::MacintoshVersion > QSysInfo::MV_10_6 )
        system("defaults write com.perforce.p4merge NSQuitAlwaysKeepsWindows -bool false");        
#endif


return a.exec();

}
