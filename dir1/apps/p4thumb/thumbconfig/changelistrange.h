#ifndef CHANGELISTRANGE_THUMB__H
#define CHANGELISTRANGE_THUMB__H

#include <QJsonObject>
#include <QString>

namespace Thumb
{

class ChangeListRange
{
public:
    ChangeListRange();

    void setRange(int from, int to);

    int fromValue() const;
    int toValue() const;

    void read(const QJsonObject& json);
    void write(QJsonObject& json) const;

private:
    int mFrom;
    int mTo;
};

}

#endif // CHANGELISTRANGE_THUMB__H
