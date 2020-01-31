#include "P4mPreferencesDialog.h"

#include "DisplayLabel.h"
#include "ObjectNameUtil.h"
#include "P4mPrefs.h"
#include "Platform.h"
#include "Prefs.h"
#include "SettingKeys.h"

#include <QBoxLayout>
#include <QButtonGroup>
#include <QCloseEvent>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFontDialog>
#include <QGroupBox>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QSignalMapper>
#include <QShortcut>
#include <QStackedWidget>

P4mPreferencesDialog::P4mPreferencesDialog( QWidget * parent, Qt::WindowFlags flags )
    : QDialog( parent, flags )
{
    setModal(true);
    setWindowTitle( tr("Preferences") );
    setObjectName("P4mPreferencesDialog");


    if (Platform::IsUnix())
        mApplicationFont = Prefs::Value( Prefs::FQ( MergePrefs::APPFONT ), QFont() );

    mFont = Prefs::Value( Prefs::FQ( MergePrefs::FONT ), QFont() );

    bool useSpaces      = Prefs::Value( Prefs::FQ( MergePrefs::TABINSERTSSPACES ), false );
    bool showTabsSpaces    = Prefs::Value( Prefs::FQ( MergePrefs::SHOWTABSSPACES ), false );
    // TOP-LEVEL LAYOUT
    //
    QVBoxLayout * mainLayout = new QVBoxLayout( this );

    mainLayout->setSizeConstraint( QLayout::SetFixedSize );


    //
    // WIDGET CREATION ------------------------------------------
    //

    // Application Font Choice
    //

    // Font / Tab Width
    //
    QLabel * appfontLabel;
    QPushButton * appfontButton = NULL;
    if (Platform::IsUnix())
    {
        QWidget* appfontWidget  = new QWidget;
        QHBoxLayout* appfontlayout = new QHBoxLayout;
        appfontlayout->setContentsMargins(6,0,10,0);

        appfontLabel = new DisplayLabel( appfontWidget, tr("Application font:") + "   " );
        appFontDescriptionLabel = new QLabel( appfontWidget );
        appFontDescriptionLabel->setObjectName( "fontDescriptionLabel" );
        appfontButton = new QPushButton( tr("C&hoose..."), appfontWidget );

        appfontlayout->addWidget(appfontLabel);
        appfontlayout->addWidget(appFontDescriptionLabel);
        appfontlayout->setStretchFactor(appFontDescriptionLabel, 150);
        appfontlayout->addWidget(appfontButton);
        appfontWidget->setLayout(appfontlayout);

        mainLayout->addWidget(appfontWidget);
    }

    QGroupBox * formattingGroup = new QGroupBox( tr("Text format (default):"), this );
    formattingGroup->setFlat( true );
    formattingGroup->setLayout( new QVBoxLayout( formattingGroup ) );
    formattingGroup->setObjectName( "formattingGroup" );

    // Tab Width
    //
    QLabel * tabWidthLabel = new DisplayLabel( formattingGroup, tr("&Tab spacing:") + "   " );

    mTabWidth = new QSpinBox( formattingGroup );
    tabWidthLabel->setBuddy( mTabWidth );
    mTabWidth->setRange(1, 99);
    mTabWidth->setValue(Prefs::Value(Prefs::FQ( MergePrefs::TABWIDTH ), 0 ) );

    // Use spaces for tabs
    //
    mTabSpaces = new QCheckBox( tr("In&sert spaces instead of tabs"), formattingGroup );
    mTabSpaces->setChecked( useSpaces );

    // Show tabs and spaces
    //
    mShowTabsSpaces = new QCheckBox( tr("Sh&ow tabs and spaces"), formattingGroup );
    mShowTabsSpaces->setChecked( showTabsSpaces );

    // Font Choice
    //
    QLabel * fontLabel = new DisplayLabel( formattingGroup, tr("Font:") + "   " );
    fontDescriptionLabel = new QLabel( formattingGroup );
    fontDescriptionLabel->setObjectName( "fontDescriptionLabel" );
    QPushButton * fontButton = new QPushButton( tr("&Choose..."), formattingGroup );
    fontButton->setObjectName( "fontButton" );

    // File Format
    //
    QGroupBox * fileFormatGroup = new QGroupBox( tr("File format (default):"), this );
    fileFormatGroup->setLayout( new QVBoxLayout( fileFormatGroup ) );
    fileFormatGroup->setFlat( true );
    fileFormatGroup->setObjectName( "formattingGroup" );

    // Char set
    //
    QLabel * charSetLabel = new QLabel( tr("Character &encoding:"), fileFormatGroup );
    charSetPopup = new QComboBox( fileFormatGroup );
    charSetLabel->setBuddy( charSetPopup );
    charSetLabel->setObjectName( "charSetLabel" );

    for ( int i = P4mPrefs::System; i < P4mPrefs::Count; i++ )
        charSetPopup->addItem( P4mPrefs::CharSetDescription( (P4mPrefs::Charset)i, P4mPrefs::ComboBoxText ) );

    charSetPopup->setCurrentIndex(  P4mPrefs::ToCharset( Prefs::Value( Prefs::FQ( MergePrefs::CHARSET ), QString() ) ) );


    QLabel * lineEndingLabel = new QLabel( tr("&Line ending type (merge file only):"), fileFormatGroup );
    lineEndingPopup = new QComboBox( fileFormatGroup );
    for ( int i = 0; i < 3; i++ )
        lineEndingPopup->addItem( P4mPrefs::LineEndingDescription( (P4mPrefs::LineEnding)i, P4mPrefs::ComboBoxText ) );
    lineEndingLabel->setBuddy( lineEndingPopup );
    lineEndingLabel->setObjectName( "lineEndingLabel" );
    lineEndingPopup->setCurrentIndex( P4mPrefs::ToLineEnding( Prefs::Value( Prefs::FQ( MergePrefs::LINEENDING ), QString() ) ) );


    // Application Language
    //
    QGroupBox * languageGroup = new QGroupBox( tr("Application Language (default):"), this );
    languageGroup->setLayout( new QVBoxLayout( languageGroup ) );
    languageGroup->setFlat( true );
    languageGroup->setObjectName( "languageGroup" );

    mAppLanguageWidget = new LanguageWidget(languageGroup);
    mAppLanguageWidget->setCurrentLanguage(LanguageWidget::LanguagePref());

    // Diff Options
    //
    QGroupBox * diffGroupBox = new QGroupBox( tr("Comparison method (default):"), this );
    diffGroupBox->setObjectName( "diffGroupBox" );
    diffGroupBox->setFlat( true );

    QButtonGroup * diffButtonGroup = new QButtonGroup( diffGroupBox );
    QSignalMapper * sm = new QSignalMapper( diffButtonGroup );

    QString buttonTitles[4] =
        { tr("&Recognize line ending and white space differences"),
         tr("&Ignore line ending differences"),
         tr("I&gnore line ending and white space length differences"),
         tr("Ig&nore line ending and all white space differences") };

    QString diffExamples[4] =
        { tr("<br><tt>sample example text</tt>&lt;cr&gt;<br>"
        "<tt>sample example text</tt>&lt;cr&gt;<br>"),

         tr("<br><tt>sample example text</tt><font color=\"blue\">&lt;cr&gt;</font><br>"
         "<tt>sample example text</tt><font color=\"blue\">&lt;cr&gt;&lt;lf&gt;</font><br>"),

         tr("<br><tt>sample example text</tt><font color=\"blue\">&lt;cr&gt;</font><br>"
         "<tt>sample</tt><font color=\"blue\">&lt;tab&gt;</font><tt>example text</tt><font color=\"blue\">&lt;cr&gt;&lt;lf&gt;</font><br>"),

         tr("<br><tt>sample <font color=\"blue\">example text</font></tt><font color=\"blue\">&lt;cr&gt;</font><br>"
         "<tt>sample</tt><font color=\"blue\">&lt;tab&gt;</font><tt><font color=\"blue\">exampletext</font></tt><font color=\"blue\">&lt;cr&gt;&lt;lf&gt;</font><br>") };

    QLabel * exampleLabel = new QLabel( tr("Example:"), diffGroupBox );
    exampleLabel->setObjectName( "exampleLabel" );

    ws = new QStackedWidget( diffGroupBox );
//    ws->setStyleSheet( "QStackedWidget { background-color: white; }" );
    ws->setAutoFillBackground ( true );

    for ( int i = 0; i < 4; i++ )
    {
        rb[i] = new QRadioButton( buttonTitles[i], diffGroupBox );
        diffButtonGroup->addButton( rb[i] );
        sm->setMapping( rb[i], i );
        ws->insertWidget( i, new QLabel( diffExamples[i], ws ) );
        connect( rb[i], SIGNAL(clicked()), sm, SLOT(map()) );
        connect( sm, SIGNAL(mapped(int) ), ws, SLOT( setCurrentIndex(int) ) );
    }
    QLabel * linesEqualLabel = new QLabel( tr("Blue shows <font color=\"blue\">ignored differences</font>"), diffGroupBox );
    linesEqualLabel->setAlignment( Qt::AlignHCenter );
    linesEqualLabel->setObjectName( "linesEqualLabel" );

    // DOCX Compare Url
    //
    QGroupBox * compareWebServiceGroup = new QGroupBox( tr("Web Services:"), this );
    QLabel * compareUrlLabel = new DisplayLabel( compareWebServiceGroup, tr("Microsoft Word") + QChar( 0x2122) + tr(" Compare URL:") + "   " );

    compareWebServiceGroup->setLayout( new QVBoxLayout( compareWebServiceGroup ) );
    compareWebServiceGroup->setFlat( true );
    compareWebServiceGroup->setObjectName( "compareWebServiceGroup" );

    mCompareUrl = new QLineEdit( compareWebServiceGroup );
    mCompareUrl->setPlaceholderText(tr("http://host:port"));
    mCompareUrl->setText(Prefs::Value(Prefs::FQ( MergePrefs::DOCXCOMPAREURL ), QString() ) );
    compareUrlLabel->setBuddy( mCompareUrl );

    // Dialog Buttons
    //
    QPushButton * okButton = new QPushButton( tr("OK"), this );
    QPushButton * cancelButton = new QPushButton( tr("Cancel"), this );
    QPushButton * applyButton = new QPushButton( tr("Apply"), this );

    //
    // LAYOUT ------------------------------------------
    //

    // Font Choice and Tab Width
    //
    QGridLayout * gl = new QGridLayout;
    formattingGroup->layout()->addItem( gl );

    // Font
    //
    gl->addWidget( fontLabel, 1, 0 );
    QHBoxLayout * hbl;
    hbl = new QHBoxLayout;
    gl->setSpacing(style()->pixelMetric(QStyle::PM_ButtonMargin));
    hbl->setSpacing(style()->pixelMetric(QStyle::PM_ButtonMargin));
    hbl->addWidget( fontDescriptionLabel );
    hbl->addWidget( fontButton );
    hbl->addStretch();
    gl->addLayout( hbl, 1, 1 );

    // Tab Width
    //
    gl->addWidget( tabWidthLabel, 2, 0 );
    hbl = new QHBoxLayout();
    hbl->setSpacing(style()->pixelMetric(QStyle::PM_ButtonMargin));
    hbl->addWidget( mTabWidth );
    hbl->addStretch(1);
    gl->addLayout( hbl, 2, 1 );

    // Insert spaces instead of tabs
    //
    gl->addWidget( mTabSpaces, 3, 0, 1, -1 );
    // Show tabs and spaces
    //
    gl->addWidget( mShowTabsSpaces, 4, 0, 1, -1 );

    mainLayout->addWidget( formattingGroup );

    // File Format
    //
    QVBoxLayout * vbl = new QVBoxLayout( );
    fileFormatGroup->layout()->addItem( vbl );

    vbl->setSpacing(style()->pixelMetric(QStyle::PM_ButtonMargin));
    hbl = new QHBoxLayout();
    hbl->addWidget( charSetLabel );
    hbl->addWidget( charSetPopup );
    vbl->addLayout( hbl );

    hbl = new QHBoxLayout();
    hbl->addWidget( lineEndingLabel );
    hbl->addWidget( lineEndingPopup );
    vbl->addLayout( hbl );

    mainLayout->addWidget( fileFormatGroup );

    // Application language
    //
    vbl = new QVBoxLayout( );
    languageGroup->layout()->addItem( vbl );
    vbl->setSpacing(style()->pixelMetric(QStyle::PM_ButtonMargin));
    vbl->addWidget( mAppLanguageWidget );

    mainLayout->addWidget( languageGroup );

    // Diff Options
    //
     QBoxLayout * diffGroupBoxLayout = new QVBoxLayout;

    // Radio buttons
    //
    for ( int i = 0; i < 4; i++ )
        diffGroupBoxLayout->addWidget( rb[i] );

     diffGroupBoxLayout->addItem( new QSpacerItem( 20, 16 ) );

    // Examples
    //
    gl = new QGridLayout;

    gl->addWidget( exampleLabel, 0, 0, 1, 0 );
    gl->addWidget( ws, 1, 1 );
    gl->addItem( new QSpacerItem( 12, 21, QSizePolicy::Fixed, QSizePolicy::Minimum ), 1, 0 );
    gl->addWidget( linesEqualLabel, 2, 1 );

    diffGroupBoxLayout->addLayout( gl );
    diffGroupBox->setLayout( diffGroupBoxLayout );

    mainLayout->addWidget( diffGroupBox );

    // DOCX Web Service URL
    //
    hbl = new QHBoxLayout( );
    compareWebServiceGroup->layout()->addItem( hbl );
    hbl->setSpacing(style()->pixelMetric(QStyle::PM_ButtonMargin));
    hbl->addWidget( compareUrlLabel );
    hbl->addWidget( mCompareUrl );

    mainLayout->addWidget( compareWebServiceGroup );


    // Dialog Buttons
    //
    hbl = new QHBoxLayout( );
    mainLayout->addLayout( hbl );

    hbl->setSpacing(style()->pixelMetric(QStyle::PM_ButtonMargin));
    hbl->addWidget( applyButton );

    QDialogButtonBox* bbox = new QDialogButtonBox(this);
    bbox->addButton( cancelButton , QDialogButtonBox::RejectRole );
    bbox->addButton( okButton, QDialogButtonBox::AcceptRole );
    ObjectNameUtil::ApplyStandardButtonNames(bbox);

    hbl->addStretch();
    hbl->addWidget( bbox );

    restoreGeometry( Prefs::Value(Prefs::FQ(p4merge::prefs::PrefDialogGeometry), QByteArray()) );

    //
    // BEHAVIOR ------------------------------------------
    //

    // Set hidden accelerators for P4WinMerge compatibility.
    //
    if ( Platform::IsWin() )
    {
        QShortcut * a = new QShortcut( this );
        a->setKey( QKeySequence( tr("Esc") ) );
        connect( a,    SIGNAL (activated() ),
                 this, SLOT   (close()) );
    }

    // Application Font
    //
    if (Platform::IsUnix())
    {
        connect( appfontButton, SIGNAL(clicked()),
            this, SLOT(showChooseAppFontDialog()) );
        appFontDescriptionLabel->setText( describeFont( mApplicationFont ) );
    }

    // Font
    //
    connect( fontButton, SIGNAL(clicked()), this, SLOT(showChooseFontDialog()) );
    fontDescriptionLabel->setText( describeFont( mFont ) );

    // Diff Options
    //
    connect( sm, SIGNAL(mapped(int)), this, SLOT(setDiffOption(int)) );

    int ci = P4mPrefs::ToDiffFlags( Prefs::Value( Prefs::FQ( MergePrefs::DIFFOPTION ), QString() ) );
    rb[ ci ]->setChecked( true );
    ws->setCurrentIndex(ci);

    // Dialog Buttons
    //
    okButton->setDefault( true );
    connect( okButton,    SIGNAL(clicked()), this, SLOT(saveAndClose()) );
    connect( cancelButton,SIGNAL(clicked()), this, SLOT(close()) );
    connect( applyButton, SIGNAL(clicked()), this, SLOT(updateSettings()) );
    applyButton->hide();
}


