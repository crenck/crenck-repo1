//    ClientPortCommandHandler.cpp - class for handling port based communication
//                                   and dispatching

#include "ClientPortCommandHandler.h"

#include "CommandIds.h"
#include "CorePath.h"
#include "DAssert.h"
#include "DebugLog.h"
#include "GuiWindowManager.h"
#include "JsonParser.h"
#include "P4VCommandLine.h"
#include "SandstormApp.h"
#include "Workspace.h"

#include <QDialog>
#include <QDir>

qLogCategory("ClientPortCommandHandler");

/*! \brief constructor

    \param cmdLine P$VCommandLine object that this object will own
*/

ClientPortCommandHandler::ClientPortCommandHandler(P4VCommandLine* cmdLine)
: mCommandLine( cmdLine )
, mSocket( NULL )
, mWorkspace( NULL )
{
    Q_ASSERT(mCommandLine.get());
    handleCommand(mCommandLine->runner());
    if (!mCurrentCommand.get())
        deleteLater();
}

/*! \brief constructor

    \param port integer port for calling back the requesting app via localhost TCP/IP
*/

ClientPortCommandHandler::ClientPortCommandHandler(int port)
: mSocket( new QTcpSocket(this) )
, mWorkspace(NULL)
{
    startClient(port);
}

/*! \brief constructor
*/

ClientPortCommandHandler::ClientPortCommandHandler(const P4VCommandRunner& cr)
: mSocket( NULL )
, mWorkspace(NULL)
{
    handleCommand(cr);
    if (!mCurrentCommand.get())
        deleteLater();
}

/*! \brief destructor
*/

ClientPortCommandHandler::~ClientPortCommandHandler()
{
}

/*! \brief start the TCP/IP client connection

    \param port integer port number for TCP/IP communication
*/

void
ClientPortCommandHandler::startClient(int port)
{
    D_ASSERT(mSocket);
    if (!mSocket)
        return;

    connect(mSocket, SIGNAL(connected()), SLOT(onConnected()));
    connect(mSocket, SIGNAL(disconnected()), SLOT(onDisconnected()));
    connect(mSocket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(onError(QAbstractSocket::SocketError)));
    connect(mSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
                     SLOT(onStateChanged(QAbstractSocket::SocketState)));
    connect(mSocket, SIGNAL(readyRead()), SLOT(onReadyRead()));

    mSocket->connectToHost("localhost", port);
}

/*! \brief slot callback for when the TCP/IP connection is established
*/

void
ClientPortCommandHandler::onConnected()
{
    rLog(1) << "connected" << qLogEnd;
    sendHandshake();

    // process any command line, release our initial command runner
    if (mCommandLine.get())
    {
        // do not allow showMainWindow with -client <port>
        if (mCommandLine->runner().command() != Cmd_MainWindow)
            handleCommand(mCommandLine->runner());
    }
}

/*! \brief send the handshake
*/

void
ClientPortCommandHandler::sendHandshake()
{
    // send version and supported message protocol
    sendJson("{ \"version\": \"1\", \"protocols\": [ \"json\" ]}");
}

/*! \brief read the block length for an incoming message
*/

int
ClientPortCommandHandler::readBlockLength()
{
    D_ASSERT(mSocket);
    if (!mSocket)
        return -1;

    QByteArray data;
    // read until we see a LF or it's impossible that we're reading the length
    while (mSocket->bytesAvailable() && data.length() < 20)
    {
        data.append(mSocket->read(1));
        if (data[data.length() - 1] == '\n')
        {
            bool ok;
            int retVal = QString::fromUtf8(data, data.length() - 1).toInt(&ok);
            return (ok) ? retVal : -1;
        }
    }

    return -1;
}

/*! \brief slot callback for when data is available to read on the port
*/

void
ClientPortCommandHandler::onReadyRead()
{
    D_ASSERT(mSocket);
    if (!mSocket)
        return;

    const int MAX_BUFFER_LEN = 1024*1024;

    // read out the first line, it has the length of the next block
    int blockLength = readBlockLength();
    if (blockLength <= 0)
    {
        rLog(1) << "bad block length: " << blockLength << qLogEnd;
        mSocket->close();
        return;
    }
    if (blockLength  >= MAX_BUFFER_LEN)
    {
        rLog(1) << blockLength << " exceeds " << MAX_BUFFER_LEN << " bytes, disconnecting" << qLogEnd;
        mSocket->close();
        return;
    }
    QByteArray data = mSocket->read(blockLength);
    rLog(1) << "completed reading: " << data << qLogEnd;

    P4VCommandRunner runner;
    QString msg;
    if (!parseJsonCommand(QString::fromUtf8(data), runner, msg))
    {
        sendError(msg);
        return;
    }

    handleCommand(runner);
    sendStatus("pending", QString("%1, %2, %3").arg(runner.config().serializedString())
                                .arg(QString::fromUtf8(runner.command().toString()))
                                .arg(runner.targets().join(", ")));
}

