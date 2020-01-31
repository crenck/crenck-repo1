#ifndef __ImgMainWindow_H__
#define __ImgMainWindow_H__

#include "Responder.h"
#include <QMainWindow>

#include "ImgDiffController.h"
#include "P4mArgs.h"

class QAction;
class QCloseEvent;
class StringID;

class ImgMainWindow : public QMainWindow, public ResponderMainWindow
{
    Q_OBJECT
public:
    ImgMainWindow(QWidget * parent = 0);
    ~ImgMainWindow();

    QString loadFiles(const QString & leftPath,
                      const QString & rightPath);

    void overrideOptions( P4mArgs args );

public slots:
    void setSideBySideViewShown (const QVariant &);
    void setUnifiedViewShown    (const QVariant &);
    void linkImages             (const QVariant &);
    void fitToWindow            (const QVariant &);
    void actualSize             (const QVariant &);
    void zoomIn                 (const QVariant &);
    void zoomOut                (const QVariant &);
    void setHighlightDiffs      (const QVariant &);
    void close                  (const QVariant &);
    void showHelp               (const QVariant &);
    void refreshDoc             (const QVariant &);

protected:
    void closeEvent             (QCloseEvent* ae);
    void customizeAction        (const StringID& cmd, QAction*);
    void updateCaption          ();

    /// resize window to fit the current desktop size
    virtual void resizeToFitDesktop();

private:
    QString files[4];
    QString names[4];

    ImgDiffController* mDiffController;
};

#endif // __ImgMainWindow_H__
