#ifndef __AdminShellApplication_h__
#define __AdminShellApplication_h__

#include "Application.h"

/*! \brief Just enough Gui::Application to satisfy calls to global dApp
           that we cannot otherwise easily squelch.
*/
class AdminShellApplication : public Gui::Application
{
Q_OBJECT

signals:
    void emitHandleMessage(const QString &message);

public slots:
    void openConnectionHandler (const Op::Configuration & config);

public:
                                AdminShellApplication(const QString & configPath,
                                                      const Gui::LogFormatter & fmt);
    virtual bool                showWorkspace       (const Op::Configuration &, bool);
    virtual Gui::Workspace *  createWorkspace     (const Gui::WorkspaceScopeName &scopeName);
    virtual Gui::Workspace*   findWorkspace       (const Op::Configuration& config,
                                                    bool disconnected=false);
    bool                      checkStatusAndShow  (const Op::Configuration& config,
                                                    const Op::CheckConnectionStatus status);

// ...promote to public methods...
    virtual void                loadGlobalActions   ()
        { Gui::Application::loadGlobalActions(); }
    virtual bool    eventFilter(QObject *obj, QEvent *e)
        { return Gui::Application::eventFilter(obj, e); }
// ...end promote...
};

#endif
