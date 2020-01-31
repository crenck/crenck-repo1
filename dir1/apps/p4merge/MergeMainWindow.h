#ifndef MergeMainWindow_h
#define MergeMainWindow_h

#include "PMessageBox.h"
#include "Responder.h"
#include <QDialog>
#include <QMainWindow>

#include "ActionStore.h"
#include "p4m.h"
#include "P4mPrefs.h"
#include "P4mArgs.h"

#include <QCheckBox>
#include <QSpinBox>

class P4mCoreWidget;
class P4mDocument;
class QCheckBox;
class QCloseEvent;
class QFileInfo;
class QFrame;
class QLabel;
class QProgressDialog;
class QToolBar;
class StringID;

class MergeMainWindow : public QMainWindow, public ResponderMainWindow
{
    Q_OBJECT

public:
    MergeMainWindow( QWidget * parent = 0 );
    ~MergeMainWindow();

     bool lineNumbersVisible();
     P4m::ReadErrors loadFiles( const QString & leftPath,
                                const QString & rightPath,
                                const QString & basePath     = QString());

    void overrideOptions( P4mArgs args );

    void setPrefDiffOption      ( int w )                    { mDiffOption = w; mRequestedDiffOption = w; };
    void setPrefGridOption      ( int w )                    { mGridOption = w; }
    void setPrefTabWidth        ( int w )                    { mTabWidth = w; };
    void setPrefLineEndings     ( P4mPrefs::LineEnding w )   { mLineEnding = w; };
    void setPrefEncoding        ( P4mPrefs::Charset w )      { mEncoding = w; };
    void setName                ( P4m::Player index, const QString & name);
    void setLabel               ( P4m::Player index, const QString & label);

    // Following two methods are public because they are used by ImgMainWindow
    static QString PathToName( const QString & path );
    static QString FileSizeWarningStr();

public slots:
    void close             (const QVariant & v = QVariant());
    bool save              (const QVariant &);
    bool saveAs            (const QVariant &);
    void chooseFont        (const QVariant &);
    void chooseTabWidth    (const QVariant &);
    void toggleToolbar     (const QVariant &);
    void refreshDoc        (const QVariant &);

    void setDiffOption          (const QVariant &);
    void setGridOption          (const QVariant &);
    void setLineEndings         (const QVariant &);
    void setEncoding            (const QVariant &);
    void setUnifiedViewShown    (const QVariant &);
    void setEditDiff            (const QVariant &);

    void toggleLineNumbersShown ( const QVariant & );

    void setSaveFile( QString );

    void setLineNumbersVisible( bool vis );

protected:
    void closeEvent( QCloseEvent * ae );
    void customizeAction( const StringID & cmd, QAction * );
    void updateCaption();

    /// resize window to fit the current desktop size
    virtual void resizeToFitDesktop();
    bool event( QEvent * );

private slots:
    void updateStatusBar();
    void toolBarToggled();
    void commitRequestedDocument();
    void showProgressDialog();
    void canceled();

private:
    void setEncoding( P4mPrefs::Charset );
    void setCustomGridOption( int );
    void setDocument( P4mDocument * );
    P4mDocument * document() const;

    // It's possible to have a main window that is not yet populated
    // and if so, we are just waiting to fill it.
    //
    P4mCoreWidget * cwb;

    QLabel * diffStatsLbl;
    QLabel * fileFormatLbl;
    QLabel * tabSpacingLbl;

    bool chooseSaveFile();
    bool needsSaving();
    bool requestSave( bool * wasSaved = NULL );
    bool saveDocument( const QString & );

    bool          makeDocumentThreaded();
    QFileInfo *   saveFile;

    QString  files[4];
    QString  names[4];
    QString  mLabels[4];

    QString totalDiffStr();
    QString fileFormatStr();
    QString diffFileFormatStr();
    QString tabSpacingStr();
    QString messageFilename();

    QFrame * mainWidget;
    QStatusBar * macStatusBar;
    QStatusBar * getStatusBar() const;

    ActionStore mToolbarStore;

    // TOOLBAR NOTES:
    //
    // toolbars are very tricky on the mac. There's only only one main area

    void loadToolbar( QToolBar * );
    bool toolbarVisible() const;
    void setToolbarVisible( bool );

    QToolBar* tb;
    QAction * mToolbarAction;
    bool      mMacToolBarAreaVisible; // This keeps track of the toolbar area on the mac

    bool checkDocument( int inCharSet, int *outCharSet );

    int     mDiffOption;
    int     mRequestedDiffOption; // If loading due to diff-option change
    int     mGridOption;
    int     mTabWidth;
    bool    mTabInsertsSpaces;
    bool    mShowTabsSpaces;
    P4mPrefs::LineEnding    mLineEnding;
    P4mPrefs::Charset       mEncoding;
    P4mDocument *           mDocument;

    void reloadDocument();

    P4mDocument        * mRequestedDocument;
    bool                 mDocumentCreationSuccessful;
    QProgressDialog    * mProgressDialog;
};

class SaveNotificationDialog : public PMessageBox
{
    Q_OBJECT

public:
    SaveNotificationDialog( const QString & text, const QString & filename, QWidget * parent = 0 );
};

class TabOptionsDialog : public QDialog
{
    Q_OBJECT

public:
    TabOptionsDialog( QWidget * parent = 0, Qt::WindowFlags f = 0 );

    int     tabWidth                () const    { return mTabWidth->value(); }
    bool    tabInsertsSpaces        () const    { return mTabInsertsSpaces->isChecked(); }
    bool    showTabsSpaces          () const    { return mShowTabsSpaces->isChecked(); }
    void    setTabWidth             ( int tw )  { mTabWidth->setValue(tw); }
    void    setTabInsertsSpaces     ( bool ts ) { mTabInsertsSpaces->setChecked(ts); }
    void    setShowTabsSpaces       ( bool ws ) { mShowTabsSpaces->setChecked(ws); }

private:
    QSpinBox  * mTabWidth;
    QCheckBox * mTabInsertsSpaces;
    QCheckBox * mShowTabsSpaces;
};

#endif // MergeMainWindow_h


