#include "VCClient.h"

#include <QtNetwork>
#include <iostream>

namespace p4vc
{

VCClient::VCClient(QSharedPointer<P4VCMessage> message) :
    mSocket(this), mPort(0), mRetryCount(45), mFirstTry(true), mMacOs105Try(false), mpMessage(message)
{

#if defined(Q_OS_OSX)
    // If MAC, we try only 15 times, if nothing then try again in MACOS10 mode 30 times
    mRetryCount = 15;
#endif

    connect(&mSocket, &QTcpSocket::connected, this, &VCClient::sendCommand);
    connect(&mSocket, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error),
        this, &VCClient::handleSocketError);
    connect(&mSocket, &QTcpSocket::readyRead, this, &VCClient::readyRead);
}

VCClient::~VCClient()
{
    mSocket.close();
}

void VCClient::connectToHost(const QString host, const quint16 port)
{
    mHost = QHostAddress(host);
    mPort = quint16(port);

    // Start our network session
    openNetworkSession();
}

void VCClient::handleSocketError(QAbstractSocket::SocketError error)
{
    switch(error)
    {
    case QAbstractSocket::ConnectionRefusedError:
        {
            // First try check
            if(mFirstTry)
            {
                mFirstTry = false;
                emit connectionRefused(mMacOs105Try);
            }
            // Connection is refused (usually because p4v isn't accepting connections yet)
            else if(--mRetryCount)
            {
                // Retry connection every second or so for x amount of times
                ConnWaiter::msleep(1000);
                sessionOpened();
            }
            else
            {
#if defined(Q_OS_OSX)
                // If MAC and reaches here, retry in OS105 launch mode instead and try again
                if(!mMacOs105Try)
                {
                    // Reset retry values
                    mMacOs105Try = true;
                    mFirstTry = true;
                    mRetryCount = 30;

                    // Try again
                    openNetworkSession();
                }
                else // reach here means we've tried launching Mac P4V both ways, timed out
                    emit timedOut();
#else
                // Timeout, killing p4vc
                emit timedOut();
#endif
            }
        }
        break;
    }
}

void VCClient::sendCommand()
{
    // Now connected, double check message isn't malformed and send
    if(!(mpMessage->isMalformed()) &&
        mSocket.state() == QAbstractSocket::ConnectedState)
    {
        // Write out the size of our message then the actual data
        // TODO: Error handling (PRETTY MUCH EVERYWHERE)
        QByteArray dataToWrite = mpMessage->serializeMessage();
        mSocket.write(intToArray(dataToWrite.size()));
        mSocket.write(dataToWrite);
    }
    else
    {
        std::cout << "Message couldn't be sent due to being malformed...\n";
        emit clientQuit(-1);
    }
}

void VCClient::sessionOpened()
{
    // Connect to our host which should have been started by launcher by now
    mSocket.connectToHost(mHost, mPort);
}

void VCClient::openNetworkSession()
{
    // Pretty much lifted wholesale from Qt's client example
    QNetworkConfigurationManager manager;
    if(manager.capabilities() & QNetworkConfigurationManager::NetworkSessionRequired)
    {
        // Get saved network configuration
        QSettings settings(QSettings::UserScope, QLatin1String("P4VC"));
        settings.beginGroup(QLatin1String("P4VCClient"));
        const QString id = settings.value(QLatin1String("DefaultNetworkConfiguration")).toString();
        settings.endGroup();

        // If the saved network configuration is not currently discovered use the system default
        QNetworkConfiguration config = manager.configurationFromIdentifier(id);
        if ((config.state() & QNetworkConfiguration::Discovered) != QNetworkConfiguration::Discovered) 
        {
            config = manager.defaultConfiguration();
        }

        mSession.reset(new QNetworkSession(config, this));
        connect(mSession.data(), static_cast<void (QNetworkSession::*)(void)>(&QNetworkSession::opened),
            this, &VCClient::sessionOpened);

        mSession->open();
    }
    else
    {
        sessionOpened();
    }
}

void VCClient::readyRead()
{
    QByteArray buffer;
    qint32 size(0);

    buffer.append(mSocket.readAll());

    // Simple buffer read
    while(buffer.size() >= 4)
    {
        size = arrayToInt(buffer.mid(0,4));
        buffer.remove(0,4);

        if (size > 0 && buffer.size() >= size)
        {
            QByteArray message = buffer.mid(0, size);
            buffer.remove(0, size);

            // Create the message from the JSON array
            QSharedPointer<P4VCMessage> serverMessage(new P4VCMessage(message));

            if(serverMessage->getCommand() == Message)
                handleInfoMessage(serverMessage);
            else if(serverMessage->getCommand() == Version)
                handleVersionMessage(serverMessage);

            // Otherwise who cares amirite!?!
        }
    }


}

void VCClient::handleInfoMessage(QSharedPointer<P4VCMessage> infoMsg)
{
    QString info = infoMsg->getFieldVariant(InfoMessageField).toString();

    if(info != DefaultField)
        if(info == "received")
            emit clientQuit(0);
        else
            std::cout << info.toStdString() << '\n';
}

void VCClient::handleVersionMessage(QSharedPointer<P4VCMessage> versMsg)
{
    QString version = versMsg->getFieldVariant(VersionField).toString();

    std::cout << '\n' << "Helix P4V Version: " << version.toStdString() << '\n';
    std::cout << "Helix P4VC Version: " << QString("%1/%2/%3").arg(ID_OS, ID_REL, ID_PATCH).toStdString() << '\n';
    emit clientQuit(0);
}

QByteArray VCClient::intToArray(qint32 source)
{
    QByteArray temp;
    QDataStream data(&temp, QIODevice::ReadWrite);
    data << source;
    return temp;
}

qint32 VCClient::arrayToInt(QByteArray source)
{
    qint32 temp;
    QDataStream data(&source, QIODevice::ReadWrite);
    data >> temp;
    return temp;
}

}
