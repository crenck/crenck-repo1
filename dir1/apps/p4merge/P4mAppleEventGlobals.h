#ifndef __P4mAppleEventGlobals_H__
#define __P4mAppleEventGlobals_H__

#define kPerforceAESuite 'p4cs'
#define kPerforceMergeMethodEvent 'mrge'

const AEKeyword portKeyword = 'port';
const AEKeyword cwdKeyword = 'cwd ';

const OSType mergeSignature = 'P4MG';

enum ShimMessages
{
    StdOutMessage,
    StdErrMessage,
    QuitMessage
};

#endif // __P4mAppleEventGlobals_H__
