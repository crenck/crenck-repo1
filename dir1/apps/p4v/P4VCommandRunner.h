/*! \brief  P4VCommandRunner - holds command settings and runs the command specified
*/

#include <QObject>

#include "StringID.h"
#include "ObjTypes.h"
#include "OpConfiguration.h"
#include "OpLogDesc.h"

#include <QVariant>

namespace Obj
{
    class Agent;
}

namespace Gui
{
    class Workspace;
}

class P4VCommandRunner : public QObject
{
    Q_OBJECT
public:
    enum Mode { P4V, Error, TestOnly, ShowHelp, OtherWindow };

    P4VCommandRunner();
    P4VCommandRunner(const P4VCommandRunner& other);

    virtual ~P4VCommandRunner();

    bool                        start           (Gui::Workspace* ws);
    void                        initStart       ();

    // access methods
    Op::Configuration&          config          ()                      { return mConfig; }
    const Op::Configuration&    config          () const                { return mConfig; }
    void                        setCommand      (StringID command)      { mStartCmd = command; }
    StringID                    command         () const                { return mStartCmd; }
    void                        setMode         (Mode mode)             { mMode = mode; }
    Mode                        mode            () const                { return mMode; }
    void                        addTarget       (const QString& obj)    { mTargets.push_back(obj); }
    void                        addTargets      (const QStringList& objs) { mTargets.append(objs); }
    QStringList                 targets         () const                { return mTargets; }
    void                        setObjectType   (Obj::ObjectType type)  { mObjectType = type; }
    void                        setTab          (const QString& tab)    { mInitTab = tab; }
    void                        setData         (const QString& data)   { mData = data; }

protected:
    // set when run
    Gui::Workspace*      mWorkspace;

    // set when constructing
    Mode                 mMode;
    StringID             mStartCmd;
    QStringList          mTargets;
    Op::Configuration    mConfig;
    QString              mErrorMessage;
    int                  mErrorCode;
    QString              mInitTab;
    Obj::ObjectType      mObjectType;
    QVariant             mData;

    // set when running a command
    QStringList          mNonExistentFiles;
    Obj::ObjectVector    mObjects;

    bool                validatePaths           ();
    bool                validateSpecs           ();
    bool                matchPathReturnType     ();
    void                emitSpecError           (const QString& id);

private slots:
    void                commandLineFilesReady   ();
    void                pathsValidated          (bool, const Obj::Agent &);
    void                propsRead               (bool, const Obj::Agent &);
    void                isInlistCallback        (bool, const QString &);

signals:
    void                startingCommand         ();
    void                error                   (const QString& msg);
    void                info                    (const QString& msg);

private:
};
