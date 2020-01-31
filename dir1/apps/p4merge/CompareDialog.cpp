#include "CompareDialog.h"

#include "DisplayLabel.h"
#include "FilePathLineEdit.h"
#include "ObjectNameUtil.h"
#include "P4mApplication.h"
#include "P4mPrefs.h"
#include "Platform.h"
#include "Prefs.h"
#include "Rule.h"
#include "SettingKeys.h"

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalMapper>
#include <QTabWidget>

#define MIN_LINEEDIT_WIDTH 300

QString mergeText[3][2];

QString diffText[2][2];

CompareDialog::CompareDialog( QWidget * parent, Qt::WindowFlags f )
    : QDialog( parent, f )
{
    static bool stringsLoaded = false;
    if ( !stringsLoaded )
    {
        mergeText [0][0] =  tr("&Base:");
        mergeText [0][1] =  tr("B&rowse...");
        mergeText [1][0] =  tr("&1st:");
        mergeText [1][1] =  tr("Br&owse...");
        mergeText [2][0] =  tr("&2nd:");
        mergeText [2][1] =  tr("Bro&wse...");

        diffText [0][0] = tr("&1st:");
        diffText [0][1] = tr("Br&owse...");
        diffText [1][0] = tr("&2nd:");
        diffText [1][1] = tr("Bro&wse...");

        stringsLoaded = true;
    }

    setWindowTitle( tr("Choose Files") );
    setSizeGripEnabled ( true );
    setModal( ! Platform::IsMac() );

    QPushButton* closeBtn = new QPushButton( tr( "Cancel" ), this );
    mCompareButton = new QPushButton(tr("OK"), this);
    closeBtn->setDefault( true );

    mTabs = new QTabWidget(this);
    mTabs->setFocusPolicy(Qt::NoFocus);

    //
    // NOTES ON LAYOUT
    // You will see some funny stuff in here and it's due to the way QPushButtons
    // behave on Mac OS X. Basically, they say their size is larger than they
    // actually look. They always have a thick 6 point margin around them when
    // they are drawn. This has the effect of making everything around them seem
    // far away because their size pushes everything in the layout around.
    // The best way around this is to butt them up against everything they
    // surrond with 0 margin. This actually makes them look like they are
    // a reasonable distance away.
    //
    // Since this dialog has a lot of QPushButtons, you will see different layout
    // for them on Mac OS X. In order for them to look right, I have to basically
    // set the spacing the layout to 0 which makes the buttons look right, but
    // then everything else has a margin of 0 so I have to add spacer items to
    // space them manually.
    //

    // DIFF PAGE
    //
    QWidget * page = new QWidget( );
    mTabs->addTab(page, diffTabLabel());

    // We use the signal mapper to map the buttons to a signal
    // that has an int as a parameter so we don't have to make so many slots.
    //
    QSignalMapper * buttonSM = new QSignalMapper( this );
    connect( buttonSM, SIGNAL( mapped( int ) ), this, SLOT( chooseFile( int ) ) );

    // Here, I combine creation, layout, and behavior since it is easier to
    // do inside the loop.
    //

    QGridLayout * gl = new QGridLayout( page );

    // Inner label inside tab pane
    //
    QLabel * lbl = new QLabel( tr("Choose files to diff:"), page );
    gl->addWidget( lbl, 0, 0, 1, 0 );

    QPushButton * diffChooseBtns[2];

    for (int i = 0; i < 2; i++)
    {
        // Creation
        //
        diffChooseBtns[i] = new QPushButton( diffText[i][1], page );
        mDiffLineEdits[i] = new FilePathLineEdit( page );
        mDiffLineEdits[i]->setAcceptDrops( true );

        DisplayLabel * dl = new DisplayLabel( page );
        dl->setText(diffText[i][0]);
        dl->setBuddy(mDiffLineEdits[i]);

        // Behavior
        //
        buttonSM->setMapping( diffChooseBtns[i], i+1 );
        connect( diffChooseBtns[i], SIGNAL( clicked() ), buttonSM, SLOT( map() ) );

        switch (i)
        {
            case 0:
                connect(mDiffLineEdits[0], &FilePathLineEdit::textChanged,
                        this,              &CompareDialog::setLeftPath);
                break;
            case 1:
                connect(mDiffLineEdits[1], &FilePathLineEdit::textChanged,
                        this,              &CompareDialog::setRightPath);
                break;
        }


        // Layout
        //
        gl->addWidget(dl, i + 1, 0);
        gl->addWidget(mDiffLineEdits[i], i + 1, 2);
        gl->addWidget(diffChooseBtns[i], i + 1, 3);

    }

    // Since the total items in the diff layout is shorter than the merge one,
    // we add this stretch and take up the slack.
    //
    gl->setRowStretch( 3, 2 );

    // Indicate that the QTextEdits need to stretch.
    //
    gl->setColumnStretch( 2, 2 );

    setTabOrder(mDiffLineEdits[0], diffChooseBtns[0]);
    setTabOrder(diffChooseBtns[0], mDiffLineEdits[1]);
    setTabOrder(mDiffLineEdits[1], diffChooseBtns[1]);
    setTabOrder(diffChooseBtns[1], mCompareButton);
    setTabOrder(mCompareButton, closeBtn);

    // MERGE PAGE
    //
    page = new QWidget();
    mTabs->addTab(page, mergeTabLabel());

    gl = new QGridLayout( page );

    // Indicate that the QTextEdits need to stretch.
    //
    gl->setColumnStretch( 2, 2 );

    // Inner label inside tab pane
    //
    lbl = new QLabel( tr("Choose files to merge:"), page );
    gl->addWidget( lbl, 0, 0, 1, 0 );

    // We use the signal mapper to map the buttons to a signal
    // that has an int as a parameter so we don't have to make so many slots.
    //
    buttonSM = new QSignalMapper(this);
    connect(buttonSM, SIGNAL(mapped    (int)),
            this,     SLOT  (chooseFile(int)));

    int offset = 1;
    QPushButton* chooseBtns[3];
    for (int i = 0; i < 3; i++)
    {
        // Creation
        //
        chooseBtns[i] = new QPushButton(mergeText[i][1], page);
        mLineEdits[i] = new FilePathLineEdit(page);
        mLineEdits[i]->setAcceptDrops( true );

        DisplayLabel * dl = new DisplayLabel(page);
        dl->setText(mergeText[i][0]);
        dl->setBuddy(mLineEdits[i]);

        // Behavior
        //
        buttonSM->setMapping(chooseBtns[i], i);
        connect(chooseBtns[i], SIGNAL(clicked()),
                buttonSM,      SLOT  (map    ()));

        switch (i)
        {
            case 0:
                connect(mLineEdits[0], &FilePathLineEdit::textChanged,
                        this,          &CompareDialog::setBasePath);
                break;
            case 1:
                connect(mLineEdits[1], &FilePathLineEdit::textChanged,
                        this,          &CompareDialog::setLeftPath);
                break;
            case 2:
                connect(mLineEdits[2], &FilePathLineEdit::textChanged,
                        this,          &CompareDialog::setRightPath);
                break;
        }

        gl->addWidget(dl, i+offset, 0);
        gl->addWidget(mLineEdits[i], i + offset, 2);
        mLineEdits[i]->setMinimumWidth(MIN_LINEEDIT_WIDTH);
        gl->addWidget(chooseBtns[i], i + offset, 3);
    }

    // LAYOUT
    //
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(mTabs);

    // Buttons
    //
    QDialogButtonBox* bbox = new QDialogButtonBox(this);
    bbox->addButton(closeBtn, QDialogButtonBox::RejectRole);
    bbox->addButton(mCompareButton, QDialogButtonBox::AcceptRole);
    ObjectNameUtil::ApplyStandardButtonNames(bbox);

    // BEHAVIOR
    //
    connect(closeBtn, &QPushButton::clicked,
            this,     &CompareDialog::close);
    connect(mCompareButton, &QPushButton::clicked,
            this,           &CompareDialog::compareClicked);
    connect(mTabs, &QTabWidget::currentChanged,
            this,  &CompareDialog::validateButtons);

    setTabOrder(mLineEdits[0], chooseBtns[0]);
    setTabOrder(chooseBtns[0], mLineEdits[1]);
    setTabOrder(mLineEdits[1], chooseBtns[1]);
    setTabOrder(chooseBtns[1], mLineEdits[2]);
    setTabOrder(mLineEdits[2], chooseBtns[2]);
    setTabOrder(chooseBtns[2], mCompareButton);
    setTabOrder(mCompareButton, closeBtn);

    mainLayout->addWidget( bbox );
    validateButtons();

    // Give the dialog a fixed height
    //
    QSize s = mainLayout->minimumSize();
    setMaximumHeight( s.height() );

    restoreGeometry(Prefs::Value(Prefs::FQ(p4merge::prefs::CompareWindowGeometry), QByteArray()));
}

