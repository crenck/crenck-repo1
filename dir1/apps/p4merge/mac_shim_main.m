// The purpose of this little ditty is to send an Apple Event to the
// P4Merge GUI program. The behavior is that it would open a window in the
// GUI program, and then wait to return until the window is closed.
//
// This is achieved by using the notification mechanism on Mac OS X.
//
// When this starts up, it reads the arguments to make sure they are valid, then
// registers to be notified by the GUI process.
// It then sends the argc/argv parameters in an AppleEvent and waits for the
// notification.
//
// Upon receiving it, it exits. It will also periodically check to see if the
// process is even runnning (in case the GUI crashes) otherwise we would just
// sit waiting for a notification that would never arrive.
//


#include <CoreServices/CoreServices.h>
#include <AppKit/AppKit.h>
#include <unistd.h>
#include <sys/param.h>

#include "P4mAppleEventGlobals.h"

# include <stdlib.h>

#include <macutil.h>

#define ZeroAEDesc(x) x.descriptorType = 0; x.dataHandle = 0


static const char * portBase = "com.perforce.p4merge_cmd_line_shim";
static ProcessSerialNumber P4MERGE_PROCESS;

OSErr
CreateDiffEventFromParameters(
    ProcessSerialNumber  psn,
    AppleEvent * diffEvent,
    int     argc,
    const char * argv[] );


//
// This is called whenever we get a message on our MessagePort
//
CFDataRef MyMessagePortCallBack (
    CFMessagePortRef local,
    SInt32 msgid,
    CFDataRef data,
    void * info )
{
    // A window was closed. Stop our loop which
    // will exit our code.

    if ( msgid == QuitMessage )
    {
        CFRunLoopStop( CFRunLoopGetCurrent() );
        return NULL;
    }

    if ( msgid == StdOutMessage )
    {
        fprintf( stdout, "%s", (const char *)CFDataGetBytePtr(data) );
        return NULL;
    }

    if ( msgid == StdErrMessage )
    {
        fprintf( stderr, "%s", (const char *)CFDataGetBytePtr(data) );
        return NULL;
    }

    return NULL;
}




//
// This is called periodically to make sure if p4merge
// exits accidentally (crashes) we aren't hung out to dry.
//
void MyTimerCallBack(
    CFRunLoopTimerRef timer,
    void * info )
{
    pid_t pid;
    if ( GetProcessPID( &P4MERGE_PROCESS, &pid ) == procNotFound )
    {
        // P4Merge is no longer around. Why?
        // We don't know but we bail anyway.
        //
        fprintf( stderr, "P4Merge.app quit\n");
            CFRunLoopStop( CFRunLoopGetCurrent() );
    }
}

bool
P4MergeApplicationFromArg0( const char * arg0, FSRef *appRef, CFStringRef * path )
{
    // We are bundled inside P4Merge.app so we
    // move up until we get to the bundle and
    // launch merge from there.
    //
    CFURLRef    app = 0;

    OSErr err = noErr;
    UInt32 type;
    UInt32 creator;
    Boolean isDir;

    err = FSPathMakeRef( (const UInt8 *)arg0, appRef, &isDir );

    if ( err != noErr )
        goto couldNotFindP4MergeApp;

    // up to 1 - Resources
    //       2 - Contents
    //       3 - P4Merge.app
    //
    int i = 0;
    for ( i = 0; i < 3; i++ )
    {
        err = FSGetCatalogInfo( appRef, kFSCatInfoNone, NULL, NULL, NULL, appRef );

        if ( err != noErr )
            goto couldNotFindP4MergeApp;
    }

    app = CFURLCreateFromFSRef( NULL, appRef );

    if ( !app )
        goto couldNotFindP4MergeApp;

    // We verify the path is an application.
    //
    *path = CFURLCopyFileSystemPath( app, kCFURLPOSIXPathStyle );

    if ( !CFStringHasSuffix( *path, CFSTR("app") ) )
    {
        CFRelease( *path );
        *path = NULL;
        goto couldNotFindP4MergeApp;
    }


    // We verify the application is p4merge.
    //
    CFBundleGetPackageInfoInDirectory( app, &type, &creator );

    if ( creator != mergeSignature )
        goto couldNotFindP4MergeApp;

    {
        bool b = CFURLGetFSRef( app, appRef );
        if ( app )
            CFRelease( app );
        return b;
    }

couldNotFindP4MergeApp:
    if ( app )
        CFRelease( app );
    return false;
};


