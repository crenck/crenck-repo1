#include "P4VCommandLine.h"

#include "ActionStore.h"
#include "CmdAction.h"
#include "CmdActionGroup.h"
#include "Command.h"
#include "CommandHandler.h"
#include "CommandIds.h"
#include "GuiForm.h"
#include "GuiFeaturesLoader.h"
#include "MainWindow.h"
#include "ObjFormList.h"
#include "ObjObject.h"
#include "ObjPendingChange.h"
#include "ObjStash.h"
#include "PathValidateAgent.h"
#include "Platform.h"
#include "SandstormApp.h"
#include "UITabFloater.h"

#include <QApplication>
#include <QRegExp>
#include <QStringList>

// FileCommandHash
// Every command in the FileCommandHash expects one or more paths
// The type of path is dependent on the command - customizeAction is called to verify.
// We validate file, file revision, and directory paths
static const char* sTree        = "tree";
static const char* sRevGraph    = "revgraph";
static const char* sAnnotate    = "annotate";
static const char* sTimeLapse   = "timelapse";
static const char* sHistory     = "history";
static const char* sPrevDiff    = "prevdiff";
static const char* sDiff        = "diff";
static const char* sDiffDialog  = "diffdialog";
static const char* sProperties  = "properties";
//                              = "submit";                // string in Cmd_Submit
static const char* sOpen        = "open";
//                              = "reconcile";             // string in Cmd_Reconcile
//                              = "syncCustom";            // string in Cmd_SyncCustom
//                              = "labelSync";             // string in Cmd_Labelsync
//                              = "rollback";              // string in Cmd_Rollback
//                              = "changeAttributes";      // string in Cmd_ChangeAttributes
//                              = "findFiles";             // string in Cmd_FindFiles);
//                              = "checkOut";              // string in Cmd_Edit);
//                              = "addFiles";              // string in Cmd_Add);
//                              = "markForDelete";         // string in Cmd_Delete);
//                              = "reopen";                // string in Cmd_Reopen);
//                              = "revert";                // string in Cmd_Revert);
//                              = "revertUnchanged";       // string in Cmd_RevertUnchanged);
//                              = "lock";                  // string in Cmd_Lock);
//                              = "unlock";                // string in Cmd_Unlock);
//                              = "resolve";               // string in Cmd_Resolve);
//                              = "shelve";                // string in Cmd_Shelve);
//                              = "unshelve";              // string in Cmd_Unshelve);
//                              = "merge";                 // string in Cmd_MergeDown);
static const char* sCopy        = "copy";
//                              = "branchFiles";           // string in Cmd_Branch);
//                              = "moveRename";            // string in Cmd_MoveRename);
static const char* sRename     = "rename";

// SpecCommandHash
// The following list commands expect no arguments
static const char* sClients     = "clients";
static const char* sBranches    = "branchmappings";
//                              = "branches";              // string in Cmd_Branches);
//                              = "groups";                // string in Cmd_Groups);
//                              = "jobs";                  // string in Cmd_Jobs);
//                              = "labels";                // string in Cmd_Labels);
//                              = "pendingChangelists";    // string in Cmd_PendingChangelists);
//                              = "submittedChangelists";  // string in Cmd_SubmittedChangelists);
//                              = "users";                 // string in Cmd_Users);
//                              = "workspaces";            // string in Cmd_Workspaces);
//                              = "streams";               // string in Cmd_Streams);
//                              = "remotes";               // string in Cmd_Remotes);

// The following list commands have one argument
static const char* sBranch                = "branchmapping";
static const char* sClient                = "client";
static const char* sGroup                 = "group";
static const char* sJob                   = "job";
static const char* sLabel                 = "label";
static const char* sPendingChangelist     = "pendingChangelist";
static const char* sSubmittedChangelist   = "submittedChangelist";
static const char* sUser                  = "user";
static const char* sWorkspace             = "workspace";
static const char* sStream                = "stream";
static const char* sRemote                = "remote";
// Backout requires a submitted changelist argument
//                              = "backout";               // string in Cmd_BackOut);
// These following commands expect a pending changelist argument
//                              = "submit";                // string in Cmd_Submit);
//                              = "revertUnchanged";       // string in Cmd_RevertUnchanged);
// Show stream graph optionally takes a depot object
//                              = "streamsGraph";          // string in Cmd_StreamGraph);

