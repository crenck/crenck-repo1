//
// P4mAppleEventHandler
// --------------
//
// P4mAppleEventHandler is responsible for handling AppleEvents to P4Merge.
// It is only active for the Mac builds. It converts AppleEvents into
// P4Merge events that the application can understand.
//
#include "P4mAppleEventHandler.h"

#include "MergeMainWindow.h"
#include "P4mApplication.h"
#include "P4mArgs.h"
#include "PMessageBox.h"

#include "P4mAppleEventGlobals.h"

#include <QDir>

// This is the C call that we register to handle the event. It simply calls
// the P4mAppleEventHandler object to handle the event.
//
OSErr HandleDiffMethod(
    const AppleEvent    *theAppleEvent,
    AppleEvent      *reply,
    SRefCon          myRefCon );

OSErr HandleOpenAppMethod(
    const AppleEvent    *theAppleEvent,
    AppleEvent      *reply,
    SRefCon          myRefCon );


bool
QuitToPort( const QString & port )
{
    // Make a port to talk to the shim
    //
    CFStringRef nameRef = CFStringCreateWithCharacters( NULL, port.utf16(), port.length() );
    CFMessagePortRef portRef = CFMessagePortCreateRemote( NULL, nameRef );
    CFRelease( nameRef );

    if ( !portRef )
        return false;

    // Send a close window message

    CFMessagePortSendRequest( portRef, QuitMessage, NULL, 1000, 0, NULL, NULL );
    CFRelease( portRef );
    return true;
}


void
PrintfToPort( const QString & port, FILE * file, const QString & chars )
{
    CFStringRef nameRef = CFStringCreateWithCharacters( NULL, port.utf16(), port.length() );
    CFMessagePortRef portRef = CFMessagePortCreateRemote( NULL, nameRef );
    CFRelease( nameRef );

    if ( portRef )
    {
        QByteArray temp = chars.toUtf8();
        CFDataRef data = CFDataCreateWithBytesNoCopy( NULL, (const UInt8*)temp.data(), temp.length(), kCFAllocatorNull );

        int messageType = StdErrMessage;

        if ( file == stdout )
            messageType = StdOutMessage;

        CFMessagePortSendRequest( portRef, messageType, data, 1000, 0, NULL, NULL );
        CFRelease( portRef );
        CFRelease( data );
    }
}


P4mAppleEventHandler::P4mAppleEventHandler( P4mApplication & app )
  : mApp( app ),
    upp( NULL ),
    uppOpenApp( NULL )
{
    attach( app );
}


P4mAppleEventHandler::~P4mAppleEventHandler()
{
    detach();
}


void
P4mAppleEventHandler::attach( P4mApplication & app )
{
    Q_UNUSED( app );
    // Register our callback
    //
    upp = NewAEEventHandlerUPP( HandleDiffMethod );

    AEInstallEventHandler(
    kPerforceAESuite,
    kPerforceMergeMethodEvent,
    upp,
    (SRefCon)this,
    false );

    uppOpenApp = NewAEEventHandlerUPP( HandleOpenAppMethod );

    AEInstallEventHandler(
    kCoreEventClass,
    kAEOpenApplication,
    uppOpenApp,
    (SRefCon)this,
    false );
}


void
P4mAppleEventHandler::detach()
{
    // De-register our callback.
    //
    if (!upp)
        return;

    AERemoveEventHandler(
    kPerforceAESuite,
    kPerforceMergeMethodEvent,
    upp,
    false );
    DisposeAEEventHandlerUPP( upp );
    upp = 0;

    AERemoveEventHandler(
    kCoreEventClass,
    kAEOpenApplication,
    uppOpenApp,
    false );
    DisposeAEEventHandlerUPP( uppOpenApp );
    uppOpenApp = 0;

}