/*! \brief slot callback for when the TCP/IP communication has disconnected
*/

void
ClientPortCommandHandler::onDisconnected()
{
    rLog(1) << "disconnected" << qLogEnd;
    deleteLater();
}

/*! \brief slot callback for when the TCP/IP communication has an error
*/

void
ClientPortCommandHandler::onError( QAbstractSocket::SocketError socketError )
{
    rLog(1) << "socketerror: " << socketError << qLogEnd;
}

/*! \brief slot callback for when the TCP/IP communication has state changes
*/

void
ClientPortCommandHandler::onStateChanged( QAbstractSocket::SocketState socketState )
{
    rLog(1) << "statechanged: " << socketState << qLogEnd;
}

/*! \brief when an entire message has been received, start the process of executing the command
*/

void
ClientPortCommandHandler::handleCommand(const P4VCommandRunner& runner)
{
    if (mCurrentCommand.get())
    {
        rLog(1) << "concurrent handle commands on the same port" << qLogEnd;
        sendError("concurrent handle commands on the same port");
        return;
    }

    if (runner.config().isEmpty())
        return;

    rLog(1) << "handleCommand()" << qLogEnd;

    // save it for later
    mCurrentCommand.reset(new P4VCommandRunner(runner));
    mWorkspace = SandstormApp::sApp->loadWorkspace(mCurrentCommand->config(), false);
    if (!mWorkspace)
    {
        rLog(1) << "failed to load workspace" << qLogEnd;
        sendError(QString("failed to load %1").arg(mCurrentCommand->config().serializedString()));
        mCurrentCommand.release();
    }
    else
    {
        connect(mWorkspace->windowManager(), SIGNAL(windowActivated(QWidget*)), SLOT(onWindowActivated(QWidget*)));
        connect( mCurrentCommand.get(), SIGNAL(error(const QString&)), SLOT(onCommandError(const QString&)) );
        connect( mCurrentCommand.get(), SIGNAL(info(const QString&)),  SLOT(onCommandInfo(const QString&)) );
        if (!mCurrentCommand->start(mWorkspace))
        {
            sendError(QString("failed to start command %1").arg(QString::fromUtf8(mCurrentCommand->command().toString())));
            disconnect(mWorkspace->windowManager(), SIGNAL(windowActivated(QWidget*)), this, SLOT(onWindowActivated(QWidget*)));
            mCurrentCommand.release();
        }
    }
}

/*! \brief slot callback for when the current command has an error
*/

void
ClientPortCommandHandler::onCommandError(const QString& msg)
{
    sendError(msg);
    // disconnect from the windowManager
    disconnect(mWorkspace->windowManager(), SIGNAL(windowActivated(QWidget*)), this, SLOT(onWindowActivated(QWidget*)));
}

/*! \brief slot callback for when the current command has an info (e.g. some
            files could not be retreived
*/

void
ClientPortCommandHandler::onCommandInfo(const QString& msg)
{
    sendStatus("information", msg);
}

/*! \brief slot callback for when the current command's GUI has started
*/

void
ClientPortCommandHandler::onWindowActivated(QWidget* window)
{
    disconnect(mWorkspace->windowManager(), SIGNAL(windowActivated(QWidget*)), this, SLOT(onWindowActivated(QWidget*)));
    rLog(1) << "connecting to destroyed" << qLogEnd;
    connect(window, SIGNAL(destroyed(QObject*)), SLOT(onWindowDestroyed(QObject*)));
    QDialog* dlg = dynamic_cast<QDialog*>(window);
    if (dlg)
    {
        connect(dlg, SIGNAL(accepted()), SLOT(onAccepted()));
        connect(dlg, SIGNAL(rejected()), SLOT(onRejected()));
    }
    sendStatus("started", QString("started command on %1").arg(mCurrentCommand->config().serializedString()));
}

/*! \brief slot callback for when the current command's GUI (dialog) has accepted
*/

void
ClientPortCommandHandler::onAccepted()
{
    sendStatus("accepted", QString("action accepted on %1").arg(mCurrentCommand->config().serializedString()));
}

/*! \brief slot callback for when the current command's GUI (dialog) has rejected
*/

void
ClientPortCommandHandler::onRejected()
{
    sendStatus("rejected", QString("action rejected on %1").arg(mCurrentCommand->config().serializedString()));
}

/*! \brief slot callback for when the current command's GUI has been destroyed
*/