QHash<QString, StringID>        P4VCommandLine::sFileCommandHash;
QHash<QString, StringID>        P4VCommandLine::sSpecCommandHash;
QHash<QString, Obj::ObjectType> P4VCommandLine::sSpecTypeHash;

void
P4VCommandLine::InitCommandHashes()
{
    if (!sFileCommandHash.isEmpty())
        return;
    QString command;
    sFileCommandHash.insert(sTree,                                Cmd_Tree);
    sFileCommandHash.insert(sRevGraph,                            Cmd_Tree);
    sFileCommandHash.insert(sAnnotate,                            Cmd_Annotate);
    sFileCommandHash.insert(sTimeLapse,                           Cmd_Annotate);
    sFileCommandHash.insert(sHistory,                             Cmd_RevisionHistory);
    sFileCommandHash.insert(sPrevDiff,                            Cmd_DiffAgainstFile);
    sFileCommandHash.insert(sDiff,                                Cmd_DiffAgainstFile);
    sFileCommandHash.insert(sDiffDialog,                          Cmd_DiffDialog);
    sFileCommandHash.insert(sProperties,                          Cmd_FileProperties);
    sFileCommandHash.insert(Cmd_Submit.toString(),                Cmd_Submit);
    sFileCommandHash.insert(sOpen,                                Cmd_MainWindow);
    sFileCommandHash.insert(Cmd_Reconcile.toString(),             Cmd_Reconcile);
    sFileCommandHash.insert(Cmd_SyncCustom.toString(),           Cmd_SyncCustom);
    sFileCommandHash.insert(Cmd_Labelsync.toString(),             Cmd_Labelsync);
    sFileCommandHash.insert(Cmd_Rollback.toString(),              Cmd_Rollback);
    sFileCommandHash.insert(Cmd_ChangeAttributes.toString(),     Cmd_ChangeAttributes);
    sFileCommandHash.insert(Cmd_FindFiles.toString(),            Cmd_FindFiles);
    sFileCommandHash.insert(Cmd_Edit.toString(),                  Cmd_Edit);
    sFileCommandHash.insert(Cmd_Add.toString(),                   Cmd_Add);
    sFileCommandHash.insert(Cmd_Delete.toString(),                Cmd_Delete);
    sFileCommandHash.insert(Cmd_Reopen.toString(),                Cmd_Reopen);
    sFileCommandHash.insert(Cmd_Revert.toString(),                Cmd_Revert);
    sFileCommandHash.insert(Cmd_RevertUnchanged.toString(),      Cmd_RevertUnchanged);
    sFileCommandHash.insert(Cmd_Lock.toString(),                  Cmd_Lock);
    sFileCommandHash.insert(Cmd_Unlock.toString(),                Cmd_Unlock);
    sFileCommandHash.insert(Cmd_Resolve.toString(),               Cmd_Resolve);
    sFileCommandHash.insert(Cmd_Shelve.toString(),                Cmd_Shelve);
    sFileCommandHash.insert(Cmd_Unshelve.toString(),              Cmd_Unshelve);
    sFileCommandHash.insert(Cmd_SubmitShelve.toString(),          Cmd_SubmitShelve);
    sFileCommandHash.insert(Cmd_MergeDown.toString(),            Cmd_MergeDown);
    sFileCommandHash.insert(sCopy,                                Cmd_CopyUp);
    sFileCommandHash.insert(Cmd_Branch.toString(),                Cmd_Branch);
    sFileCommandHash.insert(Cmd_MoveRename.toString(),            Cmd_MoveRename);
    sFileCommandHash.insert(sRename,                              Cmd_MoveRename);

    // spec commands
    sSpecCommandHash.insert(Cmd_Branches.toString(),              Cmd_Branches);
    sSpecCommandHash.insert(sBranches,                            Cmd_Branches);
    sSpecCommandHash.insert(Cmd_Groups.toString(),                Cmd_Groups);
    sSpecCommandHash.insert(Cmd_Jobs.toString(),                  Cmd_Jobs);
    sSpecCommandHash.insert(Cmd_Labels.toString(),                Cmd_Labels);
    sSpecCommandHash.insert(Cmd_PendingChangelists.toString(),   Cmd_PendingChangelists);
    sSpecCommandHash.insert(Cmd_SubmittedChangelists.toString(), Cmd_SubmittedChangelists);
    sSpecCommandHash.insert(Cmd_Users.toString(),                 Cmd_Users);
    sSpecCommandHash.insert(Cmd_Workspaces.toString(),            Cmd_Workspaces);
    sSpecCommandHash.insert(sClients,                             Cmd_Workspaces);
    sSpecCommandHash.insert(Cmd_Streams.toString(),               Cmd_Streams);
    sSpecCommandHash.insert(Cmd_Remotes.toString(),               Cmd_Remotes);

    sSpecCommandHash.insert(sBranch,                              Cmd_OpenSpec);
    sSpecCommandHash.insert(sClient,                              Cmd_OpenSpec);
    sSpecCommandHash.insert(sGroup,                               Cmd_OpenSpec);
    sSpecCommandHash.insert(sJob,                                 Cmd_OpenSpec);
    sSpecCommandHash.insert(sLabel,                               Cmd_OpenSpec);
    sSpecCommandHash.insert(sPendingChangelist,                   Cmd_OpenSpec);
    sSpecCommandHash.insert(sSubmittedChangelist,                 Cmd_OpenSpec);
    sSpecCommandHash.insert(sUser,                                Cmd_OpenSpec);
    sSpecCommandHash.insert(sWorkspace,                           Cmd_OpenSpec);
    sSpecCommandHash.insert(sStream,                              Cmd_OpenSpec);
    sSpecCommandHash.insert(sRemote,                              Cmd_OpenSpec);

    sSpecCommandHash.insert(Cmd_BackOut.toString(),              Cmd_BackOut);
    sSpecCommandHash.insert(Cmd_Submit.toString(),                Cmd_Submit);
    sSpecCommandHash.insert(Cmd_RevertUnchanged.toString(),      Cmd_RevertUnchanged);
    sSpecCommandHash.insert(Cmd_StreamGraph.toString(),          Cmd_StreamGraph);

    sSpecTypeHash.insert(sBranch,                              Obj::BRANCH_TYPE);
    sSpecTypeHash.insert(sClient,                              Obj::CLIENT_TYPE);
    sSpecTypeHash.insert(sGroup,                               Obj::GROUP_TYPE);
    sSpecTypeHash.insert(sJob,                                 Obj::JOB_TYPE);
    sSpecTypeHash.insert(sLabel,                               Obj::LABEL_TYPE);
    sSpecTypeHash.insert(sPendingChangelist,                   Obj::PENDING_CHANGE_TYPE);
    sSpecTypeHash.insert(sSubmittedChangelist,                 Obj::CHANGE_TYPE);
    sSpecTypeHash.insert(sUser,                                Obj::USER_TYPE);
    sSpecTypeHash.insert(sWorkspace,                           Obj::CLIENT_TYPE);
    sSpecTypeHash.insert(sStream,                              Obj::STREAM_TYPE);
    sSpecTypeHash.insert(sRemote,                              Obj::REMOTESPEC_TYPE);

    sSpecTypeHash.insert(Cmd_BackOut.toString(),               Obj::CHANGE_TYPE);
    sSpecTypeHash.insert(Cmd_Submit.toString(),                Obj::PENDING_CHANGE_TYPE);
    sSpecTypeHash.insert(Cmd_RevertUnchanged.toString(),       Obj::PENDING_CHANGE_TYPE);
    sSpecTypeHash.insert(Cmd_StreamGraph.toString(),           Obj::DEPOT_TYPE);
}

