
#include "AdminCommandHandler.h"
#include "AdminCmd.h"

#if !defined( QT_MODULE_OPENGL ) || defined( QT_LICENSE_PROFESSIONAL )
#define NOTREE
#endif

// included windows
#include "AdminWindow.h"
#include "AdminPasswordDialog.h"
#include "UIDiffDialog.h"
#include "DiffHandler.h"
#include "FolderDiff.h"
#ifndef NOTREE
#include "TreeHandler.h"
#endif
#include "AdminSpecHandler.h"
#include "TLVHandler.h"
#include "UIFilePropertiesDialog.h"
#include "UIFileReopenDialog.h"
#include "UIHistoryViewer.h"
#include "UISpecListHandler.h"
#include "SubmitHandler.h"
// shell
#include "OpenInTerminalHandler.h"
#include "OpenWithHandler.h"
#include "PrintHandler.h"
#include "RevealHandler.h"
#include "RevertFileHandler.h"
// gui
#include "CommandIds.h"
#include "FileCommandHandlers.h"
#include "FormCommandHandlers.h"
#include "RefreshHandler.h"

#include <QString>

AdminCommandHandler::AdminCommandHandler()
{
    registerHandler(Cmd_AdminChangePassword, new AdminPasswordHandler());
    registerHandler(Cmd_RefreshItem, new Gui::RefreshHandler());

    Gui::LockFileHandler* lockFileHandler = new Gui::LockFileHandler();
    registerHandler(Cmd_Lock, lockFileHandler);
    registerHandler(Cmd_Unlock, lockFileHandler);

    Gui::OpenFileHandler* openFileHandler = new Gui::OpenFileHandler();
    registerHandler(Cmd_Add, openFileHandler);
    registerHandler(Cmd_Edit, openFileHandler);
    registerHandler(Cmd_Delete, openFileHandler);
    registerHandler(Cmd_Reopen, openFileHandler);

    registerHandler(Cmd_DeleteSpec, new Gui::DeleteSpecHandler());
    registerHandler(Cmd_PendingRemoveJobs, new Gui::RemoveAllJobsHandler());
    registerHandler(Cmd_RemoveFromChange, new Gui::RemoveJobsHandler());

    registerHandler(Cmd_RevertUnchanged, new Gui::RevertFileHandler());
    registerHandler(Cmd_Revert, new Gui::RevertFileHandler());

    registerHandler(Cmd_FileProperties, new FilePropertiesHandler());
    registerHandler(Cmd_ChangeAttributes, new FileReopenHandler());

    AdminSpecHandler* specWindowHandler = new AdminSpecHandler();
    registerHandler(Cmd_OpenSpec, specWindowHandler);
    registerHandler(Cmd_EditSpec, specWindowHandler);
    registerHandler(Cmd_NewSpec, specWindowHandler);
    registerHandler(Cmd_NewSpecTemplate, specWindowHandler);
    registerHandler(Cmd_FileRevisionViewChange, specWindowHandler);

    registerHandler(Cmd_Submit, new SubmitHandler());
    registerHandler(Cmd_RevisionHistory, new HistoryViewerHandler());
    registerHandler(Cmd_Annotate, new TLVHandler());

    DiffHandler* diffHandler = new DiffHandler();
    registerHandler(Cmd_Diff, diffHandler);
    registerHandler(Cmd_DiffDialog, diffHandler);
    registerHandler(Cmd_DiffAgainstFile, diffHandler);
    registerHandler(Cmd_DiffAgainstWorkspace, diffHandler);

    registerHandler(Cmd_OpenFile, new Gui::OpenFileHandler());
    registerHandler(Cmd_OpenWithApp, new Gui::OpenFileHandler());
    registerHandler(Cmd_ChooseApplication, new Gui::OpenWithHandler());
    registerHandler(Cmd_PendingChangelists, new SpecListHandler());
    registerHandler(Cmd_Print, new Gui::PrintHandler());
    registerHandler(Cmd_PrintPreview, new Gui::PrintHandler());
    registerHandler(Cmd_RevealFile, new Gui::RevealHandler());
    registerHandler(Cmd_OpenTerminal, new Gui::OpenInTerminalHandler());
#ifndef NOTREE
    registerHandler(Cmd_Tree, new TreeHandler());
#endif
}

AdminCommandHandler::~AdminCommandHandler()
{
}

