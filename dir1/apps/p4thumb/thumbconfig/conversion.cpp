#include "conversion.h"

#include <QFileInfo>
#include <QJsonArray>
#include <QStandardPaths>

namespace Thumb
{

const char* EXTENSIONS = "extensions";
const char* CONVERT = "convertToPng";
const char* THUMBEXTENSION = "thumbnailExtension";
const char* EXECPATH = "execPath";
const char* ARGUMENTS = "arguments";

Conversion::Conversion()
    : mConvertToPng(true)
{
}

const QList<QString>& Conversion::extensions() const
{
    return mExtensions;
}    

const QList<QString>& Conversion::uppercaseExtensions() const
{
        return mUppercaseExtensions;
}

void Conversion::setExtensions(const QList<QString>& extensions)
{
    mExtensions = extensions;
    mUppercaseExtensions.clear();
    foreach (const QString ext, mExtensions)
    {
        mUppercaseExtensions.append(ext.toUpper());
    }
}

bool Conversion::convertToPng() const
{
    return mConvertToPng;
}

void Conversion::setConvertToPng(bool convert)
{
    mConvertToPng = convert;
}

QString Conversion::thumbnailExtension() const
{
    return mThumbnailExtension;
}

void Conversion::setThumbnailExtension(const QString& thumbnailExtension)
{
    mThumbnailExtension = thumbnailExtension;
}

QString Conversion::execPath() const
{
    return mExecPath;
}

void Conversion::setExecPath(const QString& execPath)
{
    mExecPath = execPath;
}

const QList<QString>& Conversion::arguments() const
{
    return mArguments;
}

void Conversion::setArguments(const QList<QString>& arguments)
{
    mArguments = arguments;
}

bool Conversion::isConversionValid()
{
    bool containsFileIn = false;
    bool containsFileOut = false;

    QFileInfo finfo(mExecPath);
    if (!finfo.exists()) {
        QString path = QStandardPaths::findExecutable(mExecPath);
        if (path.isEmpty())
        {
           mValidationMessage = "The conversion executable cannnot be found."; 
           return false;
        }
    }

    const QList<QString>& args = arguments();

    foreach (QString arg, args)
    {
        if (arg.contains("$FILEIN"))
        {
            containsFileIn = true;
        }
        else if (arg == "$FILEOUT")
        {
            containsFileOut = true;
        }
    }

    if (!containsFileIn)
    {
        mValidationMessage = "The mandatory \"$FILEIN\" argument is missing."; 
    }
    else if (!containsFileOut)
    {
        mValidationMessage = "iThe mandatory \"$FILEOUT\", argument is missing."; 
    }

    if ( containsFileIn && containsFileOut )
    {
        return true;
    }  
    return false;
}

QString Conversion::validationMessage()
{
    return mValidationMessage; 
}

void Conversion::read(const QJsonObject& json)
{
    mExtensions.clear();
    mUppercaseExtensions.clear();
    mArguments.clear();

    QJsonArray extensionArray = json[EXTENSIONS].toArray();
    for (int extIndex = 0; extIndex < extensionArray.size(); ++extIndex)
    {
        QJsonValue extValue = extensionArray.at(extIndex);
        mExtensions.append(extValue.toString());
        mUppercaseExtensions.append(extValue.toString().toUpper());
    }

    mExecPath = json[EXECPATH].toString();

    if (json.contains(CONVERT))
    {
        mConvertToPng = json[CONVERT].toBool();
    }

    if (json.contains(THUMBEXTENSION))
    {
        mThumbnailExtension = json[THUMBEXTENSION].toString();
    }

    QJsonArray argumentsArray = json[ARGUMENTS].toArray();
    for (int argIndex = 0; argIndex < argumentsArray.size(); ++argIndex)
    {
        QJsonValue argValue = argumentsArray.at(argIndex);
        mArguments.append(argValue.toString());
    }
}

void Conversion::write(QJsonObject& json) const
{
    QJsonArray extensionArray;
    foreach (const QString ext, mExtensions)
    {
        QJsonValue extValue(ext);
        extensionArray.append(extValue);
    }
    json[EXTENSIONS] = extensionArray;

    json[EXECPATH] = mExecPath;

    if (!mConvertToPng)
    {
        json[CONVERT] = mConvertToPng;
    }

    if (!mThumbnailExtension.isEmpty())
    {
        json[THUMBEXTENSION] = mThumbnailExtension;
    }

    QJsonArray argumentsArray;
    foreach (const QString arg, mArguments)
    {
        QJsonValue argValue(arg);
        argumentsArray.append(argValue);
    }
    json[ARGUMENTS] = argumentsArray;
}

}
