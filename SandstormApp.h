#ifndef INCLUDED_SANDSTORMAPP
#define INCLUDED_SANDSTORMAPP

#include "Application.h"
#include "OpTypes.h"
#include "RCFServer.h"

#include <QSysInfo>
#include <QVariant>
#include <QSharedPointer>

namespace Obj
{
class Stash;
}

#ifdef Q_OS_OSX
class UIAboutDialog;
#endif

class QAction;
class QEvent;
class QFileOpenEvent;
class QItemEditorFactory;
class ConnectionDef;
class HelpLauncher;
class P4VCommandHandler;
class P4VCommandLine;
class ShortCutCmds;
class UIConfigurationWizard;

#include "P4VHttp.h"

namespace Op
{
class Configuration;
}

class SandstormApp : public Gui::Application
{
    Q_OBJECT
public:
    static SandstormApp*        sApp;

    static bool                 Create                  ();
    static void                 Destroy                 ();

    // overrides from Application object, allows calls from Gui
    bool                        showWorkspace           (const Op::Configuration&,
            bool disconnected = false);
    bool                        checkStatusAndShow      (const Op::Configuration& config,
            const Op::CheckConnectionStatus status);

    Gui::Workspace*             loadWorkspace           (const Op::Configuration& constConfig,
            bool disconnected = false);
    void                        fillInDvcsInfo          (Op::Configuration& config);

    // the user selected a new configuration to
    // open a new workspace

    virtual void                showLoginStatus         (Obj::Stash* stash,
            const QString& info);

    void                        customizeAction         (const StringID& cmd,
            QAction* action);

    virtual void                setupShortcuts          ();
    virtual void                handleShortCut          (const QString& key);
    virtual ShortCutCmds*       shortCutCmds            ();

    virtual bool runCommand                  (const QString& cmd,
            QString& error,
            QObject* cb);

protected:
    SandstormApp            (const QString& configPath,
                             const Gui::LogFormatter& formatter);
    virtual                    ~SandstormApp            ();
    virtual Gui::Workspace*    createWorkspace          (const Gui::WorkspaceScopeName& scopeName);

    void                        processCommandLine      (int argc,
            QStringList argv);
    bool                        loadInstallerConfig     (Op::Configuration&);
    virtual void                installTranslators      ();
    bool                        isTopLevelWidgetOpen    ();

public slots:
    void showAbout          (const QVariant& v = QVariant());
    void showHelp           (const QVariant& v = QVariant());
    void showInfo           (const QVariant& v = QVariant());
    void showWhatsNew       (const QVariant& v = QVariant());
    void quit               (const QVariant& v = QVariant());

    void addConnection      (const QVariant& v = QVariant());
    void manageConnection   (const QVariant& v = QVariant());
    void openConnection     (const QVariant& v = QVariant());
    void connectionWizard   (const QVariant& v = QVariant());
    void newUser            (const QVariant& v = QVariant());
    void showGotoDialog     (const QVariant& v = QVariant());
    void reloadDialog       (const QVariant& v = QVariant());

    void customize          (const QVariant& v = QVariant());
    void htmlActionsManager (const QVariant& v = QVariant());
    void htmlWindowsManager (const QVariant& v = QVariant());
    void htmlTabsManager    (const QVariant& v = QVariant());

    virtual void            openConnectionHandler   (const Op::Configuration& config);
    void                    openConnectionCheckDone (Op::CheckConnectionStatus result);

    void                    onCommandError          (const QString&);
    void                    onCommandInfo           (const QString&);

private slots:
    virtual void            delayedInit             ();
    void                    handleFileOpenEvent     (QFileOpenEvent* e);
    virtual void            reopenApplication       ();
    void                    onLastWindowClosed      ();
    void                    handleAboutToQuit       ();
    void                    onConfigurationChanged  (const Op::Configuration& newC,
            const Op::Configuration& oldC);

private:
    virtual void            init                    ();
    virtual void            loadGlobalActions       ();
    HelpLauncher*           helpWindow              ();
    virtual bool            eventFilter             (QObject* obj, QEvent* e);
    void                    setIsDvcs               (Gui::Workspace* ws);

    bool                    startNewWorkspace       (const Op::Configuration& cfg = Op::Configuration());
    bool                    runConnectionWizard     (bool runforthefirsttime = true);

    void                    connectWorkspaceToRunner(Gui::Workspace* ws);

    bool                    beforeLeopard           ()
    {
        // Version enum only available on actual platform
#ifdef Q_OS_OSX
        return (QSysInfo::MacintoshVersion < QSysInfo::MV_10_5 );
#endif
        return false;
    }
    void                    startConnectionChecker      (Op::Configuration& config);
    bool                    promptConnectNoConnection   (const Op::Configuration& config);

    QString                 determineLocalHelpPath      ();
    void                    showCommandLineError        (const QString&);
    void                    showCommandLineInfo         (const QString&);
    void                    setConfigurationAndWait     (Obj::Stash* stash,
            const Op::Configuration& config);

    P4VCommandHandler*      mCommandHandler;
    HelpLauncher*           mHelpLauncher;
    P4VCommandLine*         mCommandLine;
    bool                    mRunHttp;

#ifdef Q_OS_OSX
    QPointer<UIAboutDialog> mAboutDialog;
#endif

    UIConfigurationWizard*  mConfigWizard;

    ShortCutCmds*           mShortcutCmds;

    QSharedPointer<RCFServer> mRCFServer;
    bool                    mCheckConnectDone;

    QSharedPointer<P4VHttp> mHTTPServer;
};

#endif // INCLUDED_SANDSTORMAPP

