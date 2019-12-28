#include "SwitchBranchAgent.h"

#include "ObjAgency.h"
#include "ObjStash.h"
#include "OpContext.h"
#include "OpMessage.h"
#include "OpOperation.h"

namespace Obj
{

SwitchBranchAgent::SwitchBranchAgent(Agency* agency) : AgentSingleShot(agency)
  , mList(false)
  , mRepoName(QString())
  , mBranchName(QString())
  , mCurrentBranch(QString())
{
}
