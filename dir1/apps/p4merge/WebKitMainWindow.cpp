#include "WebKitMainWindow.h"

#include "CommandIds.h"
#include "DialogUtil.h"
#include "HelpLauncher.h"
#include "IconAndTextLabel.h"
#include "MergeMainWindow.h"
#include "P4mApplication.h"
#include "P4mCommandGlobals.h"
#include "Platform.h"
#include "Prefs.h"

#include <QAction>
#include <QDesktopWidget>
#include <QDesktopServices>
#include <QDebug>
#include <QDir>
#include <QFrame>
#include <QHttpMultiPart>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProgressDialog>
#include <QTimer>
#include <QTemporaryFile>
#include <QUrl>
#include <QVBoxLayout>
#include <QWebView>


extern const char WK_WINDOW_GEOMETRY[] = "ApplicationSettings.xml:Windows/WebKitDiff/WindowGeometry";


WebKitMainWindow::WebKitMainWindow( QWidget * parent )
        : QMainWindow( parent ), ResponderMainWindow( this ),
        mProgressDialog(NULL)
{

    if ( !Platform::IsMac() && ! Platform::IsWin() )
    {
        setWindowIcon(QPixmap(":/p4mergeimage.png"));
    }

    QFrame* mainWidget = new QFrame( this );
    setCentralWidget( mainWidget );

    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainWidget->setLayout( mainLayout );

    mFile1Name  = new IconAndTextLabel(QPixmap(),
                                       QString(),
                                       false,
                                       this );
    mainLayout->addWidget(mFile1Name);

    mFile2Name = new IconAndTextLabel(QPixmap(),
                                      QString(),
                                      false,
                                      this );
    mainLayout->addWidget(mFile2Name);

    mWebView = new QWebView(this);
    connect( mWebView,   SIGNAL(loadFinished(bool)),  this, SLOT(deleteProgressDialog(bool)) );

#ifdef _DEBUG
    mWebView->page()->settings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);
#endif
    mainLayout->addWidget(mWebView, 500);

    mWebManager = new QNetworkAccessManager(this);
    mWebView->page()->setNetworkAccessManager(mWebManager);
    mWebView->page()->setLinkDelegationPolicy(QWebPage::DelegateExternalLinks);

    connect(mWebView->page(), SIGNAL(linkClicked (const QUrl&)),
            this,             SLOT  (linkClicked (const QUrl&)));

    QMenuBar * mb = Platform::GlobalMenuBar( false );
    if ( !mb )
        P4mApplication::app()->initMenuBar( menuBar() );

    resizeToFitDesktop();
}

WebKitMainWindow::~WebKitMainWindow()
{
    delete mWebView;
}


void
WebKitMainWindow::customizeAction( const StringID & cmd, QAction * a )
{
    ResponderMainWindow::customizeAction( cmd, a );

    if ( cmd == Cmd_SideBySide )
    {
        a->setEnabled ( false );
    }
    else if ( cmd == Cmd_Combined  )
    {
        a->setEnabled ( false );
    }
    else if ( cmd == Cmd_Link )
    {
        a->setEnabled ( false );
    }
    else if ( cmd == Cmd_FitToWindow )
    {
        a->setEnabled ( false );
    }
    else if ( cmd == Cmd_ActualSize )
    {
        a->setEnabled ( false );
    }
    else if ( cmd == Cmd_Help )
    {
        a->setEnabled ( true );
    }
    else if ( cmd == Cmd_ZoomIn )
    {
        a->setEnabled ( false );
    }
    else if ( cmd == Cmd_ZoomOut )
    {
        a->setEnabled ( false );
    }
    else if ( cmd == Cmd_Highlight )
    {
        a->setEnabled ( false );
    }
    else if ( cmd == Cmd_RefreshDoc )
    {
        a->setEnabled ( false );
    }

}


void
WebKitMainWindow::showHelp(const QVariant &)
{
    setObjectName("SETTINGS_DIFF_DOCX");
    P4mApplication::app()->helpWindow()->showHelp(this);
}


void
WebKitMainWindow::refreshDoc( const QVariant & )
{

}


void
WebKitMainWindow::closeEvent( QCloseEvent * e )
{
        Prefs::SetValue(Prefs::FQ(WK_WINDOW_GEOMETRY), saveGeometry());
        QMainWindow::closeEvent( e );
}