// static
StringID P4VCommandLine::FileCommandLookup(const QString& cmd)
{
    InitCommandHashes();
    return sFileCommandHash.value(cmd);
}


StringID P4VCommandLine::SpecCommandLookup(const QString& cmd)
{
    InitCommandHashes();
    return sSpecCommandHash.value(cmd);
}

Obj::ObjectType P4VCommandLine::SpecTypeLookup(const QString& objtype)
{
    InitCommandHashes();
    return (sSpecTypeHash.contains(objtype)) ?
            sSpecTypeHash.value(objtype) : Obj::UNKNOWN_TYPE;
}

P4VCommandLine::P4VCommandLine(QObject* parent)
    : QObject(parent)
    , mFaceless(false)
    , mConsoleLogOption(None)
    , mWinHandle(0)
    , mErrorCode(0)
    , mRunnable(false)
    , mHasCustomStyle(false)
    , mGridOption(0)
    , mUsingEnviroArgs(false)
{
    InitCommandHashes();
}


void
P4VCommandLine::setError(int code)
{
    mRunner.setMode(P4VCommandRunner::Error);
    mErrorCode = code;
}

bool
P4VCommandLine::parseConfigCommand(const QString& option, QStringList* args)
{
    if(!args->size() || args->front()[0] == '-')
    {
        mErrorMessage = SandstormApp::
            tr("Helix P4V: argument required for '%1' option.\n")
                .arg(option);
        setError(1);
        return false;
    }
    QString value = args->front();
    args->pop_front();
    switch(option[1].toLatin1())
    {
    case 'p':   mRunner.config().setPort(value); break;
    case 'u':   mRunner.config().setUser(value); break;
    case 'c':   mRunner.config().setClient(value);   break;
    case 'P':   break; // don't set password but allow scripts to use one
    case 'C':   mRunner.config().setCharset(value);  break;
    }
    return true;
}

