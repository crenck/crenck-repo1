#include "connection.h"

namespace Thumb
{

const char* PORT = "port";
const char* USER = "user";
const char* CLIENT = "client";

Connection::Connection()
{
}

QString Connection::port() const
{
    return mPort;
}

void Connection::setPort(const QString& port)
{
    mPort = port;
}

QString Connection::user() const
{
    return mUser;
}

void Connection::setUser(const QString& user)
{
    mUser = user;
}

QString Connection::client() const
{
    return mClient;
}

void Connection::setClient(const QString& client)
{
    mClient = client;
}

void Connection::read(const QJsonObject& json)
{
    mPort = json[PORT].toString();
    mUser = json[USER].toString();
    mClient = json[CLIENT].toString();
}

void Connection::write(QJsonObject& json) const
{
    json[PORT] = mPort;
    json[USER] = mUser;
    json[CLIENT] = mClient;
}

}