void
WebKitMainWindow::close( const QVariant & )
{
    QMainWindow::close();
}


void
WebKitMainWindow::updateCaption()
{
    QString caption;

                            //If p4merge has been passed labels for
                            //the file names, use these instead of the actual
                            //temp file names being passed in.
                            //This will eliminate having file#1.txt as the
                            //caption

    if ( Platform::IsMac() )
    {
        caption = tr( "Perforce %1" ).arg(Platform::ApplicationName());
    }
    else
    {
        const QString l = MergeMainWindow::PathToName ( !names[P4m::LEFT].isEmpty()
                                                   ? names[P4m::LEFT]
                                                   : files[P4m::LEFT] );

        const QString r = MergeMainWindow::PathToName ( !names[P4m::RIGHT].isEmpty()
                                                   ? names[P4m::RIGHT]
                                                   : files[P4m::RIGHT] );

        caption = tr ( "%1 and %2  - Perforce %3",
                       "Image window title. %1 is file on left, %2 is file on right" )
                  .arg( l, r, Platform::ApplicationName() );
    }

    setWindowTitle( caption );
}

void
WebKitMainWindow::addHttpPart( const QString& name, const QByteArray& value )
{
    QHttpPart httpPart;
    httpPart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("text/plain"));
    httpPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant(QString("form-data; name=\"%1\"").arg(name)));
    httpPart.setBody(value);
    mMultiPart->append(httpPart);
}

void
WebKitMainWindow::addHttpFilePart( const QString& filePath, const int index )
{
    QHttpPart filePart;
    QFileInfo fileInfo(filePath);

    QString name = QString("files[%1]").arg(index);
    filePart.setHeader(QNetworkRequest::ContentTypeHeader,
                        QVariant("application/vnd.openxmlformats-officedocument.wordprocessingml.document"));
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                        QVariant(QString("form-data; name=\"%1\"; filename=\"%2\"").arg(name).arg(fileInfo.fileName())));

    QFile *file = new QFile(filePath);
    file->open(QIODevice::ReadOnly);
    filePart.setBodyDevice(file);
    file->setParent(mMultiPart); // we cannot delete the file now, so delete it with the multiPart
    mMultiPart->append(filePart);

}

void
WebKitMainWindow::loadFiles( const QString & leftPath, const QString & rightPath, P4mArgs args )
{
    mArgs = args;
    QTimer::singleShot(0, this, SLOT(showProgressDialog()));

    files[P4m::LEFT] = leftPath;
    files[P4m::RIGHT] = rightPath;

    if (!args.names[P4m::LEFT].isNull())
        names[P4m::LEFT] = args.names[P4m::LEFT];
    if (!args.names[P4m::RIGHT].isNull())
        names[P4m::RIGHT] = args.names[P4m::RIGHT];

    updateCaption();

    QString leftLabel = !names[P4m::LEFT].isNull() ? names[P4m::LEFT]
                                                   : files[P4m::LEFT];

    QString rightLabel = !names[P4m::RIGHT].isNull() ? names[P4m::RIGHT]
                                                     : files[P4m::RIGHT];


   if (mArgs.compareType.isNull() || mArgs.compareType == "compare-siblings")
   {
        mFile1Name->setPixmap(QPixmap( ":/merge_right.png"));
        mFile2Name->setPixmap(QPixmap( ":/merge_left.png"));
        rightLabel.prepend(tr("<b> File 1:</b> "));
        leftLabel.prepend(tr("<b> File 2:</b> "));
   }

    mFile1Name->setText(rightLabel);
    mFile2Name->setText(leftLabel);

    QString urlParam = args.compareUrl;
    QString urlPref = Prefs::Value( Prefs::FQ( MergePrefs::DOCXCOMPAREURL ), QString() );
    if (urlParam.isEmpty() && (!urlPref.isNull() && !urlPref.isEmpty()))
        urlParam = urlPref;

    QString compareUrl = urlParam.append("/rich-compare/v1/docx/");
    QUrl url(compareUrl);

    QNetworkRequest request(url);

    mMultiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    addHttpPart(QString("protocol_version"), QByteArray("render-html5-1.0"));
    addHttpPart(QString("theme"),            QByteArray("default"));
    addHttpPart(QString("asset_prefix"),     QByteArray("/assets"));
    addHttpPart(QString("suffix_file1"),     QByteArray(tr("File 1").toUtf8()));
    addHttpPart(QString("suffix_file2"),     QByteArray(tr("File 2").toUtf8()));
    addHttpPart(QString("base_url"),         QByteArray(urlParam.toUtf8()));
    if (!args.compareType.isNull())
        addHttpPart(QString("action_to_perform"),QByteArray(args.compareType.toUtf8()));

    addHttpFilePart(leftPath, 0);
    addHttpFilePart(rightPath, 1);

    mNetworkReply = mWebManager->post(request, mMultiPart);
    mMultiPart->setParent(mNetworkReply); // delete the multiPart with the reply

    connect(mNetworkReply, SIGNAL(readyRead()),
            this,          SLOT  (dataReady()));

    connect(this,   SIGNAL(networkError      (const QString& )),
            this,   SLOT  (handleNetworkError(const QString&)));

    connect(mNetworkReply, SIGNAL(finished ()),
            this,          SLOT  (replyFinished()));

    if (!restoreGeometry(Prefs::Value(Prefs::FQ(WK_WINDOW_GEOMETRY), QByteArray())))
    {
        QRect r = QDesktopWidget().availableGeometry( this );
        resize( 400, 800 );
        move( r.topLeft() );
    }
    updateCaption();
}

