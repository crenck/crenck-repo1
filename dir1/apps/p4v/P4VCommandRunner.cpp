/*! \brief  P4VCommandRunner.cpp - implementation of settigns holder and runner
*/

#include "P4VCommandRunner.h"

#include "ActionStore.h"
#include "Application.h"
#include "CmdAction.h"
#include "CmdActionGroup.h"
#include "CommandHandler.h"
#include "CommandIds.h"
#include "GroupsAgent.h"
#include "GuiFeatures.h"
#include "GuiForm.h"
#include "GuiWindowManager.h"
#include "MainWindow.h"
#include "ObjForm.h"
#include "ObjFormList.h"
#include "ObjPendingChange.h"
#include "ObjStash.h"
#include "ObjUser.h"
#include "OpContext.h"
#include "PathValidateAgent.h"
#include "Prefs.h"
#include "PropertyAgent.h"
#include "SandstormApp.h"
#include "SettingKeys.h"
#include "UITabFloater.h"
#include "Workspace.h"

#include <QApplication>
#include <QMap>
#include <QTimer>

/*! \brief given a workspace, execute the command settings against it
*/

bool
P4VCommandRunner::start(Gui::Workspace* ws)
{
    mWorkspace = ws;
    bool callInitStartFromHere = true;

        // fetch user's groups here since forms can now be owned by groups instead of users,
        // we'll need to know whether or not
    if (ws->stash())
    {
        Op::ServerProtocol sp = ws->stash()->serverProtocol();
            // if we can have groups owning streams, then we need to retrieve and cache the current
            // user's groups on the user object
        if (sp.canGroupOwnStream())
        {
            Obj::Stash* stash = ws->stash();
            Obj::GroupsAgent* gAgent = new Obj::GroupsAgent(stash);
            Ptr<Obj::User> user = stash->user();
            gAgent->setMemberID(user->name(), true);
            connect(gAgent,        SIGNAL(groupList   (const QStringList &, const QString &, bool)),
                    user.getPtr(), SLOT  (setGroupList(const QStringList &)));
            gAgent->run();
        }

            // if the server has property tables, then we need to run the property agents to grab
            // what overrides the admins have set
        if (sp.hasPropertyTable())
        {
            Obj::PropertyAgent* fagent = new Obj::PropertyAgent(mWorkspace->stash());
            connect(fagent, SIGNAL(done     (bool, const Obj::Agent&)),
                    this,   SLOT  (propsRead(bool, const Obj::Agent&)));
            fagent->run();

            callInitStartFromHere = false;
        }
    }

    QString scope = ws->scopeName();
    Gui::PropertyFeatures::Instance()->markAsLoaded(scope);

    if ( mTargets.isEmpty() )
    {
        if ( callInitStartFromHere )
            initStart();

        return true;
    }

    if (mObjectType == Obj::FILE_TYPE)
        return validatePaths();
    else
        return validateSpecs();
}

void
P4VCommandRunner::initStart()
{

    if (!Gui::CommandHandler::Handle(Gui::Command(mStartCmd), mWorkspace))
        return;
    
    if (!mInitTab.isEmpty())
    {
        tabs::PageType initTabType = UITabFloater::GetPageFromLabel(mInitTab);
        if (initTabType != tabs::End )
        {
            // make sure that the main window has updated the command actions (job047414)
            QApplication::setActiveWindow(mWorkspace->windowManager()->mainWorkspaceWindow());
            ActionStore*       mainWindowStore = Gui::MainWindow::WindowCommands();
            CmdActionGroup*    showTab         = static_cast<CmdActionGroup*>(mainWindowStore->find(Cmd_ShowTab));
            CmdAction*         cmdA            = showTab->actionForData(initTabType);
            if (cmdA)
                cmdA->activate(QAction::Trigger);
        }
    }
}

/*! \brief when the settings require files, make sure that all of them have been validated
*/

bool
P4VCommandRunner::validatePaths()
{
    Obj::MultiPathValidateAgent* agent = new Obj::MultiPathValidateAgent(mWorkspace->stash());
    connect(agent, SIGNAL(done(bool, const Obj::Agent&)), SLOT(pathsValidated(bool, const Obj::Agent&)));
    agent->run(mTargets, matchPathReturnType());
    return true;
}