QString
CompareDialog::diffTabLabel() const
{
    return tr("Diff");
}

QString
CompareDialog::mergeTabLabel() const
{
    return tr("Merge");
}

void
CompareDialog::setBasePath( const QString & s )
{
    setPath( s, 0 );
}

void
CompareDialog::setLeftPath( const QString & s )
{
    setPath( s, 1 );
}

void
CompareDialog::setRightPath( const QString & s )
{
    setPath( s, 2 );
}

void
CompareDialog::setPath( const QString & s, int i )
{
    if (i < 0 || i > 2)
        return;

    if (mPaths[i] == s)
        return;

    //
    // Only set the text on the line-edits if it is different
    // because it will reset the input context and move
    // the cursor to the end.
    //

    if (s != mLineEdits[i]->text())
        mLineEdits[i]->setText(s);

    mPaths[i] = s;

    if (i > 0 && i < 3 && s != mDiffLineEdits[i - 1]->text())
        mDiffLineEdits[i-1]->setText(s);

    validateButtons();
}

QString
CompareDialog::basePath() const
{
    if (mTabs->tabText(mTabs->currentIndex()) == diffTabLabel())
        return QString();
    return mPaths[0];
}

QString
CompareDialog::leftPath() const
{
    return mPaths[1];
}

QString
CompareDialog::rightPath() const
{
    return mPaths[2];
}

