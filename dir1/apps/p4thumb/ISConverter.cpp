#include "ISConverter.h"

#include "AttribFilesAgent.h"
#include "AttributeAgent.h"
#include "CounterAgent.h"
#include "DebugLog.h"
#include "FileSizesAgent.h"
#include "ObjChange.h"
#include "ObjStash.h"
#include "OpConfiguration.h"
#include "OpContext.h"
#include "OpFileSystem.h"
#include "OpMessage.h"
#include "OpP4Path.h"
#include "PrintToFileAgent.h"
#include "ThumbUtil.h"

#include <QApplication>
#include <QBuffer>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QTime>
#include <QTimer>
#include <QtPlugin>

#ifdef OS_NT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

static DebugLog::Category* const _qLog2 = DebugLog::GetCategory("ISConverter.hex");     // define the category for hex attribute logging
qLogCategory("ISConverter");        // define default category for thumb

#ifndef MIN_POLL_SECONDS
#define MIN_POLL_SECONDS    1
#endif
#define MAX_POLL_SECONDS    2147483     // maxint before conversion to msecs

#define WATCHDOG_INTERVAL   30*1000     // 30 seconds watchdog timer interval

#define FILES_PER_CHUNK     100

void
ISConverter::SendToStandardOut(QString msg)
{
    QTextStream stout(stdout, QIODevice::WriteOnly);
    stout << msg << "\n";
}

QString
ISConverter::ReadFromStandardIn()
{
    QString line;

    QTextStream stream(stdin);
    do
    {
        line = stream.readLine();
    }
    while (!line.isNull());

    return line;
}

void
ISConverter::ShowUsageInfo()
{
    QString usageInfo =     QString("P4Thumb converts Qt-supported image formats to a small PNG image.\n"
                                    "Formats can be extended by adding scripted extensions supporting\n"
                                    "additional formats.\n"
                                    "The parameters and conditions are loaded from a configuration file"
                                    "Usage:\n"
                                    "p4thumb -c<configuration file> [options] \n"
                                    "Options:\n"
                                    "  -v                : Verbose status output to console, default is silent\n"
                                    "  -d                : Deletes thumbnails from server, must used with a\n"
                                    "                      ChangeListRange in the configuration file\n"
                                    "  -f                : Force to always write thumbnail\n"
                                    "  -V                : Prints application version and exits\n"
                                    "  -test <testFile>  : Test thumb conversion of testFile and exits\n"
                                   );

    SendToStandardOut(usageInfo);
}

void
ISConverter::ShowMessage(MessageType type, const QString& optionalArg)
{
    QString msg;

    if (type & UnknownStartMessage)
        msg += "Error: '-n' can't understand start of range\n";

    if (type & EndPrecedesStartMessage)
        msg += "Error: '-n' end of range precedes the start\n";

    if (type & UnknownEndMessage)
        msg += "Error: '-n' can't understand end of range\n";

    if (type & UnknownOptionMessage)
        msg += QString("Error: unknown option %1\n\n").arg(optionalArg);

    if (type & SetEndForDeleteMessage)
        msg += "Error: You must also set an end change with '-n' to delete thumbnails\n\n";

    if (!msg.isEmpty())
        SendToStandardOut(msg);
}

