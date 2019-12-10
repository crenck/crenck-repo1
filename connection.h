#ifndef __Op_Connection_h__
#define __Op_Connection_h__

#include "OpConfiguration.h"
#include "OpKeepAlive.h"
#include "OpServerProtocol.h"
#include "StringTranslator.h"

#include <QObject>

// for debugging only
// #define CONNECTION_DEBUG_PRINT

class ClientApi;
class ClientUser;
class Error;

class TestConnection;

namespace Op
{

class Connection;

class ConnectionState
{
public:
    ConnectionState()
        : mICharset(-1)
    {}

    bool setFrom(const ConnectionState& state);

    Configuration   configuration() const;
    bool            setConfiguration(const Configuration& configuration);

    void            setIsDvcsForConfig(bool isDvcs);

    QString         rootDirectory() const;
    void            setRootDirectory(const QString& rootDir);

    ServerProtocol  protocol() const;
    void            setProtocol(const ServerProtocol& protocol);

    int             charsetIndex() const;
private:
    int             mICharset;
    ServerProtocol  mProtocol;
    Configuration   mConfiguration;
    QString         mRootDir;
};

class Connection : public QObject, public StringTranslator
{
    Q_OBJECT
public:
    Connection(QObject* parent = 0);
    ~Connection();

    ConnectionState state() const;
    void            setIsLocked(const bool locked)
    {
        mIsLocked = locked;
    }
    virtual void    setState(const ConnectionState& state);
    void            setBaseProtocol(ServerProtocol protocol);
    void            setBaseState(const ConnectionState& state);

    virtual bool    run     (const QString& command,
                             const QStringList& args,
                             bool isTagged,
                             int maxLockTime,
                             int maxResults,
                             int maxScanRows,
                             ClientUser* user,
                             const QString& programName,
                             bool p4authCommand,
                             Error* e);
    virtual void    cancel  (bool byUser = true);
    virtual void    dropConnection ();
    virtual bool    lock    (const QString& programName,
                             const QString& command,
                             Error* e);
    virtual bool    unlock  (bool p4authCommand,
                             Error* e);

    ClientApi*              api()
    {
        return mApi;
    }

signals:
    void    connected      ();
    void    disconnected   ();
    void    unicodeEnabled (bool enabled);

private:
    void    createConnection(Error* e);
    bool    checkIfDropped(Error* e);
    bool    wasCancelledByUser() const;

    ConnectionState     mState;
    ClientApi*          mApi;
    KeepAlive           mKeepAlive;
    bool                mIsLocked;
    bool                mWasCancelledByUser;

    friend class ::TestConnection;
};

}

Q_DECLARE_METATYPE(Op::Connection*)

#endif // __Op_Connection_h__
