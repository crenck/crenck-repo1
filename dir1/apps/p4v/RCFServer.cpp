#include "RCFServer.h"

#include "ObjFile.h"
#include "ObjFileRevision.h"
#include "ObjStash.h"
#include "P4VCMessageBroker.h"
#include "P4VCMessage.h"
#include "Platform.h"
#include "Workspace.h"
#include "WorkspaceManager.h"

#include <QCoreApplication>
#include <QTcpSocket>

RCFServer::RCFServer(QObject* parent) : 
    QTcpServer(parent), mPort(0)
{
    // Create received message to send
    mReceivedMsg.reset(new p4vc::P4VCMessage(p4vc::Message));
    mReceivedMsg->setFieldString(QString("received"), p4vc::InfoMessageField);

    connect(this, &QTcpServer::newConnection, this, &RCFServer::newConnection);
    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &RCFServer::readyToQuit);
    subscribeToMessageTypes();
}

RCFServer::~RCFServer()
{
    shutdownSocket();
}

void RCFServer::shutdownSocket()
{
    if(mSocket)
    {
        mSocket->disconnect();
        mSocket = NULL;
    }
}

QList<p4vc::Command> RCFServer::getSubscribedMessageTypes()
{
    static QList<p4vc::Command> sMessageTypes;

    if(sMessageTypes.size() == 0)
    {
        sMessageTypes.push_back(p4vc::Shutdown);
        sMessageTypes.push_back(p4vc::Message);
        sMessageTypes.push_back(p4vc::Version);
    }

    return sMessageTypes;
}

void RCFServer::handleP4VCMessage(QSharedPointer<p4vc::P4VCMessage> commandMessage,
                    Gui::Workspace* workspace)
{
    Q_UNUSED(workspace);

    p4vc::Command command = commandMessage->getCommand();

    if(command == p4vc::Message)
        sendMessage(commandMessage);
    else if(command == p4vc::Shutdown)
        QCoreApplication::quit();
    else if(command == p4vc::Version)
    {
        QSharedPointer<p4vc::P4VCMessage> versMess(new p4vc::P4VCMessage(p4vc::Version));
        versMess->setFieldString(Platform::LongApplicationNameAndVersion(), p4vc::VersionField);
        sendMessage(versMess);
    }

}

bool RCFServer::init(const QString& address, const quint16 port)
{
    mAddress = QHostAddress(address);
    mPort = port;

    // Success, yes? No? Yes???!
    return listen(QHostAddress(address),port);
}

// QCoreApplication has signaled it's about to shut down, send a receieved message
// to let the client application know to kill itself, if its there
void RCFServer::readyToQuit()
{
    // Let client know we've received our message (or some data), wait for write
    // or we'll kill before message is sent and VC will stay open
    sendMessage(mReceivedMsg);
    mSocket->waitForBytesWritten();
    shutdownSocket();
}

void RCFServer::newConnection()
{
    while(hasPendingConnections())
    {
        mSocket = nextPendingConnection();
        connect(mSocket, &QTcpSocket::readyRead, this, &RCFServer::readyRead);
    }
}

void RCFServer::readyRead()
{
    QByteArray buffer;
    qint32 size(0);

    buffer.append(mSocket->readAll());

    // Simple buffer read
    if(buffer.size() >= 4)
    {
        size = arrayToInt(buffer.mid(0,4));
        buffer.remove(0,4);

        if (size > 0 && buffer.size() >= size) 
        {
            QByteArray message = buffer.mid(0, size);
            buffer.remove(0, size);

            // Create the message from the JSON array
            QSharedPointer<p4vc::P4VCMessage> messageToSend(new p4vc::P4VCMessage(message));

            if(messageToSend)
            {
                if(messageToSend->getCommand() == p4vc::Version || messageToSend->getCommand() == p4vc::Message)
                    p4vc::P4VCMessageBroker::getInstance().deliverMessage(messageToSend, NULL);
                else
                {
                    Gui::Workspace* pWorkspace(createWorkspace(messageToSend));

                    if(pWorkspace)
                        p4vc::P4VCMessageBroker::getInstance().deliverMessage(messageToSend, pWorkspace);
                }
            }
        }
    }

    // Let client know we've received our message (or some data)
    sendMessage(mReceivedMsg);
}

qint32 RCFServer::arrayToInt(QByteArray source)
{
    qint32 temp;
    QDataStream data(&source, QIODevice::ReadWrite);
    data >> temp;
    return temp;
}

Op::Configuration RCFServer::initConfig(const QString& server, const QString& user, const QString& charset,
                                        const QString& workspace)
{
    Op::Configuration config;
    config.setPort(server);
    config.setUser(user);
    config.setCharset(charset);
    config.setClient(workspace);

    return config;
}

Gui::Workspace* RCFServer::createInvisibleWorkspace(
                                                    const QString& workspace, Op::Configuration& config)
{
    if(!workspace.isEmpty())
        config.setClient(workspace.toUtf8());

    Gui::Workspace* pWorkspace(Gui::WorkspaceManager::Instance()->createInvisibleWorkspace(config));

    if(pWorkspace)
    {
        Gui::WorkspaceId workspaceId(config);
        Gui::WorkspaceManager::Instance()->insertWorkspace(workspaceId, pWorkspace);
    }

    return pWorkspace;
}

Gui::Workspace* RCFServer::createWorkspace(QSharedPointer<p4vc::P4VCMessage> messageToSend)
{
    // Use message data to create workspace and configs
    QString server = messageToSend->getFieldVariant(p4vc::ServerField).toString();
    QString user = messageToSend->getFieldVariant(p4vc::UserField).toString();
    QString charset = messageToSend->getFieldVariant(p4vc::CharsetField).toString();
    QString workspace = messageToSend->getFieldVariant(p4vc::WorkspaceField).toString();

    if(workspace == p4vc::DefaultField)
        workspace = QString();

    Op::Configuration config = initConfig(server, user, charset, workspace);

    Gui::WorkspaceId workspaceId(config);
    Gui::Workspace* pWorkspace(Gui::WorkspaceManager::Instance()->findWorkspace(Gui::WorkspaceId(workspaceId)));

    if(!pWorkspace)
    {
        pWorkspace = createInvisibleWorkspace(workspace, config);
    }

    return pWorkspace;
}

void RCFServer::sendMessage(QSharedPointer<p4vc::P4VCMessage> message)
{
    QByteArray dataToWrite(message->serializeMessage());
    mSocket->write(intToArray(dataToWrite.size()));
    mSocket->write(dataToWrite);
}

QByteArray RCFServer::intToArray(qint32 source)
{
    QByteArray temp;
    QDataStream data(&temp, QIODevice::ReadWrite);
    data << source;
    return temp;
}
