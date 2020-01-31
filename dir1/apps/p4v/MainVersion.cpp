#include "SandstormApp.h"

#include "Platform.h"

#ifdef Q_OS_WIN
#include "UIAboutDialog.h"
#include "WinFlags.h"

#include "PMessageBox.h"

#include <QApplication>
#include <QDialog>
#include <QResource>
#endif

#include <QTextStream>

int mainVersion(int argc, char** argv)
{
#ifdef Q_OS_WIN
    // On Windows, since p4v is built with subsystem:windows, we can't
    // send anything to the console in a manner that's supported on all
    // versions of the OS.  So, we just put up the about dialog instead.
    QApplication app(argc, argv);

    if (!QResource::registerResource(Platform::ImageResourceFile()))
    {
        PMessageBox::critical(0, Platform::LongApplicationName(),
            QApplication::tr("Error initializing %1.\nUnable to load resource file.").arg(Platform::LongApplicationName()));
        qWarning("error loading resource file");
        return false;
    }

    UIAboutDialog* about = new UIAboutDialog(":/p4vimage.png", qApp->activeWindow(),WinFlags::WindowFlags());
    about->setSizeGripEnabled(false);
    about->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    about->exec();
#else
    Q_UNUSED(argc);
    Q_UNUSED(argv);
#endif

    QString msg = SandstormApp::tr(
        "%1 - The %2\n"
        "Copyright 1995-%3 Perforce Software.\n"
        "All rights reserved.\n"
        "Rev. %4"
        )
        .arg(Platform::ApplicationName())
        .arg(Platform::LongApplicationName())
        .arg(Platform::BuildYear())
        .arg(Platform::ApplicationNameAndVersion());
    QTextStream stout(stdout, QIODevice::WriteOnly);
    stout << msg << "\n";
    return 0;
}