bool
P4VCommandLine::parsePathCommand(const QString& /*option*/, QStringList* args)
{
    if(!args->size() || args->front()[0] == '-')
    {
        mErrorMessage = SandstormApp::tr("Helix P4V: argument required for '-s' option.\n");
        setError(1);
        return false;
    }
    QString value = args->front();
    args->pop_front();
    mRunner.setObjectType( Obj::FILE_TYPE );
    mRunner.addTarget( value );
    return true;
}

bool
P4VCommandLine::parseWinHandleCommand(const QString& /*option*/, QStringList* args)
{
    // window handle arg is required; zero is not a valid value
    bool ok = args->size() > 0;
    if(ok)
        mWinHandle = args->front().toInt(&ok);
    if (!ok)
    {
        mErrorMessage = SandstormApp::tr("Helix P4V: missing or invalid windowhandle argument for '-win' option.\n");
        setError(1);
        return false;
    }
    args->pop_front();
    return true;
}


bool
P4VCommandLine::parseCmdCommand(const QString& option, QStringList* args)
{
    // this option provides a general purpose way
    // of launching p4v to show a particular type
    // of view.
    // for now, only tree and annotate are recognized
    // and each requires a path argument
    // eventually, CommandMgr will be able to deal with this.
    // Note, that since this is an undocumented interface, and
    // should only be used by p4win at this time, the error handling
    // is just for our debugging benefit.
    if(!args->size() || args->front().isEmpty())
    {
        mErrorMessage = SandstormApp::tr("Helix P4V: command argument required for '%1' option.\n")
                .arg(option);
        setError(1);
        return false;
    }

    if (isSpecCommand(option, args))
        return true;

    mRunner.setObjectType(Obj::FILE_TYPE);
    return isFileCommand(option, args);
}

