#include "AboutBox.h"
#include "Platform.h"

#include <QBoxLayout>
#include <QDate>
#include <QDesktopServices>
#include <QLabel>
#include <QUrl>

AboutBox::AboutBox( QWidget * parent, Qt::WindowFlags f )
    : QDialog( parent, f )
{
    setWindowTitle( "Helix P4Merge" );
    setStyleSheet( "QDialog { background-color: white; }" );

    QLabel * icon = new QLabel( this );
    icon->setMinimumSize( 128, 128 );
    icon->setPixmap(QPixmap(":/p4mergeimage.png"));

    QLabel * aboutText = new QLabel( this );
    aboutText->setObjectName( "AboutText" );
    aboutText->setText(tr("<h2>%1</h2>"
        "Copyright &#169; 2002-%2 <A href>Perforce Software</A>.<br>"
        "All rights reserved."
        "<hr width=500>"    // despite docs, Qt can't handle relative widths
        "%3 %4 %5 <br><br>")
        .arg(tr("The Perforce Visual Merge Tool"))
        .arg(ID_Y)
        .arg(ID_Y)
        .arg(QDate::longMonthName( QString(ID_M).toInt() ))
        .arg(ID_D) );
    aboutText->setTextInteractionFlags(Qt::TextBrowserInteraction);

    connect(aboutText, &QLabel::linkActivated,
            this,      &AboutBox::openPerforcePage);

    QLabel * versionText = new QLabel( this );
    versionText->setText( tr("Rev. %4/%1/%2/%3")
        .arg(ID_OS).arg(ID_REL).arg(ID_PATCH).arg("P4Merge") );
    versionText->setTextInteractionFlags(Qt::TextBrowserInteraction);

    QBoxLayout * masterLayout = new QHBoxLayout( this );
    masterLayout->addWidget( icon, 0, Qt::AlignTop );
    masterLayout->setSizeConstraint( QLayout::SetFixedSize );

    QBoxLayout * textLayout = new QVBoxLayout();
    masterLayout->addItem( textLayout );
    textLayout->addWidget( aboutText );
    textLayout->addWidget( versionText );
}

void
AboutBox::openPerforcePage()
{
    QDesktopServices::openUrl(QUrl("http://www.perforce.com"));
}

