//
// P4mPreferencesDialog is the dialog that allows the user to choose which files
// they would like to compare.
//

#ifndef __P4mPreferencesDialog_H__
#define __P4mPreferencesDialog_H__

#include "LanguageWidget.h"

#include <QCheckBox>
#include <QDialog>

class QComboBox;
class QLabel;
class QRadioButton;
class QSpinBox;
class QStackedWidget;
class QLineEdit;

/// \todo Write a Charset combo box to be used in the preferences dialog

class P4mPreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    P4mPreferencesDialog( QWidget * parent = 0, Qt::WindowFlags f = 0 );

protected:

    void closeEvent( QCloseEvent * );

private slots:
    void saveAndClose();
    void updateSettings();
    void showChooseAppFontDialog();
    void showChooseFontDialog();

    void setDiffOption( int );

private:

    QLabel *         appFontDescriptionLabel;
    QLabel *         fontDescriptionLabel;
    QRadioButton *   rb[4];
    QSpinBox *       mTabWidth;
    QStackedWidget * ws;
    QComboBox *      lineEndingPopup;
    QComboBox *      charSetPopup;
    QFont            mFont;
    QCheckBox *      mTabSpaces;
    QCheckBox *      mShowTabsSpaces;
    LanguageWidget * mAppLanguageWidget;
    QLineEdit *      mCompareUrl;

    QFont   mApplicationFont;

    QString describeFont( const QFont & );
};


#endif // __P4mPreferencesDialog_H__


