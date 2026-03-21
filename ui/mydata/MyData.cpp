//
// Created by Raffaele Montella on 07/06/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_MyData.h" resolved

#include "MyData.hpp"
#include "ui_MyData.h"
#include "Files.hpp"
#include "ResourceTab.hpp"
#include "HistoryTrackTab.hpp"

namespace fairwindsk::ui::mydata {
    MyData::MyData(QWidget *parent, QWidget *currenWidget): QWidget(parent), ui(new Ui::MyData) {

        ui->setupUi(this);

        // Initialize the tabs
        initTabs(0);

        m_currentWidget = currenWidget;

    }

    /*
 * removeTabs
 * Remove all  tabs
 */
    void MyData::removeTabs() {
        // While there is at least a tab in the tab widget...
        while (ui->tabWidget->count() > 0) {

            // Get the reference of the tab in position 0
            const auto tab = ui->tabWidget->widget(0);

            // Remove the tab
            ui->tabWidget->removeTab(0);

            // Delete the object
            delete tab;
        }
    }

    /*
 * initTabs
 * Add tabs, then set the current index
 */
    void MyData::initTabs(const int currentIndex) {

        // Remove tabs if present
        removeTabs();

        // Add the resources tabs
        ui->tabWidget->addTab(new ResourceTab(ResourceKind::Waypoint, this), tr("Waypoints"));
        ui->tabWidget->addTab(new ResourceTab(ResourceKind::Route, this), tr("Routes"));
        ui->tabWidget->addTab(new ResourceTab(ResourceKind::Region, this), tr("Regions"));
        ui->tabWidget->addTab(new ResourceTab(ResourceKind::Note, this), tr("Notes"));
        ui->tabWidget->addTab(new ResourceTab(ResourceKind::Chart, this), tr("Charts"));
        ui->tabWidget->addTab(new HistoryTrackTab(this), tr("Tracks"));

        // Add the applications tab
        ui->tabWidget->addTab(new Files(this), tr("Files"));

        // Set the current tab index
        ui->tabWidget->setCurrentIndex(currentIndex);
    }




    void MyData::onClose() {
        setVisible(false);
        emit closed(this);
    }

    QWidget *MyData::getCurrentWidget() {
        return m_currentWidget;
    }

    MyData::~MyData() {
        // Remove the tabs
        removeTabs();

        // Check if the ui pointer is valid
        if (ui) {

            // Delete the UI
            delete ui;

            // Set the ui pointer to null
            ui = nullptr;
        }
    }
} // fairwindsk::ui::mydata
