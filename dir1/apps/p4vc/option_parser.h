#ifndef __OPTIONPARSER_H__
#define __OPTIONPARSER_H__

#include <string>

#include "P4VCMessage.h"

/*! \brief Parse argv into args, using Perforce Options parser.
*/
namespace p4vc
{

class OptionParser
{
public :

             OptionParser             ();
    virtual ~OptionParser             ();

    enum HelpType
    {
        Help_None = 0,
        Help_General,
        Help_Commands,
        Help_Command
    };

    bool                       parse                   (int argc, char** argv);

    //void                            printHelpCommands       (const std::string & translatedStr);
    //void                            printHelpCommand        (const std::string& commandName,
    //                                                         const std::string & translatedStr);

    std::string     mCommand;       // the command (help or other)
    HelpType        mHelpType;      // type of help requesting from p4v service
    std::string     mHelpCommand;   // the actual command that help is being requested for

    QSharedPointer<P4VCMessage> mpMessage; // the constructed message

private:
    //void                       parseServer             (const std::string& server);
    bool                       parseCommandArguments   (int& argc, char**& argv);
    bool                       parseFreeField          (int& argc, char**& argv);
    //void                       parseHelpCommand        (int& argc, char**& argv);
    bool                       parseCommand            (int argc, char** argv);
    //void                       initField               (const char* value);
    void                        parseHelpCommand(char** vars);

    //bool                            nextPair                (std::string & key, std::string & value);
    bool                            convertLocaleToUtf8     (int argc,
                                                             char **argv,
                                                             char **translatedArgs,
                                                             const char * charsetName,
                                                             char * localeHost,
                                                             char ** host,
                                                             char * localePort,
                                                             char ** port,
                                                             char * localeUser,
                                                             char ** user,
                                                             char * localeCwd,
                                                             char ** cwd);

    bool                                            bForsedWorkspace;
    bool                                            mNeedsInit;
    char *                                          mToken;
    char *                                          mTmpString;
    std::string                                     mTranslatedString;
};

}

#endif // __OptionParser_h__
