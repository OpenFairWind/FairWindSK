//
// Created by Raffaele Montella on 07/06/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_MyData.h" resolved

#include <algorithm>

#include <QSignalBlocker>

#include "MyData.hpp"
#include "ui_MyData.h"
#include "Charts.hpp"
#include "Files.hpp"
#include "HistoryTrackTab.hpp"
#include "Notes.hpp"
#include "Regions.hpp"
#include "ResourceTab.hpp"
#include "Routes.hpp"
#include "Waypoints.hpp"

namespace fairwindsk::ui::mydata {
    MyData::MyData(QWidget *parent, QWidget *currenWidget): QWidget(parent), ui(new Ui::MyData) {
        ui->setupUi(this);
        connect(ui->tabWidget, &QTabWidget::currentChanged, this, &MyData::ensureTabPage);
        initTabs(0);

        m_currentWidget = currenWidget;
    }

    /*
 * removeTabs
 * Remove all  tabs
 */
    void MyData::removeTabs() {
        m_loadedPages.clear();
        m_loadingPages.clear();

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
        const QSignalBlocker blocker(ui->tabWidget);

        // Remove tabs if present
        removeTabs();

        m_loadedPages.resize(7);
        for (int index = 0; index < 7; ++index) {
            ui->tabWidget->addTab(new QWidget(ui->tabWidget), tabTitle(index));
        }

        // Set the current tab index
        ui->tabWidget->setCurrentIndex(std::clamp(currentIndex, 0, std::max(0, ui->tabWidget->count() - 1)));
        ensureTabPage(ui->tabWidget->currentIndex());
    }

    void MyData::ensureTabPage(const int index) {
        if (!ui || !ui->tabWidget || index < 0 || index >= ui->tabWidget->count()) {
            return;
        }

        if (index < m_loadedPages.size() && m_loadedPages.at(index)) {
            return;
        }
        if (m_loadingPages.contains(index)) {
            qInfo() << "MyData tab" << index << "is already being created; skipping re-entrant load";
            return;
        }

        m_loadingPages.insert(index);
        QWidget *page = createTabPage(index);
        m_loadingPages.remove(index);
        if (!page) {
            return;
        }

        const int previousIndex = ui->tabWidget->currentIndex();
        QWidget *existingPage = ui->tabWidget->widget(index);
        if (index >= m_loadedPages.size()) {
            m_loadedPages.resize(index + 1);
        }
        m_loadedPages[index] = page;

        const QSignalBlocker blocker(ui->tabWidget);
        ui->tabWidget->removeTab(index);
        ui->tabWidget->insertTab(index, page, tabTitle(index));
        if (previousIndex >= 0 && previousIndex < ui->tabWidget->count()) {
            ui->tabWidget->setCurrentIndex(previousIndex);
        }

        if (existingPage) {
            delete existingPage;
        }
    }

    QWidget *MyData::createTabPage(const int index) const {
        switch (index) {
            case 0:
                return new Waypoints(const_cast<MyData *>(this));
            case 1:
                return new Routes(const_cast<MyData *>(this));
            case 2:
                return new Regions(const_cast<MyData *>(this));
            case 3:
                return new Notes(const_cast<MyData *>(this));
            case 4:
                return new Charts(const_cast<MyData *>(this));
            case 5:
                return new HistoryTrackTab(const_cast<MyData *>(this));
            case 6:
                return new Files(const_cast<MyData *>(this));
            default:
                return nullptr;
        }
    }

    QString MyData::tabTitle(const int index) const {
        switch (index) {
            case 0:
                return tr("Waypoints");
            case 1:
                return tr("Routes");
            case 2:
                return tr("Regions");
            case 3:
                return tr("Notes");
            case 4:
                return tr("Charts");
            case 5:
                return tr("Tracks");
            case 6:
                return tr("Files");
            default:
                return {};
        }
    }

    void MyData::onClose() {
        setVisible(false);
        emit closed(this);
    }

    QWidget *MyData::getCurrentWidget() {
        return m_currentWidget;
    }

    void MyData::refreshFromConfiguration() {
        if (!ui || !ui->tabWidget) {
            return;
        }

        for (int index = 0; index < ui->tabWidget->count(); ++index) {
            if (QWidget *page = ui->tabWidget->widget(index)) {
                page->updateGeometry();
                page->update();
            }
        }

        ui->tabWidget->updateGeometry();
        ui->tabWidget->update();
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
