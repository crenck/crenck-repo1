#ifndef UIADMINPREFERENCESDIALOG_H
#define UIADMINPREFERENCESDIALOG_H

#include "UIPreferencesDialog.h"

class UIAdminPreferencesDialog : public UIPreferencesDialog
{
    Q_OBJECT
public:
    UIAdminPreferencesDialog( QWidget* parent = 0, Qt::WindowFlags fl = 0);
    virtual ~UIAdminPreferencesDialog();

    virtual void addTabWidgets();
};

#endif // UIADMINPREFERENCESDIALOG_H
