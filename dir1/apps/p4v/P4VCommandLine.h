#ifndef INCLUDED_P4VCOMMANDLINE
#define INCLUDED_P4VCOMMANDLINE

#include <QObject>
#include <QHash>
#include <QString>

#include "JsonParser.h"
#include "ObjTypes.h"
#include "OpConfiguration.h"
#include "P4VCommandRunner.h"
#include "StringID.h"

namespace Gui
{
    class Command;
};

class P4VCommandLine : public QObject
{
    Q_OBJECT
public:
    enum ConsoleLogOption { None, Partial, All };

                      P4VCommandLine       (QObject* parent = 0);

    P4VCommandRunner& runner               ()       { return mRunner; }

    P4VCommandRunner::Mode
                      mode                 () const { return mRunner.mode(); }
    StringID          startCommand         () const { return mRunner.command(); }
    ConsoleLogOption  consoleLogOption     () const { return mConsoleLogOption; }
    Op::Configuration config               () const { return mRunner.config(); }
    int               winHandle            () const { return mWinHandle; }
    QString           errorMessage         () const { return mErrorMessage; }
    int               errorCode            () const { return mErrorCode; }
    bool              faceless             () const { return mFaceless; }
    bool              hasCustomStyle       () const { return mHasCustomStyle; }
    bool              runnable             () const { return mRunnable; }
    QStringList       paths                () const { return mRunner.targets(); }
    int               gridOption           () const { return mGridOption; }
    bool              usingEnviroArgs      () const { return mUsingEnviroArgs; }

    bool              parse                (QStringList& args);
    bool              parseJson            (const QString& json);

    bool              runningCommandTool   ();

    static StringID   FileCommandLookup    (const QString&);
    static StringID   SpecCommandLookup    (const QString&);
    static Obj::ObjectType   SpecTypeLookup       (const QString&);

private:
    bool parseCommand           (const QString & cmd, const QString& option, QStringList* args);
    bool parseConfigCommand     (const QString& option, QStringList* args);
    bool parsePathCommand       (const QString& option, QStringList* args);
    bool parseWinHandleCommand  (const QString& option, QStringList* args);
    bool parseCmdCommand        (const QString& option, QStringList* args);
    bool parseTabCommand        (const QString& option, QStringList* args);
    bool isSpecCommand          (const QString& option, QStringList* args);
    bool isFileCommand          (const QString& option, QStringList* args);
    void setError               (int code);

    P4VCommandRunner     mRunner;

    bool                 mFaceless;
    ConsoleLogOption     mConsoleLogOption;
    int                  mWinHandle;
    QString              mErrorMessage;
    int                  mErrorCode;
    bool                 mRunnable;
    bool                 mHasCustomStyle;
    int                  mGridOption;
    bool                 mUsingEnviroArgs;

    static QHash<QString, StringID> sFileCommandHash;
    static QHash<QString, StringID> sSpecCommandHash;
    static QHash<QString, Obj::ObjectType> sSpecTypeHash;
    static void InitCommandHashes();
};

#endif // INCLUDED_P4VCOMMANDLINE

