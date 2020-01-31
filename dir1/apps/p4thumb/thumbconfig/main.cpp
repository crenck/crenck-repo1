#include <QCoreApplication>

#include "config.h"

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    Thumb::ExampleConfig thumbconfig;
    thumbconfig.generateExampleConfigData();

    if (!thumbconfig.saveThumbConfig("save.json"))
        return 1;

    Thumb::Config fromJsonThumbConfig;
    if (!fromJsonThumbConfig.loadThumbConfig("save.json"))
        return 1;

    if (!fromJsonThumbConfig.saveThumbConfig("save2.json"))
        return 1;

    return 0;
}
