/*!
 *  \brief  QApplication with QTranslator files
 */

#include "MiniQApp.h"

#include "AdminAboutBox.h"
#include "Platform.h"
#include "PMessageBox.h"
#include "WinFlags.h"

#include <QLibraryInfo>
#include <QLocale>
#include <QResource>
#include <QTextStream>
#include <QTranslator>


//
// Strings used by AdminMain are declared here to faciliate
// being located by the "jam lupdate" scripts.
//
const char* ADMIN_MSG_USAGE =
    QT_TRANSLATE_NOOP("MiniQApp",
        "Helix Admin - The Perforce Visual Administration Tool\n\n"
        "The application will launch and select a connection specified by the following parameters.\n"
        "Usage: p4admin [OPTION]...\n"
        "    -p port        set p4port\n"
        "    -u user        set p4user\n"
        "    -C charset     set charset\n"
        "    -V             display application version\n"
        "    -h             this message\n"
        );

const char* ADMIN_MSG_VERSION =
    QT_TRANSLATE_NOOP("MiniQApp",
        "Helix Admin - The Perforce Visual Administration Tool\n"
        "Copyright 2002-%1 Perforce Software, Inc.  All rights reserved.\n"
        "Version %2\n"
        );
//      %1 = Platform::BuildYear()
//      %2 = Platform::ApplicationNameAndVersion()

const char* ADMIN_MSG_BAD_PARAM =
    QT_TRANSLATE_NOOP("MiniQApp",
        "Helix Admin: unrecognized option '%1'\n"
        "try '%2 -h' for more information.\n"
        );
//      %1 = argv[i]
//      %2 = argv[0]    // launched application name


MiniQApp::MiniQApp(int& argc, char** argv)
: QApplication(argc, argv)
, mArgc(argc)
, mArgv(argv)
{
    QString translationDir = QLibraryInfo::location(
            QLibraryInfo::TranslationsPath);
    QTranslator* translator = new QTranslator(this);
    translator->setObjectName( "p4admin translator miniQApp" );
    if(translator->load( "p4admin_" + QLocale::system().name(),
                translationDir ))
    {
        installTranslator( translator );
    }
    else
    {
        delete translator;
    }
}

void
MiniQApp::showVersionInfo()
{
#ifdef Q_OS_WIN
    QResource::registerResource(Platform::ImageResourceFile());
    AdminAboutBox* about = new AdminAboutBox(activeWindow(), WinFlags::WindowFlags());
    about->exec();
#else
    QString msg = tr( ADMIN_MSG_VERSION )
        .arg(Platform::BuildYear())
        .arg(Platform::ApplicationNameAndVersion());

    QTextStream stout(stdout, QIODevice::WriteOnly);
    stout << msg << "\n";
#endif
}


void
MiniQApp::showUsageInfo()
{
    QString msg = tr(ADMIN_MSG_USAGE);

#ifdef Q_OS_WIN
    PMessageBox::information(NULL, Platform::ApplicationName(), msg);
#else
    QTextStream stout(stdout, QIODevice::WriteOnly);
    stout << msg << "\n";
#endif
}


void
MiniQApp::showError(QString msg)
{
#ifdef Q_OS_WIN
    PMessageBox::critical(NULL, Platform::ApplicationName(), msg);
#else
    QTextStream stout(stdout, QIODevice::WriteOnly);
    stout << msg << "\n";
#endif
}

/*!
    Warn about error in param 'i'
*/
void
MiniQApp::showParamError(int i)
{
    QString msg =  tr(ADMIN_MSG_BAD_PARAM)
        .arg( QString::fromUtf8(mArgv[i]) )
        .arg( Platform::IsUnix()
            ? "p4admin"
            : QString::fromUtf8(mArgv[0]));

    showError(msg);
}