void
P4mPreferencesDialog::closeEvent( QCloseEvent * e )
{
    Prefs::SetValue(Prefs::FQ(p4merge::prefs::PrefDialogGeometry), saveGeometry());
    QDialog::closeEvent( e );
}


void
P4mPreferencesDialog::saveAndClose()
{
    updateSettings();
    close();
}

void
P4mPreferencesDialog::showChooseAppFontDialog()
{
    bool ok;
    QFont f = QFontDialog::getFont( &ok, mApplicationFont, this, tr("Select Font"), QFontDialog::DontUseNativeDialog );
    if ( !ok )
        return;

    f.setUnderline( false );
    f.setStrikeOut( false );
    appFontDescriptionLabel->setText( describeFont( f ) );
    mApplicationFont = f;
}

void
P4mPreferencesDialog::showChooseFontDialog()
{
    bool ok;
    QFont f = QFontDialog::getFont( &ok, mFont, this, tr("Select Font"), QFontDialog::DontUseNativeDialog );
    if ( !ok )
        return;

    f.setUnderline( false );
    f.setStrikeOut( false );
    fontDescriptionLabel->setText( describeFont( f ) );
    mFont = f;
}

void
P4mPreferencesDialog::setDiffOption( int i )
{
    rb[i]->setChecked( true );
}

