#include "AdminShellApplication.h"

#include "DialogDontShow.h"
#include "GuiConnectionChecker.h"
#include "ObjStash.h"
#include "OpConfiguration.h"
#include "OpContext.h"
#include "P4AdminApplication.h"
#include "Platform.h"
#include "Prefs.h"
#include "SettingKeys.h"
#include "Workspace.h"
#include "WorkspaceId.h"
#include "WorkspaceManager.h"
#include "WorkspaceScopeName.h"

#include <QString>

/*!
    \brief Admin's Gui::Application satisfies p4api calls to dApp.
    Also, used to manage the Gui::Workspace for each Configuration.
*/
AdminShellApplication::AdminShellApplication(
    const QString & configPath,
    const Gui::LogFormatter & fmt)
    : Gui::Application(configPath, fmt)
{
    setAppIcon(QPixmap(":/admin_icon.png"));
        // note this is contrary to the default value for this preference
    Prefs::SetValue(Prefs::FQ(p4v::prefs::LogVerbose), true);
}

bool
AdminShellApplication::showWorkspace(const Op::Configuration & config, bool)
{
    P4AdminApplication* qa = (P4AdminApplication*)qApp;
    qa->handleMessage(config.serializedString());
    return true;
}

/*!
    Construct a Workspace for the provided subdir
*/
Gui::Workspace *
AdminShellApplication::createWorkspace(const Gui::WorkspaceScopeName &scopename)
{
    return new Gui::Workspace(scopename);
}


/*!
    Use "user@server" as key for workspaceMap to allow for shared ws prefs.
*/
Gui::Workspace*
AdminShellApplication::findWorkspace(const Op::Configuration& config, bool disconnected)
{
    Gui::WorkspaceScopeName scopeName(config);
    if (scopeName.isEmpty())
        return NULL;

        // Check if we've already got it in our workspace map
    Gui::WorkspaceId workspaceId(config);
    Gui::Workspace* workspace = Gui::WorkspaceManager::Instance()->findWorkspace(workspaceId);

        // create a workspace if not already in the map
    if (!workspace)
        workspace = createWorkspace(scopeName);

        // add to workspace map if successfully inited
    if(!workspace->isValid())
        workspace->init(config, disconnected);
    
    if(workspace->isValid())
        Gui::WorkspaceManager::Instance()->insertWorkspace(workspaceId, workspace);

    return workspace;
}


//-----------------------------------------------------------------------------
//  openConnection
//-----------------------------------------------------------------------------

void
AdminShellApplication::openConnectionHandler (const Op::Configuration & config)
{
    emit emitHandleMessage(config.adminString());
}

bool
AdminShellApplication::checkStatusAndShow(const Op::Configuration& config,
const Op::CheckConnectionStatus status)
{
    return true;
}