bool
P4VCommandLine::isSpecCommand(const QString& option, QStringList* args)
{
    Q_UNUSED(option)
    QString suboption = args->front();

    StringID command = sSpecCommandHash.value(suboption);
    if (command.isNull())
        return false;

    if (command == Cmd_Submit || command == Cmd_RevertUnchanged)
    {
        if (args->count() <= 1)
            return false;
        QString id = args->at(1);
        if (id != Obj::PendingChange::DefaultName())
        {
            bool success;
            id.toInt(&success);
            if (!success)
                return false;
        }
    }

    mRunner.setMode(P4VCommandRunner::OtherWindow);
    mRunner.setCommand(command);

    args->pop_front();
    if (args->isEmpty())
    {
        // TODO: clear mRunner targets?
    }
    else
    {
        mRunner.setObjectType((sSpecTypeHash.contains(suboption)) ?
                               sSpecTypeHash.value(suboption) : Obj::UNKNOWN_TYPE);
        mRunner.addTargets(*args);
        args->clear();
    }
    return true;
}

bool
P4VCommandLine::isFileCommand(const QString& option, QStringList* args)
{
    QString suboption = args->front();
    QStringList subargs;
    // this is a command line parsing correction for job015731. (running tree/annotate from the p4win)
    // It mimics the existing parser behavior for file names containing blank spaces.
    // This fix would fail, if we run tree/annotate on more then one file, which we currently are not doing.
    StringID command = sFileCommandHash.value(suboption);
    if (command.isNull())
    {
        args->pop_front();

        QStringList cmdOptions  = sFileCommandHash.keys();
        QString test;
        foreach (QString cmdOption, cmdOptions)
        {
                // We need to create a test string because starts with will match diff before it matches
                // diffdialog. Since keys of a hash aren't guaranteed ordering, it means sometimes we
                // were hitting diff instead of diffdialog which was messing up our parsing of the file
                // argument. This is the easier fix since even a QMap is ordered by key, which will put
                // diff before diffdialog anyway. Parsing the command plus a space is easier since we
                // know there will be a space between the command and args.
            test = cmdOption + " ";
            if (suboption.startsWith(test))
            {
                subargs += cmdOption;
                subargs += suboption.right(suboption.length()-(cmdOption.length() + 1));
                suboption = cmdOption;
                break;
            }
        }
         // parse the suboption again
        command = sFileCommandHash.value(suboption);
    }
    // this is the original code. (before job015731 fix)
    else
    {
        args->pop_front();
        subargs = suboption.split(" ");
    }

    // require suboption + path, no more and no less.
    if(subargs.size() != 2)
    {
        mErrorMessage = SandstormApp::tr("Helix P4V: unrecognized argument '%1' for '%2' option.\n")
                .arg(suboption).arg(option);
        setError(1);
        return false;
    }
    suboption = subargs.front();
    subargs.pop_front();

    if (command.isNull())
    {
        mErrorMessage = SandstormApp::tr("Helix P4V: unrecognized argument '%1' for '%2' option.\n")
                .arg(suboption).arg(option);
        setError(1);
        return false;
    }

    // validate the path for the command
    if(subargs.front().isEmpty())
    {
        mErrorMessage = SandstormApp::tr("Helix P4V: path argument required for '%1 %2' option.\n")
                .arg(option).arg(suboption);
        setError(1);
        return false;
    }

    mRunner.setMode(P4VCommandRunner::OtherWindow);
    mRunner.setCommand(command);

    // does the first subarg start with a data option?
    QString data = "<none>";
    if (!subargs.isEmpty() && subargs.at(0).startsWith("-"))
    {
        QRegExp optionWithPath("(\\S+)\\s+(\\S+)");
        QString leadArg = subargs.at(0);

        if(optionWithPath.exactMatch(leadArg))
        {
            data = optionWithPath.cap(1);
            subargs[0] = optionWithPath.cap(2);
        }
    }
    mRunner.setData(data);

    foreach(QString arg, subargs)
    {
        QStringList paths = arg.split("<");
        mRunner.addTargets(paths);
    }

    return true;
}

