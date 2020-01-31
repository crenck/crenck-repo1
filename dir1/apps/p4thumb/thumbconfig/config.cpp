#include "config.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>

namespace Thumb
{

const char* SETTINGS = "settings";
const char* CONVERSIONS = "conversions";

Config::Config()
{
}

const Settings& Config::settings() const
{
    return mSettings;
}

const QList<Conversion>& Config::conversions() const
{
    return mConversions;
}

bool Config::loadThumbConfig(const QString& filename)
{
    QFile loadFile(filename);

    if (!loadFile.open(QIODevice::ReadOnly))
    {
        fprintf(stderr, "Error: Couldn't open config file.\n");
        return false;
    }

    QByteArray saveData = loadFile.readAll();

    QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));

    if (loadDoc.isNull())
    {
        fprintf(stderr, "Error: JSON format failure\n");
        return false;
    }

    read(loadDoc.object());

    return true;
}

bool Config::saveThumbConfig(const QString& filename) const
{
    QFile saveFile(filename);

    if (!saveFile.open(QIODevice::WriteOnly))
    {
        fprintf(stderr,"Error: Couldn't write config file.\n");
        return false;
    }

    QJsonObject thumbconfigObject;
    write(thumbconfigObject);
    QJsonDocument saveDoc(thumbconfigObject);
    saveFile.write(saveDoc.toJson());

    return true;
}

void Config::read(const QJsonObject& json)
{
    mSettings.read(json[SETTINGS].toObject());

    mConversions.clear();
    if (json.contains(CONVERSIONS))
    {
        QJsonArray levelArray = json[CONVERSIONS].toArray();
        for (int levelIndex = 0; levelIndex < levelArray.size(); ++levelIndex)
        {
            QJsonObject levelObject = levelArray[levelIndex].toObject();
            Conversion level;
            level.read(levelObject);
            mConversions.append(level);
        }
    }
}

void Config::write(QJsonObject& json) const
{
    QJsonObject settingsObject;
    mSettings.write(settingsObject);
    json[SETTINGS] = settingsObject;

    QJsonArray levelArray;
    foreach (const Conversion level, mConversions)
    {
        QJsonObject levelObject;
        level.write(levelObject);
        levelArray.append(levelObject);
    }
    json[CONVERSIONS] = levelArray;
}

void ExampleConfig::generateExampleConfigData()
{
    mSettings = Settings();
    mSettings.setLogFile(QStringLiteral("/home/thumbnailer/log.out"));
    mSettings.setPollInterval(30);

    Connection connection;
    connection.setPort(":1666");
    connection.setUser("hvandermeer");
    connection.setClient("thumbnailClient");

    mSettings.setConnection(connection);

    QStringList args;
    args << "$FileIn" << "$FileOut";

    mConversions.clear();

    /*
     $ convert -resize 160x160  animated.gif[0] p4thumbs/animated.png
    */

    Conversion gifconv;
    QStringList gifext;
    gifext << "gif" << "tiff";

    gifconv.setExtensions(gifext);
    gifconv.setExecPath("/usr/local/bin/convert");

    QStringList gifargs;
    gifargs << "-resize" << "160x160" << "$FILEIN[0]" << "$FILEOUT";
    gifconv.setArguments(gifargs);

    mConversions.append(gifconv);

    /*
    $ convert stop11.ppm -thumbnail 160x160 p4thumbs/stop11.png
    */

    Conversion ppmconv;
    QStringList ppmext;
    ppmext << "ppm";
    ppmconv.setExtensions(ppmext);
    ppmconv.setExecPath("/usr/local/bin/convert");

    QStringList ppmargs;
    ppmargs << "$FILEIN" << "--thumbnail" << "160x160" << "$FILEOUT";
    ppmconv.setArguments(ppmargs);

    mConversions.append(ppmconv);

    /*
    $ convert -resize x160  -background white -alpha remove tutorial.pdf[0] p4thumbs/tutorial.png
    */

    Conversion pdfconv;
    QStringList pdfext;
    pdfext << "pdf";
    pdfconv.setExtensions(pdfext);
    pdfconv.setExecPath("/usr/local/bin/convert");

    QStringList pdfargs;
    pdfargs << "-resize" << "x160" << "-background" << "white" << "-alpha" << "remove" << "$FILEIN[0]" << "$FILEOUT";
    pdfconv.setArguments(pdfargs);

    mConversions.append(pdfconv);

    /*
    $ convert -resize 160x160 stop16.jpg p4thumbs/stop16.jpg
    */

    Conversion jpgconv;
    QStringList jpgext;
    jpgext << "jpg" << "jpeg";
    jpgconv.setExtensions(jpgext);
    jpgconv.setExecPath("/usr/local/bin/convert");

    jpgconv.setConvertToPng(false);

    QStringList jpgargs;
    jpgargs << "-resize" << "160x160" << "$FILEIN" << "$FILEOUT";
    jpgconv.setArguments(jpgargs);

    mConversions.append(jpgconv);

    /*
    $ convert -resize 160x160 sample.ai p4thumbs/sample.png
    $ convert -resize 160x160 image.svg p4thumbs/image.png
    */

    Conversion aiconv;
    QStringList aiext;
    aiext << "ai" << "svg";
    aiconv.setExtensions(aiext);
    aiconv.setExecPath("/usr/local/bin/convert");

    QStringList aiargs;
    aiargs << "-resize" << "160x160" << "$FILEIN" << "$FILEOUT";
    aiconv.setArguments(aiargs);

    mConversions.append(aiconv);
}

}