QString
P4mPreferencesDialog::describeFont( const QFont & f )
{
    QString name = f.family();
    name = QString( "%1 - %2" ).arg( f.family() ).arg( f.pointSize() );
    return name;
}

void
P4mPreferencesDialog::updateSettings()
{
    if (Platform::IsUnix())
        Prefs::SetValue( Prefs::FQ( MergePrefs::APPFONT ), mApplicationFont );

    Prefs::SetValue( Prefs::FQ( MergePrefs::FONT ),       mFont );
    Prefs::SetValue( Prefs::FQ( MergePrefs::TABWIDTH ),   mTabWidth->value() );
    Prefs::SetValue( Prefs::FQ( MergePrefs::DIFFOPTION ), P4mPrefs::ToString( (P4mPrefs::DiffFlags)ws->currentIndex() ) );
    Prefs::SetValue( Prefs::FQ( MergePrefs::CHARSET ),    P4mPrefs::ToString( (P4mPrefs::Charset)charSetPopup->currentIndex() ) );
    Prefs::SetValue( Prefs::FQ( MergePrefs::LINEENDING ), P4mPrefs::ToString( (P4mPrefs::LineEnding)lineEndingPopup->currentIndex() ) );
    Prefs::SetValue( Prefs::FQ( MergePrefs::TABINSERTSSPACES ),    mTabSpaces->isChecked() );
    Prefs::SetValue( Prefs::FQ( MergePrefs::SHOWTABSSPACES ), mShowTabsSpaces->isChecked() );
    LanguageWidget::SetLanguagePref(mAppLanguageWidget->currentLanguage());
    Prefs::SetValue( Prefs::FQ( MergePrefs::DOCXCOMPAREURL ), mCompareUrl->text() );

}
