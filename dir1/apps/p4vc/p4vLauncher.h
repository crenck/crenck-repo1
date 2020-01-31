#ifndef P4VLAUNCHER_H__
#define P4VLAUNCHER_H__

#include "P4VCMessage.h"
#include "VCClient.h"
#include <QCoreApplication>
#include <QSharedPointer>
#include <string>

namespace p4vc
{

class P4VLauncher : public QCoreApplication
{

Q_OBJECT

public:
    P4VLauncher(int argc, char *argv[], QSharedPointer<P4VCMessage> message);
    ~P4VLauncher();

private slots:
    void retryConnection(bool macOs105Mode);
    void connectionTimeout();
    void connect();
    void clientQuit(int exitCode);

private:
    bool autoLaunch();
    bool launch(const std::string& utf8P4vPath);
    void connect(const std::string& utf8P4vPath, unsigned short port);

    bool mMacOS105Mode;
    VCClient mClient;
    unsigned short mPort;
    std::string mLaunchPath;
};

}

#endif
