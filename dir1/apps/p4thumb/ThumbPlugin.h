#ifndef THUMBPLUGIN_THUMB__H
#define THUMBPLUGIN_THUMB__H

#include <QString>

#include "conversion.h"

namespace Thumb
{

class ThumbPlugin
{
public:
    ThumbPlugin(QList<Conversion> conversions, bool tostdout=false);

    bool convert(QString inFile);
    QString thumbFileName() const;

private:
    QList<Conversion> mConversions;

    QString mThumbFile;
    QByteArray mResult;
    bool mToStdout;
};

}

#endif // THUMBPLUGIN_THUMB__H
