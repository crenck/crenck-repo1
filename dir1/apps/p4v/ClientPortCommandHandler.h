//
//    ClientPortCommandHandler.h - class for handling port based communication
//                                 and dispatching
//

#include <QObject>
#include <QString>
#include <QTcpSocket>

#include <memory>

namespace Gui
{
    class Workspace;
}

class QTcpSocket;

class P4VCommandRunner;
class P4VCommandLine;

class ClientPortCommandHandler : public QObject
{
    Q_OBJECT

public:
    ClientPortCommandHandler(P4VCommandLine*);
    ClientPortCommandHandler(const P4VCommandRunner&);
    ClientPortCommandHandler(int port);
    virtual ~ClientPortCommandHandler();

signals:
    void    status(const QString&);

protected:
    void    startClient                     (int port);

    std::unique_ptr<P4VCommandLine>     mCommandLine;
    std::unique_ptr<P4VCommandRunner>   mCurrentCommand;
    QTcpSocket*                         mSocket;
    Gui::Workspace*                     mWorkspace;

protected slots:
    // for the tcp socket
    void    onConnected                     ();
    void    onDisconnected                  ();
    void    onError                         ( QAbstractSocket::SocketError );
    void    onStateChanged                  ( QAbstractSocket::SocketState socketState );
    void    onReadyRead                     ();

    // slots for the command runner
    void    onCommandError                  (const QString&);
    void    onCommandInfo                   (const QString&);
    void    onWindowActivated               (QWidget*);
    void    onAccepted                      ();
    void    onRejected                      ();
    void    onWindowDestroyed               (QObject*);

public:
    static bool parseJsonCommand            (const QString& json, P4VCommandRunner& outRunner, QString& error);

protected:
    int     readBlockLength                 ();
    void    handleCommand                   (const P4VCommandRunner& runner);

    void    sendError                       (const QString& msg);
    void    sendStatus                      (const QString& status, const QString& msg);
    void    sendHandshake                   ();
    void    sendJson                        (const QString& json);
};
