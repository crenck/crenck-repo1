#ifndef INCLUDED_ISCONVERTER
#define INCLUDED_ISCONVERTER

/*
 * ISConverter
 * Constructed with a Configuration to connect to specified
 * p4d server at specified interval in minutes.
 * Checks for image files indicated by P4Client workspace,
 * and creates thumbnail images for each supported image.
 * Sets thumbnail image attribute on source depot file.
 */

#include <iostream>

#include "Ptr.h"
#include "ObjTypes.h"

// Required for gcc
#include "ObjChangeFormList.h"

#include "OpErrorPrompter.h"
#include "OpLogDesc.h"

#include "ThumbPlugin.h"

class QTimer;

#include <QTextStream>

namespace Op
{
class Configuration;
class Message;
class Operation;
}

namespace Obj
{
class Agent;
class ChangeFormList;
class AttribFilesAgent;
class AttributeAgent;
class FileRevision;
class PrintToFile;
class Stash;
}

typedef unsigned int MessageType;
const MessageType NoMessage               = 0x00;
const MessageType UnknownStartMessage     = 0x01;
const MessageType EndPrecedesStartMessage = 0x02;
const MessageType UnknownEndMessage       = 0x04;
const MessageType UnknownOptionMessage    = 0x08;
const MessageType SetEndForDeleteMessage  = 0x10;


class ISErrorPrompter : public Op::ErrorPrompter
{
public:
    ISErrorPrompter() : Op::ErrorPrompter(0) {}

    Result promptLoginPassword(const Op::Configuration&, const Op::Message&, const QString&)
    {
        return Fail;
    }
};

class ISConverter : public ISErrorPrompter
{
    Q_OBJECT

public:

    static  void SendToStandardOut(QString msg);
    static  QString ReadFromStandardIn();
    static  void ShowUsageInfo();
    static  void ShowMessage(MessageType type, const QString& optionalArg = QString());

    ISConverter(const Op::Configuration& config, int outW, int outH, int maxfilesz, const QList<Thumb::Conversion>& convs);
    ~ISConverter();

    bool    beginPolling(int pollInterval);

    // Start processing at this changelist
    void    setFirstChange(unsigned int c)
    {
        mCurrentChange = c;
    }

    // Stop after processing this changelist
    void    setFinalChange(unsigned int c)
    {
        mFinalChange = c;
    }

    // Deletes thumbnail attributes instead of generating them
    void    setDeleteEnabled(bool b)
    {
        mClearAttribs = b;
    }

    // Always converts images and sets thumbnail data when set
    // Otherwise, skips files with existing thumbnail data
    void    setAlwaysConvertEnabled(bool b)
    {
        mForceSet = b;
    }

    // Enable verbose status output to console
    void    setShowConsoleOutput(bool b)
    {
        mEchoConsole = b;
    }

signals:
    void changeComplete(unsigned int changeNum);

public slots:
    void onExit();

private slots:
    void onPollTimerTimeout();
    void onWatchdogTimerTimeout();
    void onReadCounterDone(bool success, const Obj::Agent& agent);
    void onReadCounterMessage(const Op::Message& msg, const Op::Operation* op);
    void onWriteCounterDone(bool success, const Obj::Agent& agent);
    void onWriteCounterMessage(const Op::Message& msg, const Op::Operation* op);
    void onChangeListUpdated(Obj::Object*);
    void onFilesUpdated(bool);
    void filePrinted(bool success, const Obj::Agent& agent);
    void processChange(unsigned int changeNum);
    void onContextLogItem(const Op::LogDesc& desc);
    void onContextBeginBusy();
    void onContextEndBusy();
    void onContextConnected(int numConnections);
    void onContextDisconnected(int numConnections);
    void fileSizeCheck(const QString& path, unsigned size);

    // errorPrompter interface:
    virtual Result promptOldAndNewPassword(const Op::Configuration& config, const Op::Message& msg);
    virtual QString password();
    virtual QString oldPassword();
    virtual Result promptFingerprint(const Op::Configuration& config, const Op::Message& msg);
    virtual Result promptConnection(const Op::Configuration&, const Op::Message&);
    virtual Result promptReconnect();
    virtual bool promptUnhandledError(const Op::Operation&, const Op::Message&);

private:
    void logStatus(const QString&, Op::LogDesc::Type type = Op::LogDesc::LogOther);
    bool checkConnection();
    void processCurrentFiles();
    void finishChange();
    void launchPrintToFile(const QString&);

    unsigned int    mTopChange;     // head changelist on server
    unsigned int    mCounterChange; // highest processed changelist
    unsigned int    mCurrentChange; // processing this changelist
    unsigned int    mFinalChange;   // stop after this changelist
    const int       mOutW;          // thumb width
    const int       mOutH;          // thumb height
    bool            mForceSet;      // always converts images and sets thumbnail
    bool            mClearAttribs;  // delete thumbnails
    bool            mConnectionDropped; // true if server connection fails
    bool            mEchoConsole;   // repeat logStatus on console display
    int             mPollIntervalMsec;
    int             mFilesInProgress;
    const QString   mCounterName;
    const QString   mAttribKey;
    QStringList     mFilesToConvert;
    Ptr<Obj::ChangeFormList>    mChangeList;
    Obj::Stash*                 mStash;
    QTimer*                     mPollTimer;
    QTimer*                     mWatchdogTimer;
    int                         mWatchdogCounter;
    int                         mLastWatchdogCounterValue;
    Obj::AttributeAgent*        mAttribs;
    Obj::AttribFilesAgent*      mImageFilesInChange;
    Thumb::ThumbPlugin          mThumbPlugin;
    int                         mMaxFileSize;
};

#endif  // INCLUDED_ISCONVERTER