/*!
    \brief  Constructor for a thumbnail creator
    \param  config  connection configuration for server
    \param  outW    width of thumbnails to generate
    \param  outH    height of thumbnails to generate
*/
ISConverter::ISConverter(const Op::Configuration& config, const int outW,
                         const int outH, int maxfilesz, const QList<Thumb::Conversion>& convs)
    : mTopChange    (0)
    , mCounterChange(0)
    , mCurrentChange(0)
    , mFinalChange  (0)
    , mOutW         (outW)
    , mOutH         (outH)
    , mForceSet     (false)
    , mClearAttribs (false)
    , mConnectionDropped(false)
    , mEchoConsole  (true)
    , mPollIntervalMsec(0)
    , mFilesInProgress(0)
    , mCounterName("p4td-" + config.client())
    , mAttribKey("thumb")
    , mStash        (new Obj::Stash())
    , mPollTimer    (new QTimer(this))
    , mWatchdogTimer(new QTimer(this))
    , mWatchdogCounter(0)
    , mLastWatchdogCounterValue(0)
    , mAttribs      (new Obj::AttributeAgent(mStash))
    , mImageFilesInChange(new Obj::AttribFilesAgent(mStash, mAttribKey))
    , mThumbPlugin(convs)
    , mMaxFileSize(maxfilesz)
{
    mStash->setSpecListFetchMode(Obj::Single);  // only need most recent change
    mStash->setConfiguration(config);
    mStash->context()->setErrorPrompter(this);

    Op::Context::SetStreamsFeatureEnabled(true);

    connect(mStash->context(),
            SIGNAL( logItem(const Op::LogDesc&) ),
            this,
            SLOT( onContextLogItem(const Op::LogDesc&) ));
    connect(mStash->context(),
            SIGNAL( beginBusy() ),
            this,
            SLOT( onContextBeginBusy() ));
    connect(mStash->context(),
            SIGNAL( endBusy() ),
            this,
            SLOT( onContextEndBusy() ));
    connect(mStash->context(),
            SIGNAL( connected(int) ),
            this,
            SLOT( onContextConnected(int) ));
    connect(mStash->context(),
            SIGNAL( disconnected(int) ),
            this,
            SLOT( onContextDisconnected(int) ));

    mChangeList = mStash->formListForType(Obj::CHANGE_TYPE);

    connect(mImageFilesInChange,
            SIGNAL( done(bool) ),
            SLOT( onFilesUpdated(bool) ));

    connect(qApp,
            SIGNAL( aboutToQuit() ),
            SLOT( onExit() ));

    mPollTimer->setSingleShot(true);
    connect(mPollTimer,
            SIGNAL( timeout() ),
            SLOT( onPollTimerTimeout() ));

    connect(mWatchdogTimer,
            SIGNAL( timeout() ),
            SLOT( onWatchdogTimerTimeout() ));
    mWatchdogTimer->start(WATCHDOG_INTERVAL);

    QStringList qtformats;
    foreach (QByteArray fmt, QImageReader::supportedImageFormats())
    {
        qtformats <<  QString::fromUtf8(fmt);
    }
    rLog(1) << "Supported qt image formats : " << qtformats.join(" ") << qLogEnd;
}

ISConverter::~ISConverter()
{
    if (mPollTimer)
        mPollTimer->stop();
    delete mImageFilesInChange;
    delete mAttribs;
    delete mStash;
    Op::FileSystem::DeleteAllTemporaryFiles();
}

void
ISConverter::logStatus(const QString& msg, Op::LogDesc::Type type)
{
    Op::LogDesc logMsg(type, msg, mStash->context());
//    mStash->context()->emitLogItem(logMsg);
    rLog(0) << msg << qLogEnd;
    if (mEchoConsole)
        SendToStandardOut(msg);
}

bool
ISConverter::checkConnection()
{
    // test connection to server, which also sets ServerVersion
    const Op::Configuration config = mStash->configuration();
    Op::CheckConnectionStatus ccs = mStash->checkConnection();
    mWatchdogCounter++;

    if (config.client().isEmpty())
    {
        const QString msg = QString("No client is provided. p4thumb requires a client."); 
        logStatus(msg);
        // always display on stdout, too
        SendToStandardOut(msg);
	return false;
    }
    else if (Op::CCS_Ok == ccs)
    {
        const QString msg =
            QString("Starting connection: port=%1, client=%2, user=%3")
            .arg(config.port())
            .arg(config.client())
            .arg(config.user());
        logStatus(msg);
        // always display on stdout, too
        SendToStandardOut(msg);
        return true;
    }
    else
    {
        const QString msg =
            QString("Connection check failed: port=%1, client=%2, user=%3")
            .arg(config.port())
            .arg(config.client())
            .arg(config.user());
        logStatus(msg);
        // always display on stdout, too
        SendToStandardOut(msg);
    }

    QString errMsg;
    switch (ccs)
    {
    case Op::CCS_NeedPassword:
    case Op::CCS_InvalidPassword:
        errMsg = QString("A valid password is required for user '%1' on\n"
                         "the server at %2")
                 .arg(config.user())
                 .arg(config.port());
        break;

    case Op::CCS_NoSuchClient:
    case Op::CCS_ClientNoValidRoot:
    case Op::CCS_ClientLockedHost:
        errMsg = QString(
                     "Client workspace '%1' is not valid on the server at %2")
                 .arg(config.client())
                 .arg(config.port());
        break;

    case Op::CCS_ServerTooOld:
        errMsg = QString("P4TD requires a Perforce server version %1 or newer")
                 .arg(mStash->context()->serverProtocol().serverVersionString(Op::OLDEST_SUPPORTED_SERVER_VERSION));
        break;

    case Op::CCS_ClientIsUnloaded:
        errMsg = QString("This workspace cannot be used because it has been unloaded. It must be reloaded to be used (p4 reload -c).");
        break;

    case Op::CCS_UnicodeServerNeedsCharset:
        errMsg = QString("A valid charset is required by the server at %1")
                 .arg(config.port());
        break;

    default:
        errMsg = QString("Could not connect to Perforce server at\n"
                         "%1 with user '%2' and client workspace '%3'")
                 .arg(config.port())
                 .arg(config.user())
                 .arg(config.client());
    }
    logStatus(errMsg, Op::LogDesc::LogError);
    return false;
}

