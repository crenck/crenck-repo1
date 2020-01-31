#include "settings.h"

namespace Thumb
{

const char* LOGFILE ="logFile";
const char* POLLINTERVAL = "pollInterval";
const char* CONNECTION = "connection";
const char* RANGE = "changeListRange";
const char* SIZE = "nativeSize";
const char* MAXFILE = "maxFileSize";

Settings::Settings() :
    mPollInterval(30),
    mMaxFileSize(0)
{
}

QString Settings::logFile() const
{
    return mLogFile;
}

void Settings::setLogFile(const QString& logfile)
{
    mLogFile = logfile;
}

int Settings::pollInterval() const
{
    return mPollInterval;
}

void Settings::setPollInterval(int pollinterval)
{
    mPollInterval = pollinterval;
}

const Connection& Settings::connection() const
{
    return mConnection;
}

void Settings::setConnection(const Connection connection)
{
    mConnection = connection;
}

const ChangeListRange& Settings::range() const
{
    return mRange;
}

void Settings::setRange(const ChangeListRange range)
{
    mRange = range;
}

const Dimension& Settings::nativeSize() const
{
    return mDimension;
}

void Settings::setNativeSize(const Dimension size)
{
    mDimension = size;
}

int Settings::maxFileSize() const
{
    return mMaxFileSize;
}

void Settings::setMaxFileSize(int maxfilesz)
{
    mMaxFileSize = maxfilesz;
}

void Settings::read(const QJsonObject& json)
{
    mLogFile = json[LOGFILE].toString();

    if (json.contains(POLLINTERVAL))
    {
        mPollInterval = json[POLLINTERVAL].toInt();
    }

    QJsonObject connectionObject = json[CONNECTION].toObject();
    mConnection.read(connectionObject);

    QJsonObject rangeObject = json[RANGE].toObject();
    mRange.read(rangeObject);

    QJsonObject sizeObject = json[SIZE].toObject();
    mDimension.read(sizeObject);

    if (json.contains(MAXFILE))
    {
       mMaxFileSize = json[MAXFILE].toInt();
    }
}

void Settings::write(QJsonObject& json) const
{
    json[LOGFILE] = mLogFile;
    json[POLLINTERVAL] = mPollInterval;

    QJsonObject connectionObject;
    mConnection.write(connectionObject);
    json[CONNECTION] = connectionObject;

    QJsonObject rangeObject;
    mRange.write(rangeObject);
    json[RANGE] = rangeObject;

    QJsonObject sizeObject;
    mDimension.write(sizeObject);
    json[SIZE] = sizeObject;

    if (mMaxFileSize > 0) 
    {
        json[MAXFILE] = mMaxFileSize;
    }
}

}
