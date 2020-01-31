#ifndef SETTINGS_THUMB__H
#define SETTINGS_THUMB__H

#include <QJsonObject>
#include <QString>

#include "changelistrange.h"
#include "connection.h"
#include "dimension.h"

namespace Thumb
{

class Settings
{
public:
    Settings();

    QString logFile() const;
    void setLogFile(const QString& logFile);

    int pollInterval() const;
    void setPollInterval(int pollInterval);

    const Connection& connection() const;
    void setConnection(const Connection connection);

    const ChangeListRange& range() const;
    void setRange(const ChangeListRange range);

    const Dimension& nativeSize() const;
    void setNativeSize(const Dimension size);

    int maxFileSize() const;
    void setMaxFileSize(int maxfilesz);

    void read(const QJsonObject& json);
    void write(QJsonObject& json) const;

private:
    QString mLogFile;
    int mPollInterval;
    Connection mConnection;
    ChangeListRange mRange;
    Dimension mDimension;
    int mMaxFileSize;
};

}

#endif // SETTINGS_THUMB__H