/*!
    \brief Call this method to start the timer and check for work
    \param  pollInterval    polling interval in seconds

    Returns false when fails to start -- error is logged
 */
bool
ISConverter::beginPolling(int pollInterval)
{
    mWatchdogCounter++;

    if (!checkConnection())
        return false;

    mImageFilesInChange->setFilterListEnabled(!mForceSet);

    connect( this,
             SIGNAL(changeComplete(unsigned int)),
             SLOT(processChange(unsigned int)),
             Qt::QueuedConnection);

    // Start processing files 1/2 sec after QApplication starts processing events
    // This will result in a call back to onPollTimerTimeout.
    mPollTimer->start(500);

    // thereafter, use polling interval; QTimer uses milliseconds
    mPollIntervalMsec = qBound(MIN_POLL_SECONDS, pollInterval, MAX_POLL_SECONDS) * 1000;

    return true;
}

/*!
    \brief  poll server checking for new image files to convert

    Each time the timer signals, we need to check the server for new image
    files to convert.  The last change number  tested is saved in the "images"
    counter.  Run a files command to see what image files have been
    added/modified since the last time, and update the thumbnail attribute for
    each one.  When complete, set the value of "images" to the head change ID.
 */
void
ISConverter::onPollTimerTimeout()
{
    mWatchdogCounter++;

    // This couldn't be set initially, because timer was started with
    // 500ms interval for first poll.  This only needs to be set once
    // after the first poll, but it does no harm to set it each time.
    mPollTimer->setInterval(mPollIntervalMsec);

    rLog(2) << "onPollTimerTimeout" << qLogEnd;

    // Make sure all operations are complete before starting a new batch of
    // conversions.
    if (mStash->context()->isBusy())
    {
        mPollTimer->start();
        return;
    }

    Obj::CounterAgent* agent = new Obj::CounterAgent(mStash);

    connect(agent,
            SIGNAL(done(bool, const Obj::Agent&)),
            SLOT(onReadCounterDone(bool, const Obj::Agent&)));
    connect(agent,
            SIGNAL(message(const Op::Message&, const Op::Operation*)),
            SLOT(onReadCounterMessage(const Op::Message&, const Op::Operation*)));

    // start CounterAgent to read the counter
    // this will call back to onCounterRead.
    const QString msg(QString("reading counter: %1").arg(mCounterName));
    logStatus(msg);
    rLog(2) << "reading counter " << mCounterName << qLogEnd;
    agent->readValue(mCounterName);
}

void
ISConverter::onReadCounterMessage(const Op::Message& msg, const Op::Operation* op)
{
    Q_UNUSED(op)

    mWatchdogCounter++;
    rLog(2) << msg.formatted() << qLogEnd;
}

/*!
    \brief  callback from counter agent with result of reading the counter
    \param  success if the agent was successful
    \param  agent   the agent
*/
void
ISConverter::onReadCounterDone(bool success, const Obj::Agent& agent)
{
    Q_UNUSED(success)

    mWatchdogCounter++;

    mCounterChange =  agent.property("counterValue").toUInt();
    rLog(2) << "read counter " << mCounterName << ", got " << mCounterChange << qLogEnd;

    // first time through, set the current change to what's read from the
    // counter, but not less than 1.
    if (mCurrentChange == 0)
    {
        mCurrentChange = mCounterChange;
        if (mCurrentChange == 0)
            mCurrentChange = 1;
    }

    // log that we're working on the current change
    QString msg;
    msg = "Checking for work at changelist number "
          + QString::number(mCurrentChange);
    logStatus(msg);

    // trigger update of changelist list
    // this will call back to onChangeListUpdated.
    mChangeList->invokeWhenValid(this, "onChangeListUpdated", mStash, true);
}

/*!
    \brief  called when the change list list is updated

    When we get this signal, we can determine the files list for the
    current change we're processing.
 */
