#include "SWorkspace.h"

#include "GuiWindowManager.h"
#include "Platform.h"
#include "UIWorkspace2.h"
#include "WorkspaceScopeName.h"

#include <QWidget>

SWorkspace::SWorkspace(const Gui::WorkspaceScopeName& scopeName,
                       QWidget *parentWindow)
: Gui::Workspace(scopeName, parentWindow)
{
}

SWorkspace::~SWorkspace()
{
}

void
SWorkspace::save()
{
    if (Platform::RunningInP4V())
       Gui::Workspace::save();

    if (!isValid())
        return;

    // save workspace settings
    UIWorkspace2 *ws = dynamic_cast<UIWorkspace2*>(windowManager()->mainWorkspaceWindow());
    if (ws)
        ws->saveSettings();
}

void
SWorkspace::restore()
{
    Gui::Workspace::restore();
    UIWorkspace2 *ws = dynamic_cast<UIWorkspace2*>(windowManager()->mainWorkspaceWindow());
    if (ws)
        ws->restoreSettings();
}

