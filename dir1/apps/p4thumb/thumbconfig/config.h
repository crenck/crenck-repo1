#ifndef CONFIG_THUMB_H
#define CONFIG_THUMB_H

#include <QJsonObject>
#include <QList>

#include "settings.h"
#include "conversion.h"

namespace Thumb
{

class Config
{
public:
    Config();

    const Settings& settings() const;
    const QList<Conversion>& conversions() const;

    bool loadThumbConfig(const QString& filename);
    bool saveThumbConfig(const QString& filename) const;

    void read(const QJsonObject& json);
    void write(QJsonObject& json) const;

protected:
    Settings mSettings;
    QList<Conversion> mConversions;
};

class ExampleConfig : public Config
{
public:
    void generateExampleConfigData();
};

}

#endif // CONFIG_THUMB_H
