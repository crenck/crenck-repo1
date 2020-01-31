#include "ThumbPlugin.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QProcess>

#include "DebugLog.h"
#include "ISConverter.h"

qLogCategory("Thumb.ThumbPlugin");

namespace Thumb
{

ThumbPlugin::ThumbPlugin(QList<Conversion> conversions,  bool tostdout)
    : mConversions(conversions),
      mToStdout(tostdout)
{
}

bool ThumbPlugin::convert(QString inFile)
{
    if (mConversions.size() == 0)
        return false;

    QFileInfo finfo(inFile);
    QString ext = finfo.suffix();

    bool returnvalue = false;
    Conversion conv;
    foreach (Conversion cnv, mConversions)
    {
        if (cnv.uppercaseExtensions().contains(ext.toUpper()))
        {
            conv = cnv;
            returnvalue = true;
            break;
        }
    }

    if (!returnvalue)
        return false;

    bool toPng =  conv.convertToPng();

    QList<QString> arguments;
    QList<QString> stdoutarguments;
    const QList<QString> args = conv.arguments();

    foreach (QString arg, args)
    {
        if (arg.contains("$FILEIN"))
        {
            arg.replace("$FILEIN", inFile);
            arguments.append(arg);
            stdoutarguments.append("\"" + arg + "\"");
        }
        else if (arg == "$FILEOUT")
        {
            QString pathname;
            QDir dir(finfo.absolutePath());
            if (dir.cd("p4thumbs"))
            {
                pathname = dir.absolutePath();
            }
            else
            {
                if (!dir.mkdir("p4thumbs"))
                {
                    return false;
                }
                if (dir.cd("p4thumbs"))
                {
                    pathname = dir.absolutePath();
                }
            }

            QString ext;
            if (toPng)
            {
                ext = "png";
            }
            else
            {
                ext = conv.thumbnailExtension();
                if (ext.isEmpty())
                {
                    ext = finfo.suffix();
                }

            }

            mThumbFile = pathname + QDir::separator() + finfo.completeBaseName() + "." + ext;
            arguments.append(QDir::toNativeSeparators(mThumbFile));
            stdoutarguments.append("\"" + QDir::toNativeSeparators(mThumbFile) + "\"");
        }
        else
        {
            arguments.append(arg);
            stdoutarguments.append(arg);
        }
    }

    QString cmd = "\"" + conv.execPath() + "\""  + " " + stdoutarguments.join(" ");
    if (mToStdout)
    {
        ISConverter::SendToStandardOut("== executing command:");
        ISConverter::SendToStandardOut(cmd);
    }

    QProcess convertToThumb;
    convertToThumb.start(conv.execPath(), arguments);
    if (!convertToThumb.waitForStarted())
        return false;

    if (!convertToThumb.waitForFinished())
        return false;

    rLog(1) << "ThumbPlugin::convert: " << cmd << qLogEnd;

    QByteArray errOut = convertToThumb.readAllStandardError();
    QByteArray stdOut = convertToThumb.readAllStandardOutput();

    if (mToStdout)
    {
        if (!stdOut.isEmpty())
        {
            ISConverter::SendToStandardOut("== output");
            ISConverter::SendToStandardOut(stdOut.constData());
        }

        if (!errOut.isEmpty())
        {
            ISConverter::SendToStandardOut("== error");
            ISConverter::SendToStandardOut(errOut.constData());
        }
        else
        {
            ISConverter::SendToStandardOut("== Conversion successful");
        }
    }
    else
    {
        if (!errOut.isEmpty())
        {
            rLog(1) << "ThumbPlugin::error: " << errOut.constData() << qLogEnd;
        }
    }

    mResult = convertToThumb.readAll();

    return true;
}

QString ThumbPlugin::thumbFileName() const
{
    return mThumbFile;
}

}