OSErr HandleDiffMethod(
    const AppleEvent    *theAppleEvent,
    AppleEvent      *reply,
    SRefCon         rc )
{
    P4mAppleEventHandler * t = (P4mAppleEventHandler *)rc;

    return t->handleDiffEvent( theAppleEvent, reply );
}

OSErr HandleOpenAppMethod(
    const AppleEvent    *theAppleEvent,
    AppleEvent      *reply,
    SRefCon          rc )
{
    P4mAppleEventHandler * t = (P4mAppleEventHandler *)rc;

    return t->handleOpenAppEvent( theAppleEvent, reply );
}

OSErr P4mAppleEventHandler::handleOpenAppEvent(
    const AppleEvent *theAppleEvent,
    AppleEvent *reply )
{
    Q_UNUSED( theAppleEvent );
    Q_UNUSED( reply );
    if ( mApp.arguments().size() == 1 )
        mApp.showCompareDialog();
    return noErr;
}


OSErr P4mAppleEventHandler::handleDiffEvent(
    const AppleEvent    *theAppleEvent,
    AppleEvent      *reply )
{
    Q_UNUSED( reply );
    AEDescList  docList;
    long        numArgs, i;
    Size    dataSize;
    AEKeyword   keyword;
    DescType    returnedType;
    Size        actualSize;
    OSErr       err = noErr;

    QStringList arguments;
    char ** argv;
    char * port = NULL;
    char * cwdStr = NULL;

    mApp.resetZeroArguments();
    //
    // Get the list of args from the Apple Event.
    // This is the argc/argv that was passed to the command-line.
    // We are going to create our own argc/argv from them.
    //

    if ((err = AEGetParamDesc(
                        theAppleEvent,
                        keyDirectObject,
                        typeAEList,
                        &docList)) != noErr)
        return err = AEDisposeDesc(&docList);


    //
    // How many arguments are there?
    //
    if ((err = AECountItems(
                        &docList,
                        &numArgs)) != noErr)
        return err = AEDisposeDesc(&docList);

    argv = (char **)malloc( sizeof(char *) * numArgs );

    for (int i = 0; i < numArgs; i++)
        argv[i] = 0;

    //
    // Iterate through them
    //
    for ( i = 0; i < numArgs; i++ )
    {
    AESizeOfNthItem(
                &docList,
                i + 1,
                &returnedType,
                &dataSize);

    argv[i] = (char *)malloc( dataSize + 1 );

    //
    // Get the i"th" item as a string
    //
    if ((err = AEGetNthPtr(
                            &docList,
                            i + 1,
                            typeChar,
                            &keyword,
                            &returnedType,
                            (Ptr)argv[i],
                            dataSize,
                            &actualSize)) != noErr)
        return err = AEDisposeDesc(&docList);

    //
    // Null the end of the string
    //
    (argv[i])[dataSize] = 0;
    arguments.append( QString::fromUtf8(argv[i]) );
    }

    // Get the port to notify when the window closes
    //
    AESizeOfParam(
    theAppleEvent,
    portKeyword,
    &returnedType,
    &dataSize);

    port = (char *)malloc( dataSize + 1 );

    AEGetParamPtr(
    theAppleEvent,
    portKeyword,
    typeChar,
    &keyword,
    port,
    dataSize,
    &actualSize );

    port[actualSize] = 0;

    err = AESizeOfParam(
    theAppleEvent,
    cwdKeyword,
    &returnedType,
    &dataSize);

    if ( err == noErr )
    {
    cwdStr = (char *)malloc( dataSize + 1 );

    AEGetParamPtr(
        theAppleEvent,
        cwdKeyword,
        typeChar,
        &keyword,
        cwdStr,
        dataSize,
        &actualSize );

    cwdStr[actualSize] = 0;
    }
    else
    {
    cwdStr = (char *)malloc(1);
    cwdStr[0] = 0;
    }
    //
    // Make an args object from the args
    //

    P4mArgs args;
    args.parseArgs( arguments );

    QString portqstring = QString::fromUtf8(port);

    if ( args.helpRequested )
    {
        PrintfToPort( portqstring, stdout, args.helpMessage() );
        QuitToPort( portqstring );
        return noErr;
    }

    if ( args.versionRequested )
    {
        PrintfToPort( portqstring, stdout, args.versionMessage() );
        QuitToPort( portqstring );
        return noErr;
    }

    QString qcwd = QString::fromUtf8(cwdStr);

    for(int i= 0; i<4; i++)
    {
        // if it is a relative path, fix it.
        if ( !args.files[i].isNull() &&  !args.files[i].startsWith('/') )
        {
            QString messyPath = qcwd + "/" + args.files[i];
            QString cleanPath = QDir::cleanPath( messyPath );
            args.files[i] = cleanPath;
        }
    }

    // Make sure all the files that were passed in (if any)
    // are actually files, and that we have the right number.
    //
    args.checkFiles();

    if ( args.hasErrorMessage()  )
    {
        PrintfToPort( portqstring, stderr, "Incorrect parameters:\n" );
        PrintfToPort( portqstring, stderr, args.errorMessage() );
        PrintfToPort( portqstring, stderr, "\n" );
        PrintfToPort( portqstring, stderr, "Use 'launchp4merge -h' for more help.\n\n" );
        QuitToPort( portqstring );
        return noErr;
    }

//#define SHOW_ARGS
#ifdef SHOW_ARGS
    {
    QString str;
    str.append( QString("Port: %1\nCWD: %2\n").arg(QString::fromUtf8(port)).arg(QString::fromUtf8(cwdStr) ));

    for ( int i = 0; i < numArgs; i++ )
    {
        str.append( QString("Arg %1: %2\n").arg(i).arg(QString::fromUtf8(argv[i]) ));
    }

    str.append( "\nFILES --------\n" );
    for ( int i = 0; i < 4; i++ )
    {
        if ( args.files[i].size() )
            str.append( QString("%1\n").arg(args.files[i]) );
    }

    PMessageBox::information( NULL, QString(), str );
    }
#endif

//#define SHOW_ARGS1
#ifdef SHOW_ARGS1
    QString str;
    for ( int i = 0; i < arguments.size() ; i++ )
    {
        str.append( QString("Arg %1: %2\n").arg(i).arg(arguments.at(i) ));
    }
    PMessageBox::information( NULL, QString(), str );
#endif

    if (args.numFiles())
    {
        //PMessageBox::information( NULL, QString(), "P4mAppleEventHandler:Before calling main window" );
        QMainWindow * mw = mApp.createMainWindow( args.files[P4m::LEFT], args.files[P4m::RIGHT], args.files[P4m::BASE], &args );

        if ( mw )
        {
            // IMPORTANT: Here, we map the window to the port id so that when the
            // window is closed, we can look up the port and notify it.
            map[(QObject *)mw] = portqstring;
            connect( mw, SIGNAL(destroyed( QObject * )), this, SLOT(releaseShimForSender( QObject * )) );

            mw->show();
            mw->activateWindow();
            mw->raise();

           // If the app has the compare files dialog open, we hide it so it
            // doesn't distract.
            //
            mApp.hideCompareFilesDialog();
        }
        else
            return false;
    }
    else
    {
        mApp.showCompareDialog();
        return err;
    }


    // Cleanup
    //
    for ( i=0; i < numArgs; i++ )
    {
    if (argv[i]) free(argv[i]);
    }

    free(argv);
    free(port);

    return err;

}

void
P4mAppleEventHandler::releaseShimForSender( QObject * )
{
    // Which window sent this to us?
    //
    const QObject * w = sender();

    // Which window sent this to us?
    //
    if ( !map.contains( w ) )
        return;

    // Look up the window for the port and notify it.
    //
    QString s = map[w];

    QuitToPort( s );

    // Now that we have done our work, remove the entry.
    //
    map.remove( w );
}
