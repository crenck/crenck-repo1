#include "P4VCommandHandler.h"

#include "AddToWorkspaceHandler.h"
#include "BookmarkEditorCaller.h"
#include "ChangeOwnerHandler.h"
#include "CommandIds.h"
#include "DefaultTreeView.h"
#include "DiffHandler.h"
#include "DvcsCloneProxyHandler.h"
#include "DvcsCommandHandlers.h"
#include "DvcsFetchProxyHandler.h"
#include "DvcsInitProxyHandler.h"
#include "DvcsPushProxyHandler.h"
#include "DvcsSwitchProxyHandler.h"
#include "FileCommandHandlers.h"
#include "FormCommandHandlers.h"
#include "IntegrateHandler.h"
#include "NewDirectoryDialog.h" // for NewDirectoryHandler
#include "OpenInTerminalHandler.h"
#include "OpenLocalFileHandler.h"
#include "OpenWithHandler.h"
#include "Platform.h"
#include "PreferenceHandler.h"
#include "PrintHandler.h"
#include "ReconcileHandler.h"
#include "RefreshHandler.h"
#include "ReloadHandler.h"
#include "RemoteDialog.h"  // for NewRemoteHandler
#include "ResolveHandler.h"
#include "RevealHandler.h"
#include "RevertFileHandler.h"
#include "RollbackWindow.h"
#include "SaveDepotFileHandler.h"
#include "ShelveHandler.h"
#include "SpecWindowHandler.h"
#include "StreamGraphWidget.h"
#include "StreamPopulateHandler.h"
#include "SubmitHandler.h"
#include "SwarmHandler.h"
#include "SwitchToHandler.h"
#include "TLVHandler.h"
#include "UIFileFinder.h"
#include "UIFilePropertiesDialog.h"
#include "UIFileReopenDialog.h"
#include "UIFileViewer.h"
#include "UIGotoWindow.h"
#include "UIHistoryViewer.h"
#include "UILabelSyncWindow.h"
#include "UIMoveRenameDialog.h"
#include "UISpecListHandler.h"
#include "UISyncWindow.h"
#include "UIWorkspace2.h"
#include "UnloadHandler.h"

#ifndef NOADMIN
#include "AdminLaunchHandler.h"
#endif

#ifndef NOTREE
#include "TreeHandler.h"
#endif

#include <QString>

// When adding a command which is global and has a handler
// use DefaultTreeView::IgnoreCommandInViews if you don't want the views to recognize it