void
CompareDialog::closeEvent( QCloseEvent * e )
{
    Prefs::SetValue(Prefs::FQ(p4merge::prefs::CompareWindowGeometry), saveGeometry() );
    QDialog::closeEvent( e );
}


void
CompareDialog::chooseFile(int index)
{
    QString names[3] = { tr("Base"), tr("1st"), tr("2nd") };
    QString path = mLineEdits[index]->text();
    if (!path.isEmpty() && QDir(path).exists())
        path = mLineEdits[index]->text();
    else
        path = Prefs::Value(Prefs::FQ(p4merge::prefs::CompareDefaultPath), QString());

    path = QFileDialog::getOpenFileName(this, tr("Choose ") + names[index] + tr(" file"), path, QString());
    if (path.isNull())
        return;

    setPath(path, index);
    Prefs::SetValue(Prefs::FQ(p4merge::prefs::CompareDefaultPath), QFileInfo(path).dir().absolutePath());
}

void
CompareDialog::validateButtons()
{
    if (mTabs->tabText(mTabs->currentIndex()) == mergeTabLabel())
    {
        mCompareButton->setEnabled(!basePath().isEmpty() && !leftPath().isEmpty() && !rightPath().isEmpty());
        mCompareButton->setDefault(!basePath().isEmpty() && !leftPath().isEmpty() && !rightPath().isEmpty());
    }
    else
    {
        mCompareButton->setEnabled(!leftPath().isEmpty() && !rightPath().isEmpty());
        mCompareButton->setDefault(!leftPath().isEmpty() && !rightPath().isEmpty());
    }
}