void
ISConverter::onChangeListUpdated(Obj::Object*)
{
    rLog(2) << "change list updated" << qLogEnd;
    mWatchdogCounter++;

    const QStringList changeIDs = mChangeList->identifiers();

    bool    ok = false;
    if (!changeIDs.isEmpty())
        mTopChange = changeIDs.first().toUInt(&ok);
    else
        mTopChange = 0;

    if (!ok)
    {
        // Check if there are no changes on server at all
        if (mCurrentChange == 1 && mCounterChange == 0)
        {
            QString msg;
            msg = "No changes to process.  Waiting for new submissions.";
            logStatus(msg);
            mPollTimer->start();
        }
        else
        {
            rLog(2) << "No changes found; currentChange: "
                    << mCurrentChange << ", counterChange: "
                    << mCounterChange << qLogEnd;
        }
        return;
    }

    // handle special case of requesting up to change -1
    // this means (apparently) they want to process all
    // currently existing change and then stop
    if (mFinalChange == (unsigned int)-1)
        mFinalChange = mTopChange;

    processChange(mCurrentChange);
}

void
ISConverter::processChange(unsigned int changeNum)
{
    rLog(2) << "processChange:" << changeNum << qLogEnd;

    // should not have a timer running here
    Q_ASSERT( !mPollTimer->isActive() );

    bool exitNow = false;

    mWatchdogCounter++;

    if (mFinalChange && changeNum > mFinalChange)
    {
        QString msg;
        msg = "All changes processed.";
        logStatus(msg);
        exitNow = true;
    }

    if (exitNow)
    {
        mStash->context()->wait();
        QApplication::postEvent(qApp, new QEvent(QEvent::Quit));
        return;
    }

    // if we've processed all the changes in the list, check again later
    if (changeNum > mTopChange)
    {
        // next timer check the 'images' counter
        mCurrentChange = 0;

        rLog(2) << "restarting poll timer, topChange: " << mTopChange << qLogEnd;

        mPollTimer->start();

        return;
    }

    // start AttribFilesAgent to get list of files in change
    // along with their attributes so we know which ones need
    // thumbnails made for them.
    // This will call back to onFilesUpdated.
    rLog(2) << "getting file attributes for change: " << changeNum << qLogEnd;
    const QString current = QString::number(changeNum);
    mImageFilesInChange->setChange(mCurrentChange);
    mImageFilesInChange->run();
}


// For each file, create the thumbnail image and set the file attribute
void
ISConverter::onFilesUpdated(bool)
{
    mWatchdogCounter++;

    // list of depot filepaths
    mFilesToConvert = mImageFilesInChange->filesList();

    QString msg;
    // if there are no files to convert in this change, then we're done with it
    if (mFilesToConvert.isEmpty())
    {
        msg = "No images to convert at changelist "
              + QString::number(mCurrentChange);
        logStatus(msg);
        finishChange();
        return;
    }

    msg = "File list updated with " + QString::number(mFilesToConvert.count())
          + " items from changelist " + QString::number(mCurrentChange);
    logStatus(msg);

    if (mClearAttribs)
    {
        // clear attribute on each file in the list
        foreach (const QString file, mFilesToConvert)
            mAttribs->setValue(Op::P4Path::FromP4EscapedPath(file), mAttribKey, QString());
        mFilesToConvert.clear();
        finishChange();
        return;
    }

    // convert all of the files in manageable chunks
    processCurrentFiles();

    // if somehow all the files are instantly processed, we're done
    if (mFilesInProgress == 0 && mFilesToConvert.isEmpty())
        finishChange();
}

/*!
    Process the current files in chunks until all files to convert are completed.
*/
void
ISConverter::processCurrentFiles()
{
    mWatchdogCounter++;

    Q_ASSERT(!mFilesToConvert.isEmpty());

    // process the smaller of the chunkSize or filesPerChange
    int chunkSize = qMin(FILES_PER_CHUNK, mFilesToConvert.count());
    mFilesInProgress = chunkSize;

    QString msg;
    msg = "Processing " + QString::number(chunkSize)
          + " files from changelist " + QString::number(mCurrentChange);
    logStatus(msg);

    while (chunkSize--)
    {
        Q_ASSERT(!mFilesToConvert.isEmpty());
        if (mFilesToConvert.isEmpty())  // test for safety
            break;

        // Remove the files from the front of the list
        const QString depotPath = mFilesToConvert.takeFirst();

        if (mMaxFileSize == 0 )
        {
            launchPrintToFile(depotPath);
        }
        else
        {
            Obj::FileSizesAgent* fileSizesAgent = new Obj::FileSizesAgent(mStash);
            fileSizesAgent->setMode(Obj::FileSizesAgent::Detailed);
            fileSizesAgent->addPathArgument(Op::P4Path::FromP4EscapedPath(depotPath));
            connect(fileSizesAgent, SIGNAL(fileSize(const QString&, unsigned)),
                    this, SLOT(fileSizeCheck(const QString&, unsigned)));
            fileSizesAgent->run();
        }
    }
}