bool SetupMessagePortAndCallbacks( char **name )
{
    //
    //  CREATE A PORT TO COMMUNICATE WITH P4MERGE.APP
    //

    *name = (char *)malloc( 1024 );

    // Make a name for the port from a prefix and our pid.
    //
    sprintf( *name, "%s.%d", portBase, getpid() );

    CFMessagePortContext context;
    memset( &context, 0, sizeof(context) );

    CFStringRef nameRef = CFStringCreateWithCString(NULL, *name, kCFStringEncodingASCII);

    CFMessagePortRef portRef =
        CFMessagePortCreateLocal(
            NULL,
            nameRef,
            &MyMessagePortCallBack,
            &context,
            NULL );

    if ( !portRef )
    {
        CFRelease( nameRef );
        fprintf( stderr, "MessagePort could not be created." );
        return false;
    }

    CFRelease( nameRef );

    CFRunLoopRef ourRunLoop = CFRunLoopGetCurrent();

    if ( !ourRunLoop )
    {
        CFRelease( nameRef );
        fprintf( stderr, "Run Loop could not be gotten." );
        return false;
    }


    CFRunLoopSourceRef loopRef =
      CFMessagePortCreateRunLoopSource( NULL, portRef, 0 );

    if ( !loopRef )
    {
        CFRelease( nameRef );
        fprintf( stderr, "Could not create run loop source for MessagePort." );
        return false;
    }

    CFRunLoopTimerRef timerRef = CFRunLoopTimerCreate( NULL, 0, 2, 0, 0, &MyTimerCallBack, NULL );

    if ( !timerRef )
    {
        CFRelease( loopRef );
        CFRelease( nameRef );
        fprintf( stderr, "Could not create CFTimer source for RunLoop." );
        return false;
    }

    CFRunLoopAddSource( ourRunLoop, loopRef, kCFRunLoopCommonModes );
    CFRunLoopAddTimer( ourRunLoop, timerRef, kCFRunLoopCommonModes );

    return true;
}


BOOL FindProcessByBundle( CFStringRef pathRef, ProcessSerialNumber * sn )
{
    if ( !sn )
        return NO;

    NSString * path = (NSString*)pathRef;

    NSEnumerator * enumerator = [[[NSWorkspace sharedWorkspace] launchedApplications] objectEnumerator];

    NSDictionary * application = nil;

    while ( application = [enumerator nextObject] )
    {
        NSString * appPath = [application objectForKey:@"NSApplicationPath"];

        if ( ![path isEqualToString:appPath]  )
            continue;

        sn->highLongOfPSN = [[application objectForKey:@"NSApplicationProcessSerialNumberHigh"] longValue];
        sn->lowLongOfPSN  = [[application objectForKey:@"NSApplicationProcessSerialNumberLow"] longValue];
        return YES;
    }

    return NO;
}



