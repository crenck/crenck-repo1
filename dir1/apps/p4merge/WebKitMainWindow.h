#ifndef __WebKitMainWindow_H__
#define __WebKitMainWindow_H__

#include <QMainWindow>
#include "Responder.h"

#include "P4mArgs.h"

class IconAndTextLabel;
class P4mCoreWidget;
class P4mDocument;
class QCloseEvent;
class QFileInfo;
class QFont;
class QFrame;
class QHttpMultiPart;
class QLabel;
class QNetworkAccessManager;
class QNetworkReply;
class QProgressDialog;
class QUrl;
class QWebView;
class StringID;

class WebKitMainWindow : public QMainWindow, public ResponderMainWindow
{
    Q_OBJECT

public:
    WebKitMainWindow( QWidget * parent = 0 );
    ~WebKitMainWindow();

    void loadFiles( const QString & leftPath,
                    const QString & rightPath,
                    P4mArgs args);
public slots:

    void close                      (const QVariant &);
    void showHelp                   (const QVariant &);
    void refreshDoc                 (const QVariant &);

private slots:
    void replyFinished              ();
    void linkClicked                (const QUrl&);
    void dataReady                  ();
    void handleNetworkError         (const QString&);
    void showProgressDialog();
    void deleteProgressDialog       (bool b);
    void cancelProgressDialog       ();
    QString commonIconProperties    ();
    QString leg1IconProperties      ();
    QString leg2IconProperties      ();

protected:
    void closeEvent     ( QCloseEvent * ae );
    void customizeAction( const StringID & cmd, QAction * );
    void updateCaption  ();
    void addHttpPart    (const QString& name, const QByteArray& value);
    void addHttpFilePart(const QString& name, const int index);

    /// resize window to fit the current desktop size
    virtual void resizeToFitDesktop();

signals:
    void networkError(const QString&);

private:
    P4mArgs mArgs;
    QString  files[4];
    QString  names[4];
    QWebView *mWebView;
    QNetworkAccessManager *mWebManager;
    QNetworkReply *mNetworkReply;
    QByteArray mData;
    QHttpMultiPart *mMultiPart;
    QProgressDialog *mProgressDialog;
    IconAndTextLabel* mFile1Name;
    IconAndTextLabel* mFile2Name;
};


#endif // __WebKitMainWindow_H__
