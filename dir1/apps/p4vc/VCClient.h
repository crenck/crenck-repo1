#ifndef VCCLIENT_H_
#define VCCLIENT_H_

#include <QObject>
#include <QHostAddress>
#include <QNetworkSession>
#include <QSharedPointer>
#include <QTcpSocket>
#include <QThread>
#include <QSharedPointer>
#include "P4VCMessage.h"

namespace p4vc
{

class VCClient : public QObject
{
    Q_OBJECT

public:
    // Constructed with message it will send
    VCClient(QSharedPointer<P4VCMessage> message);
    ~VCClient();

    // Initiates connection
    void connectToHost(const QString host = "127.0.0.1", const quint16 port = 7999);

signals:
    // Server is possibly unlaunched
    void connectionRefused(bool macOs105Mode);
    // Timed out
    void timedOut();
    // Quit
    void clientQuit(int exitCode);

private slots:
    // Upon connection, command is sent.
    void sendCommand();
    // Upon network session opening, connect socket
    void sessionOpened();
    // Handle socket error
    void handleSocketError(QAbstractSocket::SocketError error);
    // Handle server messages
    void readyRead();


private:
    // Handles printing of an info message
    void handleInfoMessage(QSharedPointer<P4VCMessage> infoMsg);
    // Handles priting of version information
    void handleVersionMessage(QSharedPointer<P4VCMessage> versMsg);

    void openNetworkSession();

    QTcpSocket mSocket;
    QSharedPointer<QNetworkSession> mSession;

    QHostAddress mHost;
    quint16 mPort;
    quint16 mRetryCount;

    QSharedPointer<P4VCMessage> mpMessage;

    // Flag to indicate initial connection check (is p4v service up?)
    bool mFirstTry;
    // Flag to indicate if we are retrying a MAC0S105 launch instead of default
    bool mMacOs105Try;

    // Convert an integer to a QByteArray
    QByteArray intToArray(qint32 source);
    qint32 arrayToInt(QByteArray source);

    // Utility class to measure p4v retry times on first p4vc run
    class ConnWaiter : public QThread
    {
    public:
        static void msleep(unsigned long msecs) {
            QThread::msleep(msecs);
        }
    };
};

}
#endif