void
P4VCommandRunner::propsRead(bool , const Obj::Agent &agent)
{
    const Obj::PropertyAgent* pa = static_cast<const Obj::PropertyAgent*>(&agent);

    QMap<QString, QString> properties = pa->properties();
    QString scope = mWorkspace->scopeName();

    Gui::PropertyFeatures* propFeatures = Gui::PropertyFeatures::Instance();

        // URL/location based properties
    QString combineProp = properties["P4.Combine.URL"];
    propFeatures->insert(scope, Gui::Features::P4Combine, combineProp);

    QString commonsProp = properties["P4.Commons.URL"];
    propFeatures->insert(scope, Gui::Features::P4Commons, commonsProp);

    QString helpProp = properties["P4V.Help.URL"];
    propFeatures->insert(scope, Gui::Features::P4VHelp, helpProp);

    QString key;

        // PERFORMANCE PROPERTIES
    key = "P4V.Performance.MaxPreviewSize";
    if (properties.contains(key))
        Prefs::SetOverride(Prefs::WS(scope, p4v::prefs::MaxFilePreviewSize),
                           QVariant(properties[key].toInt()), true);
    key = "P4V.Performance.ServerRefresh";
    if (properties.contains(key))
        Prefs::SetOverride(Prefs::WS(scope, p4v::prefs::RefreshRate),
                           QVariant(properties[key].toInt()), true);
    key = "P4V.Performance.MaxFiles";
    if (properties.contains(key))
        Prefs::SetOverride(Prefs::WS(scope, p4v::prefs::ChangeFileFetchCount),
                           QVariant(properties[key].toInt()), true);
    key = "P4V.Performance.FetchCount";
    if (properties.contains(key))
        Prefs::SetOverride(Prefs::WS(scope, p4v::prefs::FormFetchCount),
                           QVariant(properties[key].toInt()), true);
    key = "P4V.Performance.OpenedLimit";
    if (properties.contains(key))
        Prefs::SetOverride(Prefs::WS(scope, p4v::prefs::OpenedCheckLimit),
                           QVariant(properties[key].toInt()),
                           true);
    key = QString("P4V.Performance.%1").arg(Gui::Features::AllowFullIstats);
    if (properties.contains(key))
        Prefs::SetOverride(Prefs::WS(scope, p4v::prefs::RunIstats),
                           properties[key] == "On",
                           true);

        // DASHBOARD PROPERTIES
    key = QString("P4V.Features.%1.Limit").arg(Gui::Features::DashBoard);
    if (properties.contains(key))
    {
        Prefs::SetOverride(Prefs::WS(QString(), p4v::prefs::MaxDashboardFileViewCount),
                           QVariant(properties[key].toInt()), true);
    }

        // FEATURES PROPERTIES
        // for most of these, we don't care about any 'value' associated with them
        // we use the presence of them to determine whether or not they're 'off'
        // for those that are only 'on' or 'off' we simply insert an empty QVariant
        // as the value for the key
    key = QString("P4V.Features.%1").arg(Gui::Features::Integration);
    if (properties.contains(key) && (QVariant(properties[key]).toString() == "Off")) 
    {
        propFeatures->insert(scope, Gui::Features::Integration, QVariant());
        mWorkspace->setFeatureVisible(Gui::Features::Integration, false);
    }
    key = QString("P4V.Features.%1").arg(Gui::Features::Labeling);
    if (properties.contains(key) && (QVariant(properties[key]).toString() == "Off")) 
    {
        propFeatures->insert(scope, Gui::Features::Labeling, QVariant());
        mWorkspace->setFeatureVisible(Gui::Features::Labeling, false);
    }
    key = QString("P4V.Features.%1").arg(Gui::Features::Jobs);
    if (properties.contains(key) && (QVariant(properties[key]).toString() == "Off")) 
    {
        propFeatures->insert(scope, Gui::Features::Jobs, QVariant());
        mWorkspace->setFeatureVisible(Gui::Features::Jobs, false);
    }
    key = QString("P4V.Features.%1").arg(Gui::Features::RevisionGraph);
    if (properties.contains(key) && (QVariant(properties[key]).toString() == "Off")) 
    {
        propFeatures->insert(scope, Gui::Features::RevisionGraph, QVariant());
        mWorkspace->setFeatureVisible(Gui::Features::RevisionGraph, false);
    }
    key = QString("P4V.Features.%1").arg(Gui::Features::Timelapse);
    if (properties.contains(key) && (QVariant(properties[key]).toString() == "Off")) 
    {
        propFeatures->insert(scope, Gui::Features::Timelapse, QVariant());
        mWorkspace->setFeatureVisible(Gui::Features::Timelapse, false);
    }
    key = QString("P4V.Features.%1").arg(Gui::Features::CustomTools);
    if (properties.contains(key) && (QVariant(properties[key]).toString() == "Off")) 
    {
        propFeatures->insert(scope, Gui::Features::CustomTools, QVariant());
        mWorkspace->setFeatureVisible(Gui::Features::CustomTools, false);
    }
    key = QString("P4V.Features.%1").arg(Gui::Features::Administration);
    if (properties.contains(key) && (QVariant(properties[key]).toString() == "Off")) 
    {
        propFeatures->insert(scope, Gui::Features::Administration, QVariant());
        mWorkspace->setFeatureVisible(Gui::Features::Administration, false);
    }
    key = QString("P4V.Features.%1").arg(Gui::Features::ConnectionWizard);
    if (properties.contains(key) && (QVariant(properties[key]).toString() == "Off")) 
    {
        propFeatures->insert(scope, Gui::Features::ConnectionWizard, QVariant());
        mWorkspace->setFeatureVisible(Gui::Features::ConnectionWizard, false);
    }
    key = QString("P4V.Features.%1").arg(Gui::Features::Workspaces);
    if (properties.contains(key) && (QVariant(properties[key]).toString() == "Off")) 
    {
        propFeatures->insert(scope, Gui::Features::Workspaces, QVariant());
        mWorkspace->setFeatureVisible(Gui::Features::Workspaces, false);
    }
    key = QString("P4V.Features.%1").arg(Gui::Features::DashBoard);
    if (properties.contains(key) && (QVariant(properties[key]).toString() == "Off")) 
    {
        propFeatures->insert(scope, Gui::Features::DashBoard, QVariant());
        mWorkspace->setFeatureVisible(Gui::Features::DashBoard, false);
    }
    key = QString("P4V.Features.%1").arg(Gui::Features::P4VApplets);
    if (properties.contains(key) && (QVariant(properties[key]).toString() == "Off")) 
    {
        propFeatures->insert(scope, Gui::Features::P4VApplets, QVariant());
        mWorkspace->setFeatureVisible(Gui::Features::P4VApplets, false);
    }
    key = QString("P4V.Features.%1").arg(Gui::Features::Streams);
    if (properties.contains(key) && (QVariant(properties[key]).toString() == "Off")) 
    {
        propFeatures->insert(scope, Gui::Features::Streams, QVariant());
        mWorkspace->setFeatureVisible(Gui::Features::Streams, false);
    }
    key = QString("P4V.Features.%1").arg(Gui::Features::CheckForUpdates);
    if (properties.contains(key) && (QVariant(properties[key]).toString() == "Off")) 
    {
        propFeatures->insert(scope, Gui::Features::CheckForUpdates, QVariant());
        mWorkspace->setFeatureVisible(Gui::Features::CheckForUpdates, false);
    }
    key = QString("P4.DataAnalyticsPrompt");
    if (properties.contains(key) && (QVariant(properties[key]).toString() == "Off")) 
    {
        propFeatures->insert(scope, Gui::Features::DataAnalytics, QVariant());
        mWorkspace->setFeatureVisible(Gui::Features::DataAnalytics, false);
    }
    key = QString("P4V.Features.%1").arg(Gui::Features::UnloadReload);
    if (properties.contains(key) && (QVariant(properties[key]).toString() == "Off")) 
    {
        propFeatures->insert(scope, Gui::Features::UnloadReload, QVariant());
        mWorkspace->setFeatureVisible(Gui::Features::UnloadReload, false);
    }
    key = QString("P4V.Features.%1").arg(Gui::Features::UnshelveSideways);
    if (properties.contains(key) && (QVariant(properties[key]).toString() == "Off"))
    {
        propFeatures->insert(scope, Gui::Features::UnshelveSideways, QVariant());
        mWorkspace->setFeatureVisible(Gui::Features::UnshelveSideways, false);
    }
    key = QString("P4V.Features.%1").arg(Gui::Features::Dvcs);
    if (properties.contains(key) && (QVariant(properties[key]).toString() == "Off"))
    {
        propFeatures->insert(scope, Gui::Features::Dvcs, QVariant());
        mWorkspace->setFeatureVisible(Gui::Features::Dvcs, false);
    }
    key = QString("P4V.Features.%1").arg(Gui::Features::Parallel);
    if (properties.contains(key))
    {
        QString parallelProp = properties[key];
        propFeatures->insert(scope, Gui::Features::Parallel, parallelProp);
    }

        // checking for maximum changelist allowed for the update feature needs
        // somewhat different handling
    key = QString("P4V.Features.%1").arg(Gui::Features::MaxAllowedVersion);
    if (properties.contains(key) && (QVariant(properties[key]).toInt()) )
        propFeatures->insert(scope, Gui::Features::MaxAllowedVersion, properties[key]);

    QString swarmTimeoutProp = properties["P4.Swarm.Timeout"];
    propFeatures->insert(scope, Gui::Features::SwarmTimeout, swarmTimeoutProp);

    QString swarmProp = properties["P4.Swarm.URL"];
    propFeatures->insert(scope, Gui::Features::P4Swarm, swarmProp);

	initStart();
}