void
WebKitMainWindow::linkClicked (const QUrl& url)
{
    if (url.scheme().contains("data"))
    {
        QStringList list = url.toString().split("base64,");
        QString extension;
        if (list.at(0).contains("wordprocessingml.document"))
            extension = ".docx";
        else
            extension = ".xml";

        QByteArray docData = QByteArray::fromBase64(list.at(1).toUtf8());

        QString name = QDir::tempPath() + "/p4merge_download_XXXXXX" + extension;
        {
            QTemporaryFile file(name);
            file.setAutoRemove(false);
            if (file.open())
            {
                name = file.fileName();
                file.write(docData);
            }
        }
        QUrl fileUrl("file:///" + name);
        QDesktopServices::openUrl(fileUrl);
    }
    else
        QDesktopServices::openUrl(url);
}

void
WebKitMainWindow::showProgressDialog()
{
    if ( !mProgressDialog && !mNetworkReply->isFinished())
    {
        mProgressDialog = new QProgressDialog( this );
        mProgressDialog->setMinimumDuration( 0 );
        mProgressDialog->setLabelText(tr("Loading files..."));
        mProgressDialog->setRange(0,0);

        mProgressDialog->setWindowModality(isVisible() ? Qt::WindowModal
                                                       : Qt::ApplicationModal);
        if ( !Platform::IsMac() )
        {
            mProgressDialog->setWindowTitle(Platform::ApplicationName());
            if (isVisible())
            {
                mProgressDialog->setWindowModality(Qt::WindowModal);
                mProgressDialog->setWindowFlags(WinFlags::SheetFlags());
            }
            else
            {
                mProgressDialog->setWindowFlags(WinFlags::CloseOnlyFlags());
            }
        }
        DialogUtil::HideSizeGrip(*mProgressDialog, QSize(250, 100));
        mProgressDialog->show();
        connect( mProgressDialog,   SIGNAL(canceled()),  this, SLOT(cancelProgressDialog()) );
    }
}

void
WebKitMainWindow::cancelProgressDialog()
{
    deleteProgressDialog(true);
    qApp->activeWindow()->close();
}


void
WebKitMainWindow::deleteProgressDialog(bool)
{
    if ( mProgressDialog )
    {
        delete mProgressDialog;
        mProgressDialog = NULL; // required so timer callback doesnt redisplay dialog
    }
}

void
WebKitMainWindow::handleNetworkError(const QString& errorString)
{
    deleteProgressDialog(true);
    PMessageBox msgBox( QMessageBox::Warning,
                    Platform::ApplicationName(),
                    errorString,
                    QMessageBox::Ok,
                    this);

    msgBox.setWindowModality(Qt::WindowModal);
    msgBox.setWindowFlags(WinFlags::SheetFlags());
    msgBox.exec();
    qApp->activeWindow()->close();
}

