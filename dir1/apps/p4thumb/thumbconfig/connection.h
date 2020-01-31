#ifndef CONNECTION_THUMB__H
#define CONNECTION_THUMB__H

#include <QJsonObject>
#include <QString>

namespace Thumb
{

class Connection
{
public:
    Connection();

    QString port() const;
    void setPort(const QString& port);

    QString user() const;
    void setUser(const QString& user);

    QString client() const;
    void setClient(const QString& client);

    void read(const QJsonObject& json);
    void write(QJsonObject& json) const;
private:
    QString mPort;
    QString mUser;
    QString mClient;
};

}

#endif // CONNECTION_THUMB__H
