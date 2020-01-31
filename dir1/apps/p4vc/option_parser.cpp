#include "option_parser.h"

#include "clientapi.h"
#include "options.h"
#include "i18napi.h"
#include "charcvt.h"

#ifdef Q_OS_WIN
#  include "direct.h"
#else
#  include <unistd.h>
#endif

#include <algorithm>
#include <enviro.h>
#include <iostream>
#include <vector>

#include <QString>
#include <QVector>

ErrorId usage = { ErrorOf( 0, 0, 0, 0, 0), "p4vc help for usage." };
ErrorId command_usage = { ErrorOf( 0, 0, 0, 0, 0), "p4vc help <command> for usage." };

//-----------------------------------------------------------------------------
//  isColon (static)
//-----------------------------------------------------------------------------

static bool
isColon (char ch)
{
    return ch == ':';
}

//-----------------------------------------------------------------------------
//  parse
//
//  parses the command line arguments
//  e.g. port, client user and arguments
//  uses p4 server code to help
//
//  this gets called from LaunchApp main
//-----------------------------------------------------------------------------

static std::string getCommandCharset(Options& opts)
{
    Enviro enviro;

    StrPtr *s;
    if ((s = opts[ 'Q' ]))
        return s->Text();

    const char* lc;
    if (lc = enviro.Get( "P4COMMANDCHARSET"))
        return lc;


    if ((s = opts[ 'l' ]) || (s = opts[ 'C' ]))
        return s->Text();

    if (lc = enviro.Get( "P4CHARSET"))
        return lc;

    return "none";
}

