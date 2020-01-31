//
// P4mAppleEventHandler
// --------------
//
// P4mAppleEventHandler is responsible for handling AppleEvents to P4Merge.
// It is only active for the Mac builds. It converts AppleEvents into
// P4Merge events that the application can understand.
//

#ifndef __P4mAppleEventHandler_H__
#define __P4mAppleEventHandler_H__

#include <QObject>
#include <QMap>

#include <ApplicationServices/ApplicationServices.h>

class P4mApplication;

class P4mAppleEventHandler : QObject
{
    Q_OBJECT

public:
    P4mAppleEventHandler ( P4mApplication & app );
    ~P4mAppleEventHandler ();

    void detach();

    OSErr handleDiffEvent(
        const AppleEvent *theAppleEvent,
        AppleEvent *reply );

    OSErr handleOpenAppEvent(
        const AppleEvent *theAppleEvent,
        AppleEvent *reply );

private slots:
    void releaseShimForSender( QObject * );

private:
    void attach( P4mApplication & );
    P4mApplication &  mApp;
    AEEventHandlerUPP upp;
    AEEventHandlerUPP uppOpenApp;

    QMap< const QObject *, QString > map;
};


#endif // __P4mAppleEventHandler_H__