/*! \brief slot callback when all paths have been validated
*/

void
P4VCommandRunner::pathsValidated(bool , const Obj::Agent &agent)
{
    // save the data for later
    const Obj::MultiPathValidateAgent* pv = static_cast<const Obj::MultiPathValidateAgent*>(&agent);
    mObjects            = pv->objects();
    mNonExistentFiles   = pv->failedPaths();

    // not in the middle of an Op::Operation, this could cause dialogs to appear
    QTimer::singleShot(0, this, SLOT(commandLineFilesReady()));
}

/*! \brief customize the type of paths object returned - local or depot
*/

bool
P4VCommandRunner::matchPathReturnType()
{
    // if it's the main windows we cant local paths returned as local objects for
    // -s and open
    if (mStartCmd == Cmd_MainWindow)
        return true;
    return false; // don't care but will return depot if it exists
}

/*! \brief called when all of the files are fetched and valid
*/

void
P4VCommandRunner::commandLineFilesReady()
{
    if (mStartCmd == Cmd_DiffDialog)
        Gui::Application::dApp->setDelayExit(true);

    emit startingCommand();

    if (mObjects.count())
    {
        bool handled = false;
        if (mStartCmd == Cmd_MainWindow)
        {
            handled = Gui::CommandHandler::Handle(Gui::Command(mStartCmd, mObjects), mWorkspace, mData);
        }
        else
        {
            QAction* action = ActionStore::Global().findAction(mStartCmd);
            if (action)
            {
                Gui::CommandHandler::CustomizeAction(Gui::Command(mStartCmd, mObjects), mWorkspace, action);
                if (action->isEnabled())
                    handled = Gui::CommandHandler::Handle(Gui::Command(mStartCmd, mObjects), mWorkspace, mData);
            }
        }
        if (!handled)
        {
            QString errorMessage = SandstormApp::tr("Helix P4V: The requested action could not be performed.\n");
            emit error(errorMessage);
        }
    }
    mObjects.clear();
    if (!mNonExistentFiles.isEmpty())
    {
        QString errorMessage = SandstormApp::tr("Helix P4V: The following paths do not exist:\n");
        errorMessage.append(mNonExistentFiles.join("\n"));
        // if nothing can be displayed, show an error, otherwise show info
        if (mObjects.isEmpty())
            emit error(errorMessage);
        else
            emit info(errorMessage);
        mNonExistentFiles.clear();
    }
}

