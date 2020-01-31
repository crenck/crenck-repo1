
#ifndef CONVERSION_THUMB__H
#define CONVERSION_THUMB__H

#include <QJsonObject>
#include <QList>

#include "settings.h"

namespace Thumb
{

class Conversion
{
public:
    Conversion();

    const QList<QString>& extensions() const;
    void setExtensions(const QList<QString>& extensions);
    const QList<QString>& uppercaseExtensions() const;

    bool convertToPng() const;
    void setConvertToPng(bool convert);

    QString thumbnailExtension() const;
    void setThumbnailExtension(const QString& thumbnailExtension);

    QString execPath() const;
    void setExecPath(const QString& execPath);

    const QList<QString>& arguments() const;
    void setArguments(const QList<QString>& arguments);

    bool isConversionValid();
    QString validationMessage();

    void read(const QJsonObject& json);
    void write(QJsonObject& json) const;

private:
    QList<QString>     mExtensions;
    QList<QString>     mUppercaseExtensions;
    bool               mConvertToPng;
    QString            mThumbnailExtension;
    QString            mExecPath;
    QList<QString>     mArguments;
    QString            mValidationMessage;
};

}

#endif // CONVERSION_THUMB__H
