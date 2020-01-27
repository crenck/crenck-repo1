#ifndef INCLUDED_SWORKSPACE
#define INCLUDED_SWORKSPACE

#include "Workspace.h"

class UIWorkspace2;

namespace Gui
{
class WorkspaceScopeName;
}

class SWorkspace : public Gui::Workspace
{
    Q_OBJECT

public:
    SWorkspace(const Gui::WorkspaceScopeName& scopeName, QWidget* parentWidget);
    virtual ~SWorkspace();

    // Gui::Workspace overrides
    virtual void save();

protected:
    virtual void restore();

};

#endif // INCLUDED_SWORKSPACE
