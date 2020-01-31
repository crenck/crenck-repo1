/*! \class AdminPreferencesDialog
    \brief A class which create a framework for a preferences window.

    1. This is an abstract class. Each subclass must implement addTabWidgets()
       in order to add the pages for that application.  See P4V examples.
       You can call addTabWidgets from the constructor of your subclass.

    2. A QTabWidget page must subclass UIAdminPreferenceWidget.h to get the interface
       for loading/saving preferences.

    3. If a page is only used by your application, store it in your application
       directory.

    4. Some of the pages in this directory may have configurable items in them,
       specifically the General and Connection widgets.  If you make other items
       configureable you MUST update the other application subclasses to keep
       including these items.  If you add new items you must decide whether all
       applications need them.
*/
#include "UIAdminPreferencesDialog.h"

#include "AppletsPage.h"
#include "BehaviorPage.h"
#include "ConnectionsPage.h"
#include "DiffPage.h"
#include "DisplayPage.h"
#include "EditorPage.h"
#include "FilesHistoryPage.h"
#include "FontPage.h"
#include "GuiFeaturesLoader.h"
#include "LogPage.h"
#include "Platform.h"
#include "PrefListItem.h"
#include "ServerDataPage.h"
#include "ToolsPage.h"

#include <QStackedWidget>

UIAdminPreferencesDialog::UIAdminPreferencesDialog(QWidget* parent, Qt::WindowFlags fl)
    : UIPreferencesDialog(parent, fl)
{
    setObjectName(winKey::admin::Preferences);
    addTabWidgets();
}

UIAdminPreferencesDialog::~UIAdminPreferencesDialog()
{
}

void
UIAdminPreferencesDialog::addTabWidgets()
{
    UIPreferenceWidget* widget;
    PrefListItem* listItem;
    // create the tabs for the preference dialog

        // General
    widget = new ConnectionsPage(Pref_App_P4Admin, mPageStack);
    mPageStack->addWidget(widget);
    listItem = new PrefListItem(QStringList(tr("Connections")), widget);
    mTree->addTopLevelItem(listItem);

    widget = new ServerDataPage(Pref_App_P4Admin, mPageStack);
    mPageStack->addWidget(widget);
    listItem = new PrefListItem(QStringList(tr("Server Data")), widget);
    mTree->addTopLevelItem(listItem);

        // Log
    widget = new LogPage(Pref_App_P4Admin, mPageStack);
    mPageStack->addWidget(widget);
    listItem = new PrefListItem(QStringList(tr("Logging")), widget);
    mTree->addTopLevelItem(listItem);


        // Display
    widget = new DisplayPage(Pref_App_P4Admin, mPageStack);
    mPageStack->addWidget(widget);
    listItem = new PrefListItem(QStringList(tr("Display")), widget);
    mTree->addTopLevelItem(listItem);

        // Files
    widget = new FilesHistoryPage(Pref_App_P4Admin, mPageStack);
    mPageStack->addWidget(widget);
    listItem = new PrefListItem(QStringList(tr("Files")), widget);
    mTree->addTopLevelItem(listItem);

        // Font
    widget = new FontPage(Pref_App_P4Admin, mPageStack);
    mPageStack->addWidget(widget);
    listItem = new PrefListItem(QStringList(tr("Application Font")), widget);
    mTree->addTopLevelItem(listItem);

        // Behavior
    widget = new BehaviorPage(Pref_App_P4Admin, mPageStack);
    mPageStack->addWidget(widget);
    listItem = new PrefListItem(QStringList(tr("Behavior")), widget);
    mTree->addTopLevelItem(listItem);

        // Tools
    widget = new ToolsPage(mPageStack);
    mPageStack->addWidget(widget);
    listItem = new PrefListItem(QStringList(tr("Tools")), widget);
    mTree->addTopLevelItem(listItem);

        // Editor
    widget = new EditorPage(mPageStack);
    mPageStack->addWidget(widget);
    listItem = new PrefListItem(QStringList(tr("Editor")), widget);
    mTree->addTopLevelItem(listItem);

        // Diff
    widget = new DiffPage(mPageStack);
    mPageStack->addWidget(widget);
    listItem = new PrefListItem(QStringList(tr("Diff")), widget);
    mTree->addTopLevelItem(listItem);

        // Applets
    widget = new AppletsPage(Pref_App_P4Admin, mPageStack);
    mPageStack->addWidget(widget);
    listItem = new PrefListItem(QStringList(tr("Applets")), widget);
    mTree->addTopLevelItem(listItem);

    mTree->expandAll();


    // set default button to "apply"
    UIPreferencesDialog::makeDefaultApply();
}