void
WebKitMainWindow::dataReady()
{
    QNetworkReply* reply = static_cast<QNetworkReply*>(sender());
    mData = mData.append(reply->readAll());
}

void
WebKitMainWindow::replyFinished()
{

    QNetworkReply* reply = static_cast<QNetworkReply*>(sender());
    if (reply->isRunning())
        return;

    if (reply->error() != QNetworkReply::NoError)
        emit networkError(reply->errorString());
    else
    {
        if (mArgs.compareType.isNull() || mArgs.compareType == "compare-siblings")
        {
            mData.append(leg1IconProperties().toUtf8());
            mData.append(leg2IconProperties().toUtf8());
        }
        mWebView->setHtml(QString::fromUtf8(mData.constData()));
    }
}

/** resizeToFitDesktop
    \brief  Resize the window to fit within the bounds of the indicated screen.
*/
void
WebKitMainWindow::resizeToFitDesktop()
{
// Ensure the window isn't bigger than its screen.
    QDesktopWidget *desk = qApp->desktop();
    QRect   deskRect = desk->screenGeometry(this);
    QRect   frameRect = frameGeometry();
    int     frameW = frameRect.width() - width();
    int     frameH = frameRect.height() - height();
    if (frameRect.width() > deskRect.width())
        resize(deskRect.width() - frameW, height());
    if (frameRect.height() > deskRect.height())
        resize(width(), deskRect.height() - frameH);
}

QString
WebKitMainWindow::commonIconProperties()
{
    return " width: 20px; height: 20px; padding-right: 5px;";
}

QString
WebKitMainWindow::leg1IconProperties()
{

    QColor color =     QColor::fromRgb(217, 255, 217);
    QColor highlight = QColor::fromRgb(183, 215, 183);

    QString props = "<style type=\"text/css\">.icon-added_leg1 { ";
            props.append(commonIconProperties());
            props.append(" background-image: url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABQAAAAUCAYAAACNiR0NAAAABGdBTUEAANbY1E9YMgAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAANOSURBVHjaYvz//z8DNQFAADExUBkABBDVDQQIIKobCBBALPgkQ6cEmTIyMfoy/P3v8OfPX50/P//+//fr3+V/f/4d+Pf779Yd7btPo+sBCCBGXJESPj0kk02UsUJIlkVOUViYQYiVl+HLzy8M117cY7hw7T4D4yO++2wfODs18x/OmqJ+D24IQACx4DJMWIWr30ZLn11LQJPhH8N/hn9//jJ8/P2WgQOog4XtGcN51heKH2/y919qlmJgWMIwE6YXIICYsHmTWYihwlJTm91Q2JRBkF0SiMUYuNkEGHjZhBj4OAQZeLn4GcTF/jCwSb7j/Mr+udwy0MIEph8ggDAMBIUZt9RvOVVeFQYOJi4GdiZ2BmYmoLOYfzP8YnrJ8Jv5FcOP/+8YmBkYGQR5/zL85P2g+PfnPx+YfoAAwvQyMAJE+VkZ/v9nYvj/l4HhJ+NPht8MPxi+/L3M8P7POYYnv64ADfzM8Ovvf4Y/3xgY2Dh/M/z/9d8Bph0ggDAMBMWmGAs/w99f/xi+M/1g+PPvO8PP/zcY3vw9zHD/212G99++Mvz4DlT38z/Dz1//GTjZGBi+//qrC9MPEECYBgKTxptvLxle/3gAdOxPhh8sdxg+/rvA8PzHfYZX378xfPr6j+Hbl78MPz4DXfiVkeH/P2CEAS2HAYAAwjAQlM6uPHtsL8R5luHdb16GH4xPGV7+ecTw49cPhq8//jJ8+vSX4ev7fww/PjCADfvyBZhi/vy7DNMPEECYBv7+d+Dlg9/217kvMfBx8jF8//+W4TfjL4Y/QFf8/MnI8O3jb4ZPL/8z/PgITD6MDAzf3jIxAIPxAEw/QABhGPj3598tzM/5466xvFAUE/rAwMwGTv4Mf38zMPz69p/hG9BlXz/8Z2AGOuzDh38MjI95H/z7+28LTD9AAGEYyCJ7+yz7dcWuD7/5+p98es8hwAVM1kCX/P8DjIRvkMhgAQbZx49AV97j+MHxir3j6tUrZ2D6AQIIZ9azi7FJ/8ryufwH33tFFvbfDFwc/8HJCBRmP94DU+ETrgdsYMOuzkTWBxBAjPgKWFAOACXa/7//OwCDQhcUm0DvXf7/i+EAyJtXryBcBgMAAcRI7RIbIICoXh4CBBgA2I97U8qjZQIAAAAASUVORK5CYII=); } .added_leg1 { background-color: ");
            props.append(color.name());
            props.append("; }  .selected.added_leg1, .highlight.added_leg1 { background-color: ");
            props.append(highlight.name());
            props.append("; } </style>");
    return props;
}

