#include "OpConnection.h"

#include "DebugLog.h"
#include "OpMsgs.h"
#include "OpOperation.h"
#include "OpTypes.h"
#include "Platform.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QStringList>

qLogCategory("Op.Connection");

namespace Op
{

/*!
    \brief copies the state from another ConnectionState
    \param state the source ConnectionState to copy

    Copies the complete source state into this object, and determines
    if any associated ClientApi needs to drop its connection and call
    Init again.
*/
bool
ConnectionState::setFrom(const ConnectionState& state)
{
    bool dropConnection = false;
    if (mConfiguration.port() != state.mConfiguration.port() ||
            mConfiguration.user() != state.mConfiguration.user() ||
            mConfiguration.charset() != state.mConfiguration.charset() ||
            mProtocol != state.mProtocol)
    {
        dropConnection = true;
    }
    *this = state;
    return dropConnection;
}

Configuration
ConnectionState::configuration() const
{
    return mConfiguration;
}
