#ifndef __P4AdminApplication_H__
#define __P4AdminApplication_H__

#include <QtSingleApplication>
#include "Responder.h"

#include <QDialog>
#include <QSysInfo>
#include <QVariant>

#include "AdminShellApplication.h"
#include "HelpLauncher.h"
#include "OpConfiguration.h"


class QString;
class AdminCommandHandler;
class AdminWindow;

namespace Gui
{
    class LogFormatter;
    class Workspace;
};



class P4AdminApplication : public QtSingleApplication, public Responder
{
    Q_OBJECT

public:
    P4AdminApplication (int &argc, char **argv);
    ~P4AdminApplication ();

    // must call this method to launch the AdminWindow with a config
    bool run();

    void loadGlobalActions();

    HelpLauncher * helpWindow();

//    virtual bool notify(QObject *obj, QEvent *e);

    void customizeAction( const StringID & cmd, QAction * a);

    AdminWindow*    adminWindow()   { return mAdminWindow; }
protected:
    void createAdminWindow(const Op::Configuration& config);

public slots:
    void handleMessage(const QString &message);
    void setPrefHelpLocation(QString helpDir);
    void adminOpenConnection( const QVariant & );
    void showHelp    (const QVariant &);
    void showPrefs   (const QVariant &);
    void showInfo    (const QVariant &);
    void close       (const QVariant &);
    void quit        (const QVariant &);
    void adminAbout  (const QVariant &);
    void refreshAll  (const QVariant &);
    void maximizeWindow(const QVariant &);
    void minimizeWindow(const QVariant &);
    void handleRefreshData();
    void onPreferencesChanged();

signals:
    void stopRefreshTimer();

private:
    void initFileLogging();  // reads saved prefs
    bool eventFilter(QObject* obj, QEvent* e);
    bool allWindowsClosed();
    void startRefreshTimer(int rate);

private:
    HelpLauncher *          mHelpLauncher;
    Gui::Workspace *      mWorkspace;
    AdminShellApplication*  mAdminShellApp;
    AdminWindow*            mAdminWindow;
    AdminCommandHandler*    mAdminCommandHandler;

    bool                    mInRefreshData;
    int                     mRefreshRate;

    QString realApplicationDirPath();

    bool beforeLeopard()
    {
        #ifdef Q_OS_OSX
            // Version enum only available on actual platform
            return (QSysInfo::MacintoshVersion < QSysInfo::MV_10_5 );
        #endif
        return false;
    }
};


#endif // __P4AdminApplication_H__


