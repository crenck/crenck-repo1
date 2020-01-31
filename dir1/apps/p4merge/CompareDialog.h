//
// This file is part of Perforce - the FAST SCM System.
//
//
// CompareDialog
// --------------
//
// CompareDialog is the dialog that allows the user to choose which files
// they would like to compare.
//

#ifndef __CompareDialog_H__
#define __CompareDialog_H__

#include <QDialog>

class QLineEdit;
class QPushButton;
class QTabWidget;

class CompareDialog : public QDialog
{
    Q_OBJECT
public:
    CompareDialog( QWidget * parent = 0, Qt::WindowFlags f = 0 );

    QString basePath() const;
    QString leftPath() const;
    QString rightPath() const;

public slots:
    void setBasePath( const QString & s );
    void setLeftPath( const QString & s );
    void setRightPath( const QString & s );

signals:
    void compareClicked();

protected:
    void closeEvent( QCloseEvent * e );

private slots:
    void chooseFile( int );
    void validateButtons();

private:
    QString diffTabLabel() const;
    QString mergeTabLabel() const;

    void setPath(const QString & s, int);

    QPushButton* mCompareButton;
    QTabWidget* mTabs;

    QLineEdit* mLineEdits[3];
    QLineEdit* mDiffLineEdits[2];

    QString mPaths[3];
};


#endif // __CompareDialog_H__