bool
P4VCommandLine::parseTabCommand(const QString& option, QStringList* args)
{
    if(!args->size() || args->front().isEmpty())
    {
        mErrorMessage = SandstormApp::tr("Helix P4V: command argument required for '%1' option.\n")
                .arg(option);
        setError(1);
        return false;
    }

    QString suboption = args->front();
    mRunner.setTab(suboption);
    args->pop_front();

    return true;
}

bool
P4VCommandLine::runningCommandTool()
{
    return ( mRunner.mode() == P4VCommandRunner::OtherWindow );
}

bool
P4VCommandLine::parseCommand(const QString& cmd, const QString& option, QStringList* args)
{
    if (option == "-p" ||
        option == "-u" ||
        option == "-c" ||
        option == "-P" ||
        option == "-C")
    {
        mUsingEnviroArgs = true;
        return parseConfigCommand(option, args);
    }
    if (option == "-faceless")
    {
        Platform::SetApplication(Platform::P4V_App_CommandLineMode);
        mFaceless = true;
        return true;
    }
    if (option == "-s")
    {
        //Platform::SetApplication(Platform::P4V_App_CommandLineMode);
        //mRunnable = true;
        return parsePathCommand(option, args);
    }
    if (option == "-ignorep4vfeatures")
    {
        Gui::FeaturesLoader::setEnabled(false);
        return true;
    }
    if (option == "-log")
    {
        mConsoleLogOption = Partial;
        return true;
    }
    if (option == "-logall")
    {
        mConsoleLogOption = All;
        return true;
    }
#ifdef TEST
    if (option == "-testonly")
    {
        mMode = TestOnly;
        return false;
    }
#endif
    if (option == "-help" ||  option == "-h")
    {
        // for some reason, we don't want to show help info if other
        // args are specified
        if(args->size())
            return true;
        // TODO: do we need a runner for this?
        mRunner.setMode(P4VCommandRunner::ShowHelp);
        return false;
    }
#ifdef Q_OS_WIN
    if (option == "-win")
    {
        return parseWinHandleCommand(option, args);
    }
#endif
    else if (option == "-cmd")
    {
        Platform::SetApplication(Platform::P4V_App_CommandLineMode);
        mRunnable = true;
        return parseCmdCommand(option, args);
    }
    if (option == "-t")
    {
        mRunnable = true;
        return parseTabCommand(option, args);
    }
    if (option == "-style")
    {
        mHasCustomStyle = true;
        return true;
    }
    if (option == "-dg")
    {
        mGridOption = 1;
        return true;
    }
    if (!option.isEmpty())
    {
        mRunner.config() = Op::Configuration::DefaultConfiguration(option);
        if (!mRunner.config().isEmpty())
            return true;

        mErrorMessage = SandstormApp::tr("Helix P4V: unrecognized option '%1'\ntry '%2 -help' for more information.\n")
        .arg(option).arg( !Platform::IsUnix() ? cmd : "p4v" );
        setError(1);
        return false;
    }
    // ignore empty options?
    return true;
}

bool
P4VCommandLine::parse(QStringList& args)
{
    QString arg0;
    if (!args.isEmpty())
        arg0 = args[0];

    // strip of arg0, before we start walking the argument list.
	if (args.size())
        args.pop_front();

    while(args.size())
    {
        QString option = args.front();
        args.pop_front();
        if(!parseCommand(arg0, option, &args))
            return false;
    }

    // running in -faceless? then don't prompt for -p/-u/-c
    if (faceless())
        return true;

    if (!mRunner.config().isEmpty() &&
        (mRunner.config().port().isEmpty() || mRunner.config().user().isEmpty()))
    {
        mRunner.config().clear();
        mErrorMessage = SandstormApp::tr(
                    "Helix P4V: You must specify a port, user, and optionally a client.\n"
                    "    -p port     set p4port\n"
                    "    -u user     set p4user\n"
                    "    -c client   set p4client\n");
        setError(1);
        return false;
    }
    return true;
}