P4VCommandHandler::P4VCommandHandler()
{
    registerHandler(Cmd_Prefs, new Gui::PreferenceHandler());

    registerHandler(Cmd_RefreshItem, new Gui::RefreshHandler());

    Gui::LockFileHandler* lockFileHandler = new Gui::LockFileHandler();
    registerHandler(Cmd_Lock, lockFileHandler);
    registerHandler(Cmd_Unlock, lockFileHandler);

    Gui::OpenFileHandler* openFileHandler = new Gui::OpenFileHandler();
    registerHandler(Cmd_Edit, openFileHandler);
    registerHandler(Cmd_EditOpen, openFileHandler);
    registerHandler(Cmd_Add, openFileHandler);
    registerHandler(Cmd_Delete, openFileHandler);
    registerHandler(Cmd_Reopen, openFileHandler);

    registerHandler(Cmd_SaveDepotFile, new Gui::SaveDepotFileHandler());
    Gui::SyncFileHandler* syncFileHandler = new Gui::SyncFileHandler();
    registerHandler(Cmd_SyncHead, syncFileHandler);
    registerHandler(Cmd_RemoveFromWorkspace, syncFileHandler);
    registerHandler(Cmd_FileRevisionSync, syncFileHandler);

    registerHandler(Cmd_DeleteFile, new Gui::DeleteFileHandler());

    registerHandler(Cmd_Rollback, new RollbackHandler());
    registerHandler(Cmd_BackOut, new Gui::BackOutHandler());
    registerHandler(Cmd_P4Undo, new UndoHandler());

    registerHandler(Cmd_DeleteSpec, new Gui::DeleteSpecHandler());
    registerHandler(Cmd_SwitchTo, new SwitchToHandler());
	registerHandler(Cmd_SwitchTo, new Gui::DvcsSwitchProxyHandler());

    registerHandler(Cmd_ChangePendingOwner, new Gui::ChangeOwnerHandler());

    registerHandler(Cmd_PendingRemoveJobs, new Gui::RemoveAllJobsHandler());
    registerHandler(Cmd_RemoveFromChange, new Gui::RemoveJobsHandler());

    registerHandler(Cmd_RevertUnchanged, new Gui::RevertFileHandler());
    registerHandler(Cmd_Revert, new Gui::RevertFileHandler());

    // p4v specific
    registerHandler(Cmd_GotoDialog, new GotoWindowHandler());
    registerHandler(Cmd_SyncCustom, new CustomSyncHandler());
    registerHandler(Cmd_Labelsync, new LabelSyncHandler());
    DiffHandler* diffHandler = new DiffHandler();

    registerHandler(Cmd_Diff,                   diffHandler);
    registerHandler(Cmd_DiffAgainstFile,      diffHandler);
    registerHandler(Cmd_DiffAgainstHead,      diffHandler);
    registerHandler(Cmd_DiffAgainstWorkspace, diffHandler);
    registerHandler(Cmd_DiffDialog,            diffHandler);
    registerHandler(Cmd_DiffBranch,            diffHandler);
    registerHandler(Cmd_DiffStream,            diffHandler);
    registerHandler(Cmd_ReconcileFD,           diffHandler);

    Obj::ReconcileHandler* reconcileHandler = new Obj::ReconcileHandler();
    registerHandler(Cmd_Reconcile,              reconcileHandler);
    registerHandler(Cmd_CleanWorkspace,         reconcileHandler);

    registerHandler(Cmd_ChangeAttributes, new FileReopenHandler());

    registerHandler(Cmd_Annotate, new TLVHandler());
    registerHandler(Cmd_StreamGraph, new StreamGraphHandler());

    Gui::ShelveHandler* shelveHandler = new Gui::ShelveHandler();
    registerHandler(Cmd_Shelve, shelveHandler);
    registerHandler(Cmd_Unshelve, shelveHandler);
    registerHandler(Cmd_DeleteShelve, shelveHandler);
    registerHandler(Cmd_PromoteShelve, shelveHandler);

    SwarmHandler* swarmHandler = new SwarmHandler();
    registerHandler(Cmd_SwarmRequestReview, swarmHandler);
    registerHandler(Cmd_SwarmOpenReview, swarmHandler);
    registerHandler(Cmd_SwarmOpenChangelist, swarmHandler);
    registerHandler(Cmd_SwarmUpdateReview, swarmHandler);
    registerHandler(Cmd_SwarmAddToReview, swarmHandler);


#ifndef NOADMIN
    registerHandler(Cmd_Admin, new AdminLaunchHandler());
#endif
    ResolveHandler* resolveHandler = new ResolveHandler();
    registerHandler(Cmd_Resolve, resolveHandler);
    registerHandler(Cmd_ResolveFiles, resolveHandler);
    registerHandler(Cmd_ReResolve, resolveHandler);
    registerHandler(Cmd_ReResolveFiles, resolveHandler);

    Gui::OpenLocalFileHandler* openHandler = new Gui::OpenLocalFileHandler();
    registerHandler(Cmd_OpenFile, openHandler);
    registerHandler(Cmd_OpenWithApp, openHandler);
    registerHandler(Cmd_ChooseApplication, new Gui::OpenWithHandler());
    registerHandler(Cmd_RevealFile, new Gui::RevealHandler());
    registerHandler(Cmd_OpenTerminal, new Gui::OpenInTerminalHandler());

    registerHandler(Cmd_FileProperties, new FilePropertiesHandler());

    registerHandler(Cmd_MoveRename, new MoveRenameHandler());
    registerHandler(Cmd_Submit, new SubmitHandler());
    registerHandler(Cmd_SubmitShelve, new SubmitShelveHandler());
    registerHandler(Cmd_Print, new Gui::PrintHandler());
    registerHandler(Cmd_PrintPreview, new Gui::PrintHandler());
    registerHandler(Cmd_PrintPdf, new Gui::PrintHandler());

    Gui::NewDirectoryHandler* newSpecHandler = new Gui::NewDirectoryHandler();
    registerHandler(Cmd_NewFilespec, newSpecHandler);
    registerHandler(Cmd_NewDirectory, newSpecHandler);
#ifndef NOTREE
    registerHandler(Cmd_Tree, new TreeHandler());
#endif
    BookmarkEditorCaller* bookmarkEditorHandler = new BookmarkEditorCaller();
    registerHandler(Cmd_AddBookmark, bookmarkEditorHandler);
    registerHandler(Cmd_AddBookmark2, bookmarkEditorHandler);

    registerHandler(Cmd_AddToWorkspace, new AddToWorkspaceHandler());

    SpecWindowHandler* specWindowHandler = new SpecWindowHandler();
    registerHandler(Cmd_OpenSpec, specWindowHandler);
    registerHandler(Cmd_EditSpec, specWindowHandler);
    registerHandler(Cmd_NewSpec, specWindowHandler);
	registerHandler(Cmd_NewSpec, new Gui::DvcsSwitchProxyHandler());
    registerHandler(Cmd_NewSpecTemplate, specWindowHandler);
    registerHandler(Cmd_FileRevisionViewChange, specWindowHandler);
    registerHandler(Cmd_CreateStream, specWindowHandler);
	registerHandler(Cmd_CreateStream, new Gui::DvcsSwitchProxyHandler());

    // these handlers are backups if not shown in the main window
    registerHandler(Cmd_RevisionHistory, new HistoryViewerHandler());
    registerHandler(Cmd_FileDetails, new FileViewerHandler());
    registerHandler(Cmd_FindFiles, new FileFinderHandler());

    SpecListHandler* specListHandler = new SpecListHandler();
    registerHandler(Cmd_SubmittedChangelists, specListHandler);
    registerHandler(Cmd_PendingChangelists, specListHandler);
    registerHandler(Cmd_Users, specListHandler);
    registerHandler(Cmd_Workspaces, specListHandler);
    registerHandler(Cmd_Branches, specListHandler);
    registerHandler(Cmd_Labels, specListHandler);
    registerHandler(Cmd_Jobs, specListHandler);
    registerHandler(Cmd_Groups, specListHandler);
    registerHandler(Cmd_Streams, specListHandler);
    registerHandler(Cmd_Remotes, specListHandler);
    registerHandler(Cmd_CreateRemote, new NewRemoteHandler());

    MainWindowHandler* mainHandler = new MainWindowHandler();
    registerHandler(Cmd_MainWindow, mainHandler);

    IntegrateHandler* integratehandler = new IntegrateHandler();
    registerHandler(Cmd_MergeDown, integratehandler);
    registerHandler(Cmd_CopyUp, integratehandler);
    registerHandler(Cmd_Branch, integratehandler);
    registerHandler(Cmd_Branch, new Gui::DvcsSwitchProxyHandler());
    registerHandler(Cmd_IntegrateDragAndDrop, integratehandler);

    StreamPopulateHandler * pStreamPopulateHandler;
    pStreamPopulateHandler = new StreamPopulateHandler ();
    registerHandler (Cmd_StreamPopulate, pStreamPopulateHandler);

    registerHandler(Cmd_Unload, new UnloadHandler());
    ReloadHandler * reloadHandler = new ReloadHandler();
    registerHandler(Cmd_Reload, reloadHandler);

    // DVCS commands
    registerHandler(Cmd_DvcsInit, new Gui::DvcsInitProxyHandler());

    // handle both context menu and toolbar clone cmd originations
    Gui::DvcsCloneProxyHandler* pDvcsCloneProxyHandler;
    pDvcsCloneProxyHandler = new Gui::DvcsCloneProxyHandler();
    registerHandler(Cmd_DvcsClone, pDvcsCloneProxyHandler);         // toolbar
    registerHandler(Cmd_DvcsCloneContext, pDvcsCloneProxyHandler);  // context menu

    registerHandler(Cmd_DvcsFetch, new Gui::DvcsFetchProxyHandler());
    registerHandler(Cmd_DvcsPush, new Gui::DvcsPushProxyHandler());

    // These are all commands which have handlers, but should not be handled by
    // DefaultTreeView and its subclasses
    Gui::DefaultTreeView::IgnoreCommandInViews(Cmd_Admin);
    Gui::DefaultTreeView::IgnoreCommandInViews(Cmd_GotoDialog);
}

P4VCommandHandler::~P4VCommandHandler()
{
}

