// Provides the QTcpServer implementation initialized during
// a p4v -rcf call.  Intended to handle commands from the
// p4vc client contained in apps/p4vc

#ifndef RCFServer_h
#define RCFServer_h

#include "OpConfiguration.h"
#include "P4VCMessage.h"
#include "P4VCMessageSubscriber.h"

#include <QTcpServer>
#include <QThread>
#include <QSharedPointer>
#include <QString>

namespace Gui
{
    class Workspace;
}

class RCFServer : public QTcpServer, 
                  public p4vc::P4VCMessageSubscriber
{
    Q_OBJECT

public:
    RCFServer(QObject* parent);

    // Shutdown will be taken care of in the destructor
    // as I particularly don't care if it blows up on the way out
    ~RCFServer();

    // Defaulting address to localhost and port to 7999 as that's what we need 
    // for our purposes 100% of the time right now
    bool init(const QString& address = "127.0.0.1", const quint16 port = 7999);

    void handleP4VCMessage(QSharedPointer<p4vc::P4VCMessage> commandMessage,
                           Gui::Workspace* workspace);

public slots:
    void newConnection();
    void readyRead();
    void readyToQuit();

private:
    // Message to ack messages received to the client
    QSharedPointer<p4vc::P4VCMessage> mReceivedMsg;

    void sendMessage(QSharedPointer<p4vc::P4VCMessage> message);
    void shutdownSocket();
    qint32 arrayToInt(QByteArray source);
    Op::Configuration initConfig(const QString& server, const QString& user, const QString& charset,
                                 const QString& workspace);
    Gui::Workspace* createInvisibleWorkspace(const QString& workspace, Op::Configuration& config);
    Gui::Workspace* createWorkspace(QSharedPointer<p4vc::P4VCMessage> messageToSend);
    QByteArray intToArray(qint32 source);

    QList<p4vc::Command> getSubscribedMessageTypes();

    QHostAddress mAddress;
    quint16 mPort;
    QTcpSocket* mSocket;
};

#endif
