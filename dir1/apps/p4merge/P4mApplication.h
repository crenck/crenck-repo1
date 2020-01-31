#ifndef __P4mApplication_H__
#define __P4mApplication_H__

#include "Responder.h"
#include <QApplication>
#include <QDialog>

#include <QSysInfo>
#include <QVariant>

class AboutBox;
class CompareDialog;
class HelpLauncher;
class P4mArgs;
class QMenuBar;
class QSettings;
class QStringList;
class StringID;
class Targetable;

class P4mApplication : public QApplication, public Responder
{
    Q_OBJECT
public:
    enum GridFlags { Optimal = 0, Guarded };

    P4mApplication ( int & argc, char ** argv );
    ~P4mApplication ();

    static P4mApplication * app() { return (P4mApplication *)qApp; };

    void loadGlobalActions();
    void initMenuBar( QMenuBar * );

    QMainWindow* createMainWindow( QString file1, QString file2, P4mArgs * args = NULL );
    QMainWindow* createMainWindow( QString file1, QString file2, QString file3, P4mArgs * args = NULL );

    void hideCompareFilesDialog();

    HelpLauncher * helpWindow();

    void customizeAction( const StringID & cmd, QAction * a);

    virtual bool notify(QObject *obj, QEvent *e);

    void resetZeroArguments() { mZeroArguments = false; }

    GridFlags gridOption() { return mGridOption; }

public slots:
    void showAbout   (const QVariant & v = QVariant());
    void showPrefs   (const QVariant &);
    void showHelp    (const QVariant &);
    void openDocument(const QVariant &);
    void close       (const QVariant &);
    void quit        (const QVariant &);

    void showCompareDialog();
    void scheduleShowCompareDialog();

    void testLightWeightText(const QVariant &);
    bool processArguments(const QStringList& args);

private slots:
    void dialogCompared();

private:
    CompareDialog *         mCompareFilesDialog;
    QPointer<AboutBox>      mAboutBox;
    HelpLauncher *          mHelpLauncher;
    bool                    mZeroArguments;
    GridFlags               mGridOption;

    bool allWindowsClosed();
    QString realApplicationDirPath();

    bool beforeLeopard()
    {
        #ifdef Q_OS_OSX
            // Version enum only available on actual platform
            return (QSysInfo::MacintoshVersion < QSysInfo::MV_10_5 );
        #endif
        return false;
    }
};


class CharSetConfirmationDialog : public QDialog
{
    Q_OBJECT
public:
    CharSetConfirmationDialog( int p4charSet, int alternateCharSet, bool isDiff, QWidget * parent = 0 );
    int getNewCharSet() const;

protected:
    void done(int r) {
        hide();
        setResult(r);
    }

public slots:
    void charSetChanged();
    void continueWithOrigCharSet(int);

private:
    int newCharSet;
    int origCharSet;
    class CharsetCombo * combo;
};

#endif // __P4mApplication_H__


