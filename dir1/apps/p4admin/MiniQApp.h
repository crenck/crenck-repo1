/// A QApplication that loads translation files for P4Admin

#include <QApplication>


/*!
    A QApplication subclass that loads P4Admin translators
*/
class MiniQApp : public QApplication
{
    Q_OBJECT

public:
    MiniQApp(int& argc, char** argv);

    void showVersionInfo(); //display build version/about info
    void showUsageInfo();   //display program usage info
    void showParamError(int i); // warn bad param
    void showError(QString);   //display error

private:
    int&    mArgc;
    char**  mArgv;
};