namespace p4vc
{

//-----------------------------------------------------------------------------
// OptionParser
//-----------------------------------------------------------------------------

OptionParser::OptionParser()
{
    mNeedsInit = true;
    mToken     = NULL;
    mTmpString = NULL;
}

//-----------------------------------------------------------------------------
// ~OptionParser
//-----------------------------------------------------------------------------

OptionParser::~OptionParser()
{
    delete mTmpString;
}

bool OptionParser::parse(int argc, char** argv)
{
    ClientApi    client;
    Error        e;
    Options      opts;

    int          count = argc - 1;
    char **      vars;

    char        localeCwd[4096];

    StrPtr      localeCharset;
    StrPtr      localeHost;
    StrPtr      localePort;
    StrPtr      localeUser;

    char *      pTmpHost      = NULL;
    char *      pTmpPort      = NULL;
    char *      pTmpUser      = NULL;

    char *      host          = NULL;
    char *      port          = NULL;
    char *      user          = NULL;
    char *      cwd           = NULL;

    char *      translatedArgs[20];

    client.Init (&e);

                    // apparently the StrPtr does not set the char * buffer to NULL,
                    // so can't use strPtr.Text() and test for NULL.  Instead we will
                    // test this with StrPtr::Lengh().  Ask me how much fun this was
                    // to figure this out.

    char** csvars = argv + 1;
    int cscount = argc - 1;

    // No arguments were passed besides p4vc.exe
    if(cscount == 0)
    {
        std::cout << "No arguments were passed to p4vc...\n";
        return false;
    }

    opts.Parse(cscount, csvars, "p:u:C:c:Q:", OPT_ANY, usage, &e);

    std::string charset = getCommandCharset(opts);

    localeHost = client.GetClientNoHost();
    if (localeHost.Length () != 0)
        pTmpHost = localeHost.Text ();

    localePort = client.GetPort();
    if (localePort.Length () != 0)
        pTmpPort = localePort.Text ();

    localeUser = client.GetUser();
        pTmpUser = localeUser.Text ();

    getcwd (localeCwd, 4096);

    if ((charset != "none") &&
        (convertLocaleToUtf8 (argc, argv,
                              translatedArgs,
                              charset.c_str(),
                              pTmpHost,
                              &host,
                              pTmpPort,
                              &port,
                              pTmpUser,
                              &user,
                              localeCwd,
                              &cwd)))
    {
        vars = translatedArgs + 1;
    }
    else
    {
        host = pTmpHost;
        port = pTmpPort;
        user = pTmpUser;
        cwd  = localeCwd;
        vars = argv + 1;
    }
                // version and short-help options
    opts.Parse (count, vars, "Vh?", OPT_ONE, usage, &e);

    StrPtr* help1 = opts.GetValue('h', 0);
    StrPtr* help2 = opts.GetValue('?', 0);
    if (help1 || help2 || argc == 1)
    {
        mCommand = "short-help";
        std::cout << "\np4vc [options] command [arg ...]\n\n"
        << "Options:\n"
        << "\t-h -?           print this message\n"
        << "\t-V              print client version\n"
        << "\t-c client       set client name (default $P4CLIENT)\n"
        << "\t-C charset      set character set (default $P4CHARSET)\n"
        << "\t-p port         set server port (default $P4PORT)\n"
        << "\t-u user         set user's username (default $P4USER)\n\n"
        << "Try 'p4vc help' for information on Perforce commands.\n";
        return false;
    }

    StrPtr* version = opts.GetValue('V', 0);
    if (version)
    {
        // Request P4V version from P4V, wait for version back
        mCommand = Version.toStdString();
        mpMessage.reset(new P4VCMessage(Version));
        return true;
    }

                // port, user and client/workspace and charset
    opts.Parse(count, vars, "p:u:C:c:Q:", OPT_ANY, usage, &e);

    if (count == 0)
    {
        std::cout << "No command was passed to p4vc...\n";
        return false;
    }
        //RERRMSG(p4::missing, "there is no command specified");

                // thinking it's a command e.g. tlv, revisiongraph
                // if we find it in the array of commands then parse the command
    mCommand = *vars;
    std::transform(mCommand.begin(), mCommand.end(), mCommand.begin(), ::tolower);
    ++vars; --count;

    // Check if this is a valid command
    if(!P4VCMessage::GetValidCommands().contains(QString(mCommand.c_str())))
    {
        std::cout << "Unknown command or option flag passed.\np4vc help for usage.\nInvalid command: " << mCommand.c_str();
        return false;
    }

    // TODO: Break this up to be held in the actual message (class code, not message data)
    if(mCommand == "help" && count == 0)
    {
        std::cout << "\n\n"
                  << "help             Print the requested help message\n"
                  << "branchmappings   Shows list of branch mappings\n"
                  << "branches         Same as branchmappings. Shows list of branch mappings\n"
                  << "diff             Shows diff dialog\n"
                  << "groups           Shows list of groups\n"
                  << "branch           Show/Edit branch\n"
                  << "change           Show/Edit change\n"
                  << "client           Same as workspace. Show/Edit workspace\n"
                  << "workspace        Show/Edit workspace\n"
                  << "depot            Show/Edit depot\n"
                  << "group            Show/Edit group\n"
                  << "job              Show/Edit job\n"
                  << "label            Show/Edit label\n"
                  << "user             Show/Edit user\n"
                  << "jobs             Shows list of jobs\n"
                  << "labels           Shows list of labels\n"
                  << "pendingchanges   Shows list of pending changes\n"
                  << "resolve          Launches resolve dialog\n"
                  << "revisiongraph    Shows revision graph\n"
                  << "revgraph         Same as revisiongraph. Shows revision graph\n"
                  << "streamgraph      Shows stream graph pane\n"
                  << "streams          Shows list of streams\n"
                  << "submit           Launches submit dialog\n"
                  << "submittedchanges Shows list of submitted changes\n"
                  << "timelapse        Shows timelapse view\n"
                  << "timelapseview    Same as timelapse. Shows timelapse view\n"
                  << "tlv              Same as timelapse. Shows timelapse view\n"
                  << "users            Shows list of users\n"
                  << "workspaces       Shows list of workspaces\n"
                  << "clients          Same as workspaces. Shows list of workspaces\n"
                  << "shutdown         Shut downs the p4v service\n\n";
        return false;
    }
    else if(mCommand == "help")
    {
        //Count is more than 0, so may possibly be a command help        
        --count;
        parseHelpCommand(vars);
        // Always just return false here to kill the program
        return false;
    }

    // Create message based on command
    // Validates whether it's a valid command as well
    mpMessage.reset(new P4VCMessage(Command(mCommand.c_str())));

    // If invalid, message will be null
    if(!mpMessage)
    {
        std::cout << "Some command or command option was invalid...\n";
        return false;
    }

    // Set connection/workspace/server/charset/user values in message
    // First set of ifs, not sure why there but leaving cause that's how it was and works

   mpMessage->setFieldString(QString(charset.c_str()), CharsetField);

    if (host)
        mpMessage->setFieldString(QString(host), WorkspaceField);
    if (port)
        mpMessage->setFieldString(QString(port), ServerField);
    if (user)
        mpMessage->setFieldString(QString(user), UserField);

    mpMessage->setFieldString(QString(cwd), WorkingDirectoryField);

    StrPtr* pPort = opts.GetValue('p', 0);
    if (pPort)
        mpMessage->setFieldString(QString(pPort->Text()), ServerField);

    StrPtr* pUser = opts.GetValue('u', 0);
    if (pUser)
        mpMessage->setFieldString(QString(pUser->Text()), UserField);

    StrPtr* pCharset = opts.GetValue('C', 0);
    if (pCharset)
        mpMessage->setFieldString(QString(pCharset->Text()), CharsetField);

    StrPtr* pWorkspace = opts.GetValue('c', 0);
    bForsedWorkspace = pWorkspace != NULL;
    if (pWorkspace)
        mpMessage->setFieldString(QString(pWorkspace->Text()), WorkspaceField);

    return parseCommand (count, vars);
}

//-----------------------------------------------------------------------------
//  parseCommand
//
//  the command was found in the array of real commands
//  let's cull any relevant arguments and create a message to send over to
//  the p4v service
//
//  ** help is handled a little differently because of the type and number of
//  arguments
//
//  the simplest way to get the translated string was to have P4V translate the
//  string for all of the help and send it back to p4vc - then have p4vc break
//  up the translated string to display what the user is requesting.
//
//  the other option is to set up the proto buffers to handle an enum type
//  and command for help and then have the p4v service just respond with the
//  translated string for the type of help request.
//
//  not doing that right now because I am just learning about the proto buffers
//  and want to get this out quickly for the release -
//  if you want to change it to fit the proto paradigm - have it.
//-----------------------------------------------------------------------------

void OptionParser::parseHelpCommand(char** vars)
{
    // Find out if extra var is actually a command name, if so, output its syntax
    QString command = *vars;
    if(P4VCMessage::GetValidCommands().contains(command))
    {
        P4VCMessage commandHelp(command);
        std::cout << commandHelp;
    }
    else
    {
        std::cout << "Invalid help command request...\n";
    }
}

bool OptionParser::parseCommand (int argc, char** argv)
{
    // Required data was originally picked out and stuffed into protobuff message here
    // removed since all messages will carry this information if required.
    
    // Check fields to see if any required fields are missing based on message prototype
    // There's alot of me assuming going on here, not a good interface for just anyone to use
    // currently. Should be more general ~*fartz*~

    // These components are in every message, so not going to check if they exist
    if(mpMessage->isMalformed(CharsetField))
    {
        std::cout << "Required charset is missing...\n";
        return false;
    }
    if(mpMessage->isMalformed(WorkspaceField))
    {
        std::cout << "Required workspace is missing...\n";
        return false;
    }
    if(mpMessage->isMalformed(ServerField))
    {
        std::cout << "Required server is missing...\n";
        return false;
    }
    if(mpMessage->isMalformed(UserField))
    {
        std::cout << "Required user is missing...\n";
        return false;
    }

    // Not all messages have WorkingDirs, and not all of them are required    
    if(mpMessage->getFieldVariant(WorkingDirectoryField).isValid() && 
        mpMessage->isMalformed(WorkingDirectoryField))
    {
        std::cout << "Required working directory is missing...\n";
        return false;
    }

    // This check was in the case a workspace was specified and wasn't required
    // I don't particularly care at the moment, if it wasn't required it will just be ignored
    /*bool                                workspaceTaken   = false;

    if (!workspaceTaken && bForsedWorkspace)
        RERRMSG (p4::unexpected, "workspace (client) should not be specified for this command");*/

    if(!parseCommandArguments (argc, argv))
        return false;

    if(!parseFreeField        (argc, argv))
        return false;

    return true;
}

//-----------------------------------------------------------------------------
// parseCommandArguments
//-----------------------------------------------------------------------------

bool OptionParser::parseCommandArguments (int & argc, char **& argv)
{
    std::string flags;
    flags += "c:";
    flags += "f";

    StrPtr * pOption = NULL;
    Error    e;
    Options  opts;

    opts.Parse (argc, argv, flags.c_str(), OPT_ANY, usage, &e);

    pOption = opts.GetValue ('f', 0);

    // TODO: Add an explicit "exists" call for fields (this hacky isMalformed only works cause they're optional)
    // Do these fields exist, if so we look for a flag
    if(mpMessage->fieldExists(ForcedField))
    {
        if (pOption)
            mpMessage->setFieldString("true", ForcedField);
        else
            mpMessage->setFieldString("false", ForcedField);
    }

    pOption = NULL;
    pOption = opts.GetValue ('c', 0);

    if (pOption && mpMessage->fieldExists(ChangelistField))
    {
        const char* value = pOption->Text();
        // primitive check is the string is unsigned number
        for (const char* p = value; *p != 0; ++p)
            if (strchr("0123456789", *p) == NULL)
            {
                std::cout << "Changelist id must be a number...\n";
                //RERRMSG(p4::invalidArgument, "changelist id must be a number");
                return false;
            }

        mpMessage->setFieldString(QString(value), ChangelistField);
    }

    return true;
}

//-----------------------------------------------------------------------------
//   What is a free field???  Ahh it's the args at the end like filename,
//   depot path, spec name etc.
//-----------------------------------------------------------------------------

bool OptionParser::parseFreeField (int& argc, char**& argv)
{
    // There shouldn't be any free fields if message doesn't need them
    if (mpMessage->isMalformed(SpecField) && mpMessage->isMalformed(PathField))
    {
        if (argc > 0)
        {
            std::cout << "Unexpected argument at the end of the command line...\n";
            return false;
        }

        return true;
    }


    if(argc == 0)
        return true;

    // Removed for now, going to try sending error back from P4V handler to get more resolution on errors
    // required argument, check to make sure that it's not missing
    /*if (argc == 0)
    {
        if (argsField->isMalformed())
        {
            std::cout << "Required argument is missing at the end of the command line...\n";
            return false;
        }

        return true;
    }*/

    //Parse rest of arguments and add by super roundabout way to QVariantList
    QStringList args;

    while(argc)
    {
        args.push_back(QString(*argv));
        --argc;
        ++argv;
    }

    // Assign to correct field (spec or paths)
    if(!mpMessage->isMalformed(SpecField))
        mpMessage->setFieldList(args, SpecField);
    else if(!mpMessage->isMalformed(PathField))
        mpMessage->setFieldList(args, PathField);

    return true;
}

//-----------------------------------------------------------------------------
//  parseServer
//-----------------------------------------------------------------------------

//void OptionParser::parseServer(const std::string& server, services::protocol::objects::ServerRef* pServer)
//{
//    pServer->set_magic_string(server);
//    return p4::ok;
//}

//-----------------------------------------------------------------------------
// initField
//-----------------------------------------------------------------------------

//void OptionParser::initField(const char* value)
//{
//        if (pFieldDescriptor->message_type() == services::protocol::objects::DepotRef::descriptor())
//        {
//            services::protocol::objects::DepotRef* depotField = (services::protocol::objects::DepotRef*)
//                    mMessage->GetReflection()->
//                    MutableMessage(mMessage.get(), pFieldDescriptor);
//            depotField->set_name(value);
//            return p4::ok;
//        }
//
//        if (pFieldDescriptor->message_type() == services::protocol::objects::ChangeListRef::descriptor())
//        {
//            
//            return p4::ok;
//        }
//
//    return p4::unexpected;
//}

//-----------------------------------------------------------------------------
//  parseHelpCommand
//
//  figures out what kind of help the user is requesting
//  e.g.
//  help
//  help commands
//  help <command>
//
//  sends general help request to p4v service for the translated help string
//  the entire translated string comes back here and then gets processed to
//  display what the user requested
//-----------------------------------------------------------------------------

//void OptionParser::parseHelpCommand (int& argc, char**& argv)
//{
//    if (argc == 0)
//    {
//        mHelpType = Help_General;
//        return p4::ok;
//    }
//
//    if (argc > 1)
//        RERRMSG(p4::tooMany, "help expects only 1 argument");
//
//            // get the command for the help we're trying to get
//    mHelpCommand = argv[0];
//    std::transform (mHelpCommand.begin(), mHelpCommand.end(), mHelpCommand.begin(), ::tolower);
//
//            // convert into a request for all of the commands
//    if (mHelpCommand == "commands")
//    {
//        mHelpType = Help_Commands;
//        return p4::ok;
//    }
//
//            // convert into a request for help for a specific command
//    bool knownCommand = false;
//    for (int i = 0; i < sizeof(commands) / sizeof(commands[0]); ++i)
//    {
//        if (mHelpCommand == commands[i].command)
//        {
//            knownCommand = true;
//            mHelpType = Help_Command;
//            break;
//        }
//    }
//
//    if (!knownCommand)
//        RERRMSG(p4::unknown, "unknown command");
//
//    return p4::ok;
//}


//-----------------------------------------------------------------------------
//  convertLocaleToUtf8
//
//-----------------------------------------------------------------------------

bool OptionParser::convertLocaleToUtf8 (int argc,
                                   char ** argv,
                                   char ** translatedArgs,
                                   const char * charset,
                                   char * localeHost,
                                   char ** host,
                                   char * localePort,
                                   char ** port,
                                   char * localeUser,
                                   char ** user,
                                   char * localeCwd,
                                   char ** cwd)
{
    int          i;
    int          retlen    = 0;
    int          inputLen  = 0;
    char *       output;
    CharSetCvt * cvt;

                // deal with charset conversion to utf8 of the command line args
                // and environment variables because that's what P4V service
                // is expecting
    CharSetApi::CharSet csLocale  = CharSetApi::Lookup (charset);
    CharSetApi::CharSet csUtf8    = CharSetApi::Lookup ("utf8");

    if (csLocale == -1 || csLocale == CharSetApi::NOCONV ||csUtf8 == -1)
        return false;

    cvt = CharSetCvt::FindCvt (csLocale, csUtf8);

            // convert the environment client host
    if (localeHost)
    {
        inputLen = strlen (localeHost);
        *host     = cvt->CvtBuffer (localeHost, inputLen, &retlen);
    }

            // convert and set the environment client port
    if (localePort)
    {
        inputLen = strlen (localePort);
        *port     = cvt->CvtBuffer (localePort, inputLen, &retlen);
    }

            // convert the environment client user
    if (localeUser)
    {
        inputLen = strlen (localeUser);
        *user     = cvt->CvtBuffer (localeUser, inputLen, &retlen);
    }

            // convert the current working directory
    if (localeCwd)
    {
        inputLen = strlen (localeCwd);
        *cwd     = cvt->CvtBuffer (localeCwd, inputLen, &retlen);
    }

            // convert the command line args
    char * tmpOut;

    for (i = 0; i < argc; ++i)
    {
        inputLen = strlen (argv [i]);
        tmpOut = argv[i];
        output = cvt->CvtBuffer (argv[i], inputLen, &retlen);
        translatedArgs[i] = output;
    }

    return true;
}

//-----------------------------------------------------------------------------
// printHelpCommands
//-----------------------------------------------------------------------------

//void OptionParser::printHelpCommands (const std::string & translatedStr)
//{
//    int             maxLen = 0;
//    int             len    = 0;
//    bool            ok     = false;
//    std::string     commandName;
//    std::string     description;
//
//    mTranslatedString.assign (translatedStr);
//    ok = nextPair (commandName, description);
//
//                // loop thru the commands to get the length of the longest
//                // command for formatting
//    while (ok)
//    {
//        if (commandName == "opt_options")
//            break;
//
//        len = commandName.length ();
//        if (maxLen < len)
//            maxLen = len;
//
//        ok = nextPair (commandName, description);
//
//    }
//
//    mNeedsInit  = true;
//    mToken      = NULL;
//    commandName.clear ();
//
//    ok = nextPair (commandName, description);
//
//    while (ok)
//    {
//        if (commandName == "opt_options")
//            break;
//
//        std::cout << std::setw (8)
//                  <<  " "
//                  << std::setw (maxLen)
//                  << setiosflags (std::ios_base::left)
//                  << commandName
//                  << " "
//                  << description
//                  << std::endl;
//
//        ok = nextPair (commandName, description);
//    }
//}

//-----------------------------------------------------------------------------
// nextPair
//-----------------------------------------------------------------------------

//bool OptionParser::nextPair (std::string & key, std::string & value)
//{
//    int             len;
//    char *          keyDel   = ":";
//    char *          valueDel = ",";
//
//    if (!mNeedsInit && mToken)
//    {
//        mToken = strtok (NULL, keyDel);
//    }
//    else
//    {
//                // put the translated line into a char * so that we can
//                // split it into tokens using strtok
//        len        = mTranslatedString.length ();
//        mTmpString = new char [len + 1];
//        strcpy (mTmpString, mTranslatedString.c_str ());
//
//        mNeedsInit  = false;
//        mToken      = strtok (mTmpString, keyDel);
//    }
//
//    if (!mToken)
//    {
//        mNeedsInit = false;
//        return false;
//    }
//    else
//    {
//        key    = mToken;
//        mToken = strtok (NULL, valueDel);
//        value  = mToken;
//    }
//
//    return true;
//}

//-----------------------------------------------------------------------------
// printHelpCommand
//-----------------------------------------------------------------------------

//void OptionParser::printHelpCommand (const std::string& commandName,
//                                const std::string & translatedStr)
//{
//    int             i;
//    bool            ok;
//    bool            knownCommand       = false;
//    bool            optionsNeeded      = false;
//    bool            workspaceNeeded    = false;
//    bool            bForse             = false;
//    bool            bChangeList        = false;
//    std::string     arguments;
//    std::string     desc;
//    std::string     cmd;
//
//    mTranslatedString.assign (translatedStr);
//
//    ok = nextPair (cmd, desc);
//    if (!ok)
//        return;
//
//                // get the translated command name and description
//    while (ok)
//    {
//        if (commandName == cmd)
//        {
//            knownCommand = true;
//            std::cout << "    " << cmd << " -- " << desc << std::endl << std::endl;
//            break;
//        }
//
//        ok = nextPair (cmd, desc);
//    }
//
//                // cycle thru this list to figure out if it has arguments and options
//    knownCommand = false;
//    for (i = 0; i < sizeof (commands) / sizeof (commands[0]); ++i)
//    {
//        if (commandName == commands[i].command)
//        {
//            knownCommand = true;
//            break;
//        }
//    }
//
//    if (!knownCommand)
//        return;
//
//    const Command & command = commands[i];
//
//    if (!command.pDescriptor)
//        return;
//
//                    // init required fields in the Message
//    for (i = 0; i < command.pDescriptor->field_count(); ++i)
//    {
//        const google::protobuf::FieldDescriptor* pFieldDescriptor = command.pDescriptor->field(i);
//
//        if (pFieldDescriptor->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE)
//        {
//                    // if it's a workspace
//            if (pFieldDescriptor->message_type() == services::protocol::objects::WorkspaceRef::descriptor())
//                workspaceNeeded = true;
//
//                    // if it's a connection
//            else if (pFieldDescriptor->message_type() == services::protocol::objects::ConnectionRef::descriptor())
//                optionsNeeded = true;
//
//                    // changelist
//            else if (pFieldDescriptor->message_type() == services::protocol::objects::ChangeListRef::descriptor())
//            {
//                arguments += " [ -c num ]";
//                bChangeList = true;
//            }
//        }
//        else if (pFieldDescriptor->type() == google::protobuf::FieldDescriptor::TYPE_BOOL)
//        {
//            if (pFieldDescriptor->name() == "force")
//            {
//                arguments += " [ -f ]";
//                bForse = true;
//            }
//        }
//    }
//
//                    // print the program name
//    std::cout << "    p4vc ";
//
//                    // print the options description
//    if (optionsNeeded)
//    {
//        ok = nextPair (cmd, desc);
//        if (!ok)
//            return;
//
//        while (ok)
//        {
//            if (cmd == "opt_options")
//            {
//                std::cout << desc;
//                break;
//            }
//            ok = nextPair (cmd, desc);
//        }
//    }
//
//                    // print the command and the command arguments
//    std::cout << " " << command.command << arguments;
//
//                    // add the free args to help
//    if (command.freeArgumentsFieldNumber != -1)
//    {
//        const google::protobuf::FieldDescriptor* pFieldDescriptor =
//            command.pDescriptor->FindFieldByNumber(command.freeArgumentsFieldNumber);
//        assert(pFieldDescriptor);
//
//        if (pFieldDescriptor->is_optional())
//            std::cout << " [ " << pFieldDescriptor->name() << " ]";
//        if (pFieldDescriptor->is_required())
//            std::cout << " " << pFieldDescriptor->name();
//        if (pFieldDescriptor->is_repeated())
//            std::cout << " [ " << pFieldDescriptor->name() << ", ... ]";
//    }
//
//    std::cout << std::endl << std::endl;
//
//    if (!bForse && !bChangeList && workspaceNeeded)
//        return;
//
//    ok = nextPair (cmd, desc);
//    if (!ok)
//        return;
//
//    while (ok)
//    {
//        if (bForse           && cmd == "opt_force"   ||
//            bChangeList      && cmd == "opt_change"  ||
//            !workspaceNeeded && cmd == "opt_no_workspace")
//        {
//            std::cout << "        " << desc << std::endl << std::endl;
//        }
//
//        ok = nextPair (cmd, desc);
//    }
//}

}