int main (int argc, const char * argv[])
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    char cwd[MAXPATHLEN];
    if (!getcwd(cwd, MAXPATHLEN - 1) )
    {
        fprintf( stderr, "Couldn't get the current directory!\n" );
        return -1;
    }

    char * portName;
    FSRef appRef;
    CFStringRef appPath = 0;

    OSErr err;

    AppleEvent ae, replyEvent;
    bool mergeIsRunning;

    // 1 - Setup message port and callbacks

    if ( !SetupMessagePortAndCallbacks( &portName ) )
        goto bail;

    // 2 - See if p4merge has started...

    if ( !P4MergeApplicationFromArg0( argv[0], &appRef, &appPath ) )
        goto bail;

    mergeIsRunning = FindProcessByBundle( appPath, &P4MERGE_PROCESS );

    // 3 - Create an apple event with the parameters for the port
    //     Use the merge process if you have one.

    err = CreateDiffEventFromParameters(
            P4MERGE_PROCESS,
            &ae,
            argc, argv);

    if ( err != noErr )
        goto bail;

    err = AEPutParamPtr(
        &ae,
        portKeyword,
        typeChar,
        portName,
        strlen(portName));

    if ( err != noErr )
        goto bail;

    err = AEPutParamPtr(
            &ae,
            cwdKeyword,
            typeChar,
            cwd,
            strlen(cwd));

    if ( err != noErr )
        goto bail;

    // 4 - if merge has started, send it the apple event

    if ( mergeIsRunning )
    {
        err = AESendMessage(
            &ae,
            &replyEvent,
            kAENoReply,
            30);

        if ( err != noErr )
        {
            fprintf( stderr, "Could not send event to P4Merge.app (%d)\n", err );
            goto bail;
        }
        else
        {
            AEDisposeDesc( &replyEvent );
        }
        AEDisposeDesc( &ae );

         // Try to force P4Merge to the front. OK (but annoying) if this fails.
        err = SetFrontProcess(&P4MERGE_PROCESS);
    }
    else
    {
        // 5 - if it hasn't, start it with the apple event

        LSApplicationParameters lParams;
        lParams.version           = 0;
        lParams.flags             = kLSLaunchDefaults;
        lParams.application       = &appRef;
        lParams.asyncLaunchRefCon = NULL;
        lParams.environment       = NULL;
        lParams.argv              = NULL;
        lParams.initialEvent      = &ae;

        err = LSOpenApplication( &lParams, &P4MERGE_PROCESS );

        if ( err != noErr )
        {
            fprintf( stderr, "Could not start P4Merge.app (%d)\n", err );
            goto bail;
        }

#ifdef HALT_FOR_DEBUG
        printf("halting for debug");
        char c = getchar();
#endif //HALT_FOR_DEBUG
    }

    // Success!!

    // The loop will stop when the correct callback is called

    CFRunLoopRun();

    if ( appPath )
        CFRelease(appPath);
    [pool drain];

    return 0;

bail:
    if ( appPath )
        CFRelease(appPath);
    [pool drain];

    return -1;
}



OSErr
CreateDiffEventFromParameters(
    ProcessSerialNumber  psn,
    AppleEvent * diffEvent,
    int     argc,
    const char * argv[] )
{
    AEAddressDesc appAddress;
    AEDesc *      aeArgs = NULL;
    AEDescList    aeMainArguments;

    int i;

    OSErr err = noErr;

    ZeroAEDesc(appAddress);
    ZeroAEDesc(aeMainArguments);

    aeArgs = (AEDesc *)malloc( argc * sizeof(AEDesc) );

    for ( i = 0; i < argc; i++ )
        ZeroAEDesc(aeArgs[i]);

    // Make an application address descriptor so that
    // the AppleEvent Manager knows what application we
    // want to send this to
    //
    if ( 0 != psn.highLongOfPSN || 0 != psn.lowLongOfPSN )
    {
        err = AECreateDesc(typeProcessSerialNumber, &psn, sizeof(psn), &appAddress);
        if ( err != noErr )
            goto bail;
    }

    err = AECreateAppleEvent(
        kPerforceAESuite,
        kPerforceMergeMethodEvent,
        &appAddress,
        kAutoGenerateReturnID,
        kAnyTransactionID,
        diffEvent);


    if ( err != noErr )
    goto bail;

    err = AECreateList(
        NULL,
        0,
        false,
        &aeMainArguments);

    if ( err != noErr )
    goto bail;

    //
    // add the commands
    //
    {
        for ( i = 0; i < argc; i++ )
        {
            err = AECreateDesc(typeChar, argv[i], strlen(argv[i]), &aeArgs[i]);

            if ( err != noErr ) goto bail;

            err = AEPutDesc (&aeMainArguments, i + 1, &aeArgs[i]);

            if ( err != noErr ) goto bail;
        }
    }

    //
    // Add the list as the direct parameter
    //
    err = AEPutParamDesc(diffEvent, keyDirectObject, &aeMainArguments);

    if ( err != noErr )
    goto bail;

bail:
    return err;
}
