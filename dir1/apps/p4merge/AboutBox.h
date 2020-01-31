#ifndef __AboutBox_H__
#define __AboutBox_H__

#include <QDialog>

class AboutBox : public QDialog
{
    Q_OBJECT
public:
    AboutBox( QWidget * parent = 0, Qt::WindowFlags f = 0 );

private slots:
    void openPerforcePage();

};


#endif // __AboutBox_H__


