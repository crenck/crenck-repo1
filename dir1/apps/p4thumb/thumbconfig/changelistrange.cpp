#include "changelistrange.h"

namespace Thumb
{

const char* FROM ="from";
const char* TO = "to";

ChangeListRange::ChangeListRange() :
    mFrom(0),
    mTo(0)
{
}

void ChangeListRange::setRange(const int from, const int to)
{
    mFrom = from;
    mTo = to;
}

int ChangeListRange::fromValue() const
{
    return mFrom;
}

int ChangeListRange::toValue() const
{
    return mTo;
}

void ChangeListRange::read(const QJsonObject& json)
{
    if (json.contains(FROM))
    {
        mFrom = json[FROM].toInt();
    }
    if (json.contains(TO))
    {
        mTo = json[TO].toInt();
    }
}

void ChangeListRange::write(QJsonObject& json) const
{
    json[FROM] = mFrom;
    json[TO] = mTo;
}

}