void
ISConverter::fileSizeCheck(const QString& path, unsigned size)
{
    if (size < (unsigned)mMaxFileSize)
    {
        launchPrintToFile(path);
    }
    else
    {
        QString msg;
        msg = path + " file size: " + QString::number(size) + " is larger then the maxFileSize";
        logStatus(msg);

	 // continue processing files.
	 onPollTimerTimeout();
	 mCurrentChange++;
    }
}


void
ISConverter::launchPrintToFile(const QString& depotPath)
{
    // print the file to a temp file for thumb generation
    // this will call back to onPRFUpdated
    Obj::PrintToFileAgent* printtofileAgent = new Obj::PrintToFileAgent(mStash);
    printtofileAgent->addPathArgument(Op::P4Path::FromP4EscapedPath(depotPath));
    connect(printtofileAgent, SIGNAL( done(bool, const Obj::Agent&) ),
            this, SLOT( filePrinted(bool, const Obj::Agent&) ));
    printtofileAgent->run();
}

void
ISConverter::filePrinted(const bool success,
                         const Obj::Agent& agent)
{
    mWatchdogCounter++;

    QBuffer b;
    b.open(QIODevice::ReadWrite);
    const QString path = ((const Obj::PrintToFileAgent&)agent).getPath();
    const QString depotPath = ((const Obj::PrintToFileAgent&)agent).getDepotPath();

    if (mThumbPlugin.convert(path))
    {
        QString thumbfile = mThumbPlugin.thumbFileName();
        QFileInfo tinfo(thumbfile);
        QString ext = tinfo.suffix();
        QImage thumbImg;
        if (thumbImg.load(thumbfile))
        {
            if (thumbImg.save(&b, "PNG"))
            {
                QByteArray data(b.buffer());

                const QString msg = QString::number(b.size()) + " byte thumbnail from " + depotPath;
                logStatus(msg);

                // set thumb attribute for the file
                mAttribs->setValue(Op::P4Path::FromP4EscapedPath(depotPath), mAttribKey, &data);
                rLog(1) << mFilesInProgress << " files remaining" << qLogEnd;
                rLog(1) << qLogEnd;
                rLogToCategory("ISConverter.hex", 1) << data.toHex().constData() << qLogEnd;
            }
            else
            {
                const QString msg = "Can't convert image to thumbnail for " + path;
                logStatus(msg, Op::LogDesc::LogError);
            }
        }
        else
        {
            const QString msg = "Thumbnail file not in Qt supported image format " + path;
            logStatus(msg, Op::LogDesc::LogError);
        }

        QFile tfile(thumbfile);
        if (!tfile.remove())
        {
            const QString msg = "Can't delete thumbnail file " + thumbfile;
            logStatus(msg, Op::LogDesc::LogError);
        }
    }
    else if (success && ThumbUtil::ConvertFileToThumbnail(path, &b, mOutW, mOutH))
    {
        QByteArray data(b.buffer());

        const QString msg = QString::number(b.size()) + " byte thumbnail from " + depotPath;
        logStatus(msg);

        // set thumb attribute for the file
        mAttribs->setValue(Op::P4Path::FromP4EscapedPath(depotPath), mAttribKey, &data);
        rLog(1) << mFilesInProgress << " files remaining" << qLogEnd;
        rLog(1) << qLogEnd;
        rLogToCategory("ISConverter.hex", 1) << data.toHex().constData() << qLogEnd;
    }
    else
    {
        const QString msg = "Can't convert image file " + path;
        logStatus(msg, Op::LogDesc::LogError);
    }


    // remove the file printed
    QFile pfile(path);
    if (!pfile.remove())
    {
        const QString msg = "Can't delete temporary file " + path;
        logStatus(msg, Op::LogDesc::LogError);
    }

    // continue current chunk if not done
    if (--mFilesInProgress > 0)
        return;

    // if no more files, all done with this change
    if (mFilesToConvert.isEmpty())
    {
        finishChange();
        return;
    }

    // else start next chunk if files left
    processCurrentFiles();
}