/*! \brief for commands that use specs, validate the spec targets
*/

bool
P4VCommandRunner::validateSpecs()
{
    Ptr<Obj::FormList> dl = mWorkspace->stash()->formListForType( mObjectType );

    bool retVal = false;
    foreach(QString id, mTargets)
    {
        if ((mObjectType == Obj::PENDING_CHANGE_TYPE && id != Obj::PendingChange::DefaultName())
            || mObjectType == Obj::CHANGE_TYPE)
        {
            bool success;
            id.toInt(&success);
            if (!success)
            {
                emitSpecError(id);
                continue;
            }
        }

        retVal = true;
        dl->inList(id, mWorkspace->stash(), this, SLOT( isInlistCallback(bool, const QString &) ));
    }

    return retVal;
}

/*! \brief slot callback for commands that use specs indicating if a target exists
*/

void
P4VCommandRunner::isInlistCallback( bool inList, const QString & id )
{
    if ( inList )
    {
        // TODO: this will launch multiple windows (see validateSpecs), which is a problem
        Ptr<Obj::Form> f = mWorkspace->stash()->formForID(mObjectType, id);
        Gui::CommandHandler::Handle(Gui::Command(mStartCmd, f), mWorkspace);
    }
    else
    {
        emitSpecError(id);
    }
}

/*! \brief helper function to format and init an error
*/

void
P4VCommandRunner::emitSpecError(const QString& id)
{
    QString message = tr("%1 : '%2' does not exist.").arg(Gui::Form::FormTitle(mObjectType)).arg(id);
    emit error(message);
}

/*! \brief constructor
*/

P4VCommandRunner::P4VCommandRunner()
: QObject(NULL)
, mWorkspace(NULL)
, mMode(P4V)
, mStartCmd(Cmd_MainWindow)
, mErrorCode(0)
, mObjectType(Obj::UNKNOWN_TYPE)
{
}

/*! \brief copy constructor (shallow, settings only)
*/

P4VCommandRunner::P4VCommandRunner(const P4VCommandRunner& other)
: QObject(NULL)
, mWorkspace(NULL)
{
    mMode       = other.mMode;
    mStartCmd   = other.mStartCmd;
    mTargets    = other.mTargets;
    mConfig     = other.mConfig;
    mErrorMessage = other.mErrorMessage;
    mErrorCode  = other.mErrorCode;
    mInitTab    = other.mInitTab;
    mObjectType = other.mObjectType;
    mData       = other.mData;
}

/*! \brief destructor
*/

P4VCommandRunner::~P4VCommandRunner()
{
}