QString
WebKitMainWindow::leg2IconProperties()
{
    QColor color =     QColor::fromRgb(236, 236, 255);
    QColor highlight = QColor::fromRgb(198, 198, 215);

    QString props = "<style type=\"text/css\">.icon-added_leg2 { ";
    props.append(commonIconProperties());
    props.append(" background-image: url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABQAAAAUCAYAAACNiR0NAAAABGdBTUEAANbY1E9YMgAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAANtSURBVHjaYvz//z8DNQFAADExUBkABBDVDQQIILwGTpmylbGurpKZEQhA/EmT1jDp6Cgx49MDEECM6GFobNzAZG2tqiIkxBrCysrm8vv3H/1v337y/v37n+Xfv3+Mnz59//Hhw4+bnz59ufzt29/dQKuOAOmH5841/gXpBwggFnQbpKR4LRgZ/3bx8YlYKyqK/RcQ4PvDw8PJ+u8fC8OLF18Znj37wvHkyQf9Fy/eAvHTmLdv395kZmauBGpdD9IPEEAYBmpqSk3W0pI10tFR/S0lJcrKwsLAysDwl+H7938MLCxcDIyM/AwfP/IwfPrEx8DPzwukb6j//fuyE2YgQABhGPjnzz9VCQkJBj4+Ptb///8xgEKPiek/AxvbfzD98eMvoCG/GN6//wbEn4HybAw/fryRgOkHCCCMSPn9+//thw9fAhX9Yfj27R/Dr1//GYBhB7ToD9CgHwyfP/8AuvYnUO47kP8ZaMFbhr9/Ga/A9AMEEBOmC39OunnzKcPLl18Z3rz5wfDhwx+Gt2//AMPuJzAMQfyfQEO+AQ3+CjT4G9ABX4Eu/7sGph8ggDC8/O7dkwMsLFJvbtx4IiInJw90BQPQlT+BBvwEWvKD4fXrb2ADv379CvTFF6CXmd58//56N0w/QABhGMjEJPPk9+/v/Vev3m9lZhb+x8z8j+n//99AQ74zvHr1DYi/AtlfGH7+/A4Mih9A9d8OMjH9uAPTDxBALJhJ8/+/r1/fr/v+nTGalfWelpSUKtA1P4Eu/Qb0+hdgMHwCh923b2+Arvv/5tu3O92srP9/wHQDBBBGGC5fnvKfl1fy3s+fX2ffu3fj48uXb96/fPkNGH6fgN79AIzZ90AL3gDD+hPQyy8PsLL+vnn79jZ47gAIIEZcpY2X13LxT58uN3JxyaZzcyv//PLlO/unT++BEfERaNA7kE++/vhxwe7Ro03nkPUBBBDOvLxtW+RLDg7mntevLxx79uwy+9evL4Ax+hbosjdA2X/f//69Uv3ixedb6PoAAoiRUHlobT1P782bfcu4uNS0f//+CUqPP3/8eNDOwPC+78GDXZ/R1QMEECMxBayOToju58/fpjMyshoAs2EzM/PfGXfvbv+ITS1AADESW2KrqHgqsrExyf/8+e8C0LAPuNQBBBAjtasAgAADAIqBvaJ7I3EnAAAAAElFTkSuQmCC); } .added_leg2 { background-color: ");
    props.append(color.name());
    props.append("; }  .selected.added_leg2, .highlight.added_leg2 { background-color: ");
    props.append(highlight.name());
    props.append("; } </style>");
    return props;
}


