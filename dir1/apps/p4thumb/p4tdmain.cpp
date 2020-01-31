#include "OpConfiguration.h"
#include "OpContext.h"
#include "ISConverter.h"
#include "ThumbUtil.h"
#include "DebugLog.h"
#include "Platform.h"
#include "XMLManager.h"
#include "config.h"
#include "ThumbPlugin.h"

#include "global_object_manager.h"

//#include <QApplication>
#include <QDir>
#include <QFile>
#include <QLibraryInfo>
#include <QTranslator>
#include <QtCore/QCoreApplication>

#include <stdio.h>

static void showVersionInfo()
{
    QString versionInfo = QString(
                              "P4Thumb - The Helix Thumbnail Daemon\n"
                              "Copyright 2004-" ID_Y " Perforce Software. All rights reserved.\n"
                              "Rev. P4TD/" ID_OS "/" ID_REL "/" ID_PATCH "\n"
                          );

    ISConverter::SendToStandardOut(versionInfo);
}

void
P4ThumbMessageOutput(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    Q_UNUSED(context);

    switch (type)
    {
    case QtDebugMsg:
#ifdef DEBUG
        fprintf(stdout, "Debug: %s\n", msg.toLocal8Bit().constData());
#endif
        break;
    case QtWarningMsg:
#ifdef DEBUG
        fprintf(stdout, "Warning: %s\n", msg.toLocal8Bit().constData());
#endif
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s\n", msg.toLocal8Bit().constData());
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s\n", msg.toLocal8Bit().constData());
        abort();
        break;
    default:
        break;
    }
}

int main( int argc, char** argv )
{
    GlobalObjectManager gom;

    qInstallMessageHandler(P4ThumbMessageOutput);

    QString testfile;

#ifdef DEBUG_GUI
    QCoreApplication a( argc, argv, true);
#else
    QCoreApplication a( argc, argv, false);
#endif
    Platform::SetApplicationName("P4Thumb");
    Platform::SetApplication(Platform::P4Thumb_App);

    /*
    // Install P4Thumb translations
    QString translationDir = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
    QTranslator* translator = new QTranslator(&a);
    if (translator->load( "p4thumb_" + QLocale::system().name(), translationDir ))
        qApp->installTranslator( translator );
    else
        delete translator;
   */
    
    if (argc == 1 || !strcmp(argv[1], "-h") || !strcmp(argv[1], "-help"))
    {
        ISConverter::ShowUsageInfo();
        return 1;
    }

    if (!strcmp(argv[1], "-V"))
    {
        showVersionInfo();
        return 1;
    }

    QString configFile("thumbconfig.json");

    bool consoleOutput = false;
    bool alwaysConvert = false;
    bool deleteThumbs = false;

    for (int i = 1; i < argc ; ++i)
    {
        const QString p = QString::fromLatin1(argv[i]);
        if (p.startsWith("-c"))
        {
            QString cfile = p.mid(2);
            if (cfile.isEmpty() && (i+1) < argc && *argv[i+1] != '-')
                cfile = QString::fromLatin1(argv[++i]);
            if (!cfile.isEmpty())
            {
                QFileInfo finfo(cfile);
                configFile = finfo.absoluteFilePath();
            }
        }
	if (p.startsWith("-test"))
	{
            testfile = p.mid(5);
	    if (testfile.isEmpty() && (i+1) < argc && *argv[i+1] != '-')
                testfile = QString::fromLatin1(argv[++i]);
	}
        else if (p.startsWith("-v"))
        {
            consoleOutput = true;
        }
        else if (p.startsWith("-f"))
        {
            alwaysConvert = true;
        }
        else if (p.startsWith("-d"))
        {
            deleteThumbs = true;
        }
    }

    QFileInfo fileexists(configFile);
    if (!fileexists.exists() || !fileexists.isFile())
    {
        QString msg;
        msg = "Cannot find configuration file : " + configFile;
        ISConverter::SendToStandardOut(msg);
        return 1;
    }

    Thumb::Config thumbConfig;
    if (!thumbConfig.loadThumbConfig(configFile))
    {
        QString msg;
        msg = "Failed to load the configuration file : " + configFile;
        ISConverter::SendToStandardOut(msg);
        return 1;
    }

    const Thumb::Settings sets = thumbConfig.settings();

    Op::Configuration config = Op::Configuration::DefaultConfiguration();

    const Thumb::Connection conn = sets.connection();
    config.setPort(conn.port());
    config.setUser(conn.user());
    config.setClient(conn.client());

    int pollInterval = sets.pollInterval();

    const Thumb::ChangeListRange fromto = sets.range();
    int fromChg = fromto.fromValue();
    int toChg = fromto.toValue();

    QString logPath = sets.logFile();

    const Thumb::Dimension sze = sets.nativeSize();
    int outW = sze.width();
    int outH = sze.height();

    int maxfilesz = sets.maxFileSize();

    if (deleteThumbs)
    {
        if (toChg == 0)
        {
            ISConverter::ShowMessage(SetEndForDeleteMessage);
            return 1;
        }
        alwaysConvert = true;   // don't want to skip if deleting
    }

    QString configPath = QDir::home().absoluteFilePath(".p4thumb");
    if ( Platform::IsMac() )
    {
        QDir tempDir = QDir::home();
        tempDir.cd( "Library" );
        tempDir.cd( "Preferences" );
        configPath = tempDir.absoluteFilePath( "com.perforce.p4thumb" );
    }

    // create the config dir location if it doesn't exist.
    QDir configDir(configPath);
    if (!configDir.exists())
        configDir.mkpath(configPath);
    qLogConfigureAndMonitor(configPath + "/p4thumb.log.config");
    DebugLog::Enable(true);
    // supply a default value for the output log located in the config dir
    if (logPath.isEmpty())
    {
        logPath = configDir.absoluteFilePath("p4thumb.log");
    }
    try
    {
        DebugLog::SetOutputFile(logPath);
        DebugLog::SetOutputFileMaxFileSize(500*1024);
        qLogToCategory("startup", 1)
                << "---- application startup "
                << QTime::currentTime().toString()
                << " ----"
                << qLogEnd;
    }
    catch (...)
    {
        // Squelch all log errors
    }

    XMLManager::setConfigDir(configPath);

    const QList<Thumb::Conversion> convs = thumbConfig.conversions();

    foreach (Thumb::Conversion cnv, convs)
    {
       if (!cnv.isConversionValid())
       {
            QStringList exts = cnv.extensions();
            QString msg = "The conversion for extension(s): \"" 
                  + exts.join(", ") + "\" will fail. " + cnv.validationMessage();
	    ISConverter::SendToStandardOut(msg);

            return 1;
       }
    }

    if (!testfile.isEmpty())
    {
	Thumb::ThumbPlugin plugin(convs, true);
	if (!plugin.convert(testfile))
	{
	    QString msg = "failed to convert : " + testfile;
	    ISConverter::SendToStandardOut(msg);
	}
        return 0;
    }

    ISConverter* poller = new ISConverter(config, outW, outH, maxfilesz, convs);
    poller->setShowConsoleOutput(consoleOutput);

    if (deleteThumbs)
        poller->setDeleteEnabled(true);

    if (fromChg)
        poller->setFirstChange(fromChg);

    if (toChg)
        poller->setFinalChange(toChg);

    if (alwaysConvert)
        poller->setAlwaysConvertEnabled(true);

    if (poller->beginPolling(pollInterval) == false)
        return -1;

    int result = a.exec();

    //DebugLog::Reset();

    return result;
}