void
ClientPortCommandHandler::onWindowDestroyed(QObject*)
{
    rLog(1) << "client port command finished" << qLogEnd;
    sendStatus("completed", QString("completed command on %2").arg(mCurrentCommand->config().serializedString()));
    // command is over, reset the runner
    mCurrentCommand.reset(NULL);
    if (!mSocket)
        deleteLater();
}

/*! \brief helper function for formatting errors sent to the caller
*/

void
ClientPortCommandHandler::sendError(const QString& msg)
{
    rLog(1) << "error: " << msg << qLogEnd;
    sendJson(QString("{ \"error\": \"%1\" }").arg(msg));
    if (!mSocket)
        deleteLater();
}

/*! \brief helper function for formatting status messages sent to the caller
*/

void
ClientPortCommandHandler::sendStatus(const QString& status, const QString& msg)
{
    rLog(1) << "status: " << msg << qLogEnd;
    sendJson(QString("{ \"status\": \"%1\", \"message\": \"%2\" }").arg(status).arg(msg));
}

/*! \brief send formatted json data
*/

void
ClientPortCommandHandler::sendJson(const QString& json)
{
    QString noCRLFs = json;
    noCRLFs.replace('\n', ' ');

    emit status(noCRLFs);

    if (!mSocket)
        return;
    mSocket->write(QString("%1\n").arg(QString::number(json.length())).toUtf8());
    mSocket->write(json.toUtf8());
    mSocket->flush();
}

/*! \brief static helper function for turning JSON strings into command runners
*/

bool
ClientPortCommandHandler::parseJsonCommand(
    const QString& json,
    P4VCommandRunner& outRunner,
    QString& outError)
{
    Json::ParseResult result = Json::Parser::parse(json);
    if (!result.success)
    {
        outError = result.errorMessage;
        return false;
    }

    Json::Object root = result.value.value<Json::Object>();
    if (root.isEmpty())
    {
        outError = SandstormApp::tr("empty JSON root");
        return false;
    }

    const Json::Object configJson = root["config"].value<Json::Object>();
    if (configJson.isEmpty())
    {
        outError = SandstormApp::tr("missing 'config'");
        return false;
    }

    outRunner.config().setPort(configJson["port"].value<QString>());

    QString charset = configJson["charset"].value<QString>();
    if (!charset.isEmpty())
        outRunner.config().setCharset(charset);

    outRunner.config().setUser(configJson["user"].value<QString>());
    outRunner.config().setClient(configJson["client"].value<QString>());
    if (outRunner.config().port().isEmpty() || outRunner.config().user().isEmpty())
    {
        outError = SandstormApp::tr("incomplete 'config'");
        return false;
    }

    QString command = root["command"].value<QString>();
    if (command.isEmpty())
    {
        outError = SandstormApp::tr("missing 'command'");
        return false;
    }

    QString currentdir = root["currentDir"].value<QString>();

    // file command?
    StringID commandID = P4VCommandLine::FileCommandLookup(command);
    if (!commandID.isNull())
    {
        // file commands requires one or more file/path/revision
        // make paths part of command?
        const Json::Array paths = root["commandArgs"].value<Json::Array>();
        if (paths.isEmpty())
        {
            outError = SandstormApp::tr("missing 'paths'");
            return false;
        }

        outRunner.setMode(P4VCommandRunner::OtherWindow);
        outRunner.setCommand(commandID);
        outRunner.setObjectType(Obj::FILE_TYPE);
        foreach(QVariant v, paths)
        {
            QString path = v.toString();
            if (!path.isEmpty())
            {
                if ( Core::Path::IsDepotPath(path) )
                {
                    outRunner.addTarget(path);
                }
                else
                {
                    if ( QDir::isAbsolutePath(path) )
                    {
                        outRunner.addTarget(path);
                    }
                    else
                    {
                        if ( path.startsWith(".")) 
                            path = path.right( path.length()-1 );
                        if ( path.startsWith(QDir::separator ()) )
                            path = path.right( path.length()-1 );

                        QFileInfo finfo( currentdir + QDir::separator () + path );

                        outRunner.addTarget(finfo.absoluteFilePath());
                    }
                }
            }
        }
        return true;
    }

    // spec command?
    commandID = P4VCommandLine::SpecCommandLookup(command);
    if (!commandID.isNull())
    {
        Json::Array objs = root["commandArgs"].value<Json::Array>();
        foreach(QVariant v, objs)
            if (!v.toString().isEmpty())
                outRunner.addTarget(v.toString());
        outRunner.setObjectType(P4VCommandLine::SpecTypeLookup(command));
        outRunner.setMode(P4VCommandRunner::OtherWindow);
        outRunner.setCommand(commandID);

        return true;
    }

    outError = SandstormApp::tr("unrecognized command");
    return false;
}