void
ISConverter::finishChange()
{
    mWatchdogCounter++;

    Q_ASSERT(mFilesInProgress == 0);

    rLog(2) << "finished change " << mCurrentChange << qLogEnd;

    // prepare to process the next changelist
    ++mCurrentChange;
    mChangeList->clearList();   // keeps list from growing
    Op::FileSystem::DeleteAllTemporaryFiles();

    if (!mClearAttribs && mCounterChange < mCurrentChange)
    {
        // Set the counter to next change after all files processed
        // only if it increases the number, and we are not deleting
        Obj::CounterAgent* agent = new Obj::CounterAgent(mStash);

        connect(agent,
                SIGNAL(done(bool, const Obj::Agent&)),
                SLOT(onWriteCounterDone(bool, const Obj::Agent&)));
        connect(agent,
                SIGNAL(message(const Op::Message&, const Op::Operation*)),
                SLOT(onWriteCounterMessage(const Op::Message&, const Op::Operation*)));

        rLog(2) << "updating counter " << mCounterName << qLogEnd;

        // this will call back to onCounterWritten
        agent->writeValue(mCounterName, mCurrentChange);
        return;
    }

    // this will call back to processChange
    emit changeComplete(mCurrentChange);
}

void
ISConverter::onWriteCounterMessage(const Op::Message& msg, const Op::Operation* op)
{
    Q_UNUSED(op)

    mWatchdogCounter++;
    rLog(2) << "onWriteCounterMessage: " << msg.formatted() << qLogEnd;
}

/*!
    completed the changelist, and wrote new counter
 */
void
ISConverter::onWriteCounterDone(bool, const Obj::Agent&)
{
    mWatchdogCounter++;

    rLog(2) << "counter " << mCounterName << " updated" << qLogEnd;

    // this will call back to processChange
    emit changeComplete(mCurrentChange);
}

void
ISConverter::onExit()
{
    rLog(2) << "QUITTING!" << qLogEnd;

    mPollTimer->stop();
    delete mPollTimer;
    mPollTimer = 0;
    delete mImageFilesInChange;
    mChangeList = Ptr<Obj::ChangeFormList>( NULL );
    delete mAttribs;
    Op::FileSystem::DeleteAllTemporaryFiles();
}

void
ISConverter::onContextLogItem(const Op::LogDesc& desc)
{
    QString msg = desc.text();
    rLog(0) << msg << qLogEnd;
}

void
ISConverter::onContextBeginBusy()
{
    rLog(2) << "onContextBeginBusy" << qLogEnd;
}

void
ISConverter::onContextEndBusy()
{
    rLog(2) << "onContextEndBusy" << qLogEnd;
}

void
ISConverter::onContextConnected(int num)
{
    rLog(2) << "onContextConnected " << num << qLogEnd;
}

void
ISConverter::onContextDisconnected(int num)
{
    rLog(2) << "onContextDisconnected " << num << qLogEnd;
}

Op::ErrorPrompter::Result
ISConverter::promptOldAndNewPassword(const Op::Configuration&, const Op::Message&)
{
    return Fail;
}

QString
ISConverter::password()
{
    return QString();
}

QString
ISConverter::oldPassword()
{
    return QString();
}

Op::ErrorPrompter::Result
ISConverter::promptFingerprint(const Op::Configuration&, const Op::Message&)
{
    rLog(2) << "promptFingerprint" << qLogEnd;
    return Retry;
}

Op::ErrorPrompter::Result
ISConverter::promptConnection(const Op::Configuration&, const Op::Message&)
{
    rLog(2) << "promptConnection" << qLogEnd;
    return Retry;
}

Op::ErrorPrompter::Result
ISConverter::promptReconnect()
{
    rLog(2) << "promptReconnect" << qLogEnd;
    return Retry;
}

bool
ISConverter::promptUnhandledError(const Op::Operation&, const Op::Message& msg)
{
    rLog(0) << msg.formatted() << qLogEnd;
    SendToStandardOut(msg.formatted());
    return true;
}

void
ISConverter::onWatchdogTimerTimeout()
{
    if (mLastWatchdogCounterValue == mWatchdogCounter)
    {
        rLog(2) << "no activity detected for "
                << WATCHDOG_INTERVAL/1000
                << " seconds ["
                << mWatchdogCounter
                << "]" << qLogEnd;
    }
    mLastWatchdogCounterValue = mWatchdogCounter;
}

