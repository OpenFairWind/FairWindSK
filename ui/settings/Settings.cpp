//
// Created by Raffaele Montella on 18/03/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_Settings.h" resolved

#include <iostream>

#include <QSignalBlocker>

#include "Settings.hpp"

#include "FairWindSK.hpp"
#include "Units.hpp"

#include "Main.hpp"
#include "Comfort.hpp"
#include "Connection.hpp"
#include "SignalK.hpp"
#include "Apps.hpp"
#include "System.hpp"


#include "ui_Settings.h"

namespace fairwindsk::ui::settings {
    namespace {
        constexpr int kLiveApplyDelayMs = 150;
    }

    void Settings::applyConfiguration() {
        const quint32 runtimeChanges = m_pendingRuntimeChanges == 0 ? FairWindSK::RuntimeAll : m_pendingRuntimeChanges;

        // Get the configuration root element
        const auto configurationAsJson = m_configuration.getRoot();

        // Persist the edited local configuration snapshot first.
        m_configuration.save();

        // Set the new configuration in the FairWindSK singleton instance
        FairWindSK::getInstance()->getConfiguration()->setRoot(configurationAsJson);

        // Save the configuration permanently
        FairWindSK::getInstance()->getConfiguration()->save();
        FairWindSK::getInstance()->reconfigureRuntime(runtimeChanges);
        m_hasPendingUiChanges = false;
        m_pendingRuntimeChanges = 0;
    }


    /*
 * Settings
 * The class constructor
 */
    Settings::Settings(QWidget *parent, QWidget *currenWidget): QWidget(parent), ui(new Ui::Settings) {

        // Set the UI
        ui->setupUi(this);

        // Copy the pointer locally
        m_currentConfiguration = FairWindSK::getInstance()->getConfiguration();

        // Set the filename of the local configuration as the file name of FairWindSK configuration
        m_configuration.setFilename(m_currentConfiguration->getFilename());

        // Get the current configuration root as a json object
        const auto configurationAsJson = m_currentConfiguration->getRoot();

        // Set the local configuration with the json object
        m_configuration.setRoot(configurationAsJson);

        // Set the default tab
        auto currentIndex = 0;

        // Check if no server is set
        if (m_configuration.getSignalKServerUrl().isEmpty()) {

            // Set the tab index to the connection tab
            currentIndex = 2;
        }

        // Initialize the tabs
        initTabs(currentIndex);

        // Store the current widget pointer
        m_currentWidget = currenWidget;

        connect(ui->tabWidget, &QTabWidget::currentChanged, this, &Settings::onTabChanged);

        m_applyTimer = new QTimer(this);
        m_applyTimer->setSingleShot(true);
        connect(m_applyTimer, &QTimer::timeout, this, [this]() {
            if (m_rebuildingTabs) {
                return;
            }
            applyConfiguration();
        });
    }

    /*
 * removeTabs
 * Remove all  tabs
 */
    void Settings::removeTabs() {
        if (!ui || !ui->tabWidget) {
            m_tabPages.clear();
            return;
        }

        disconnect(ui->tabWidget, &QTabWidget::currentChanged, this, &Settings::onTabChanged);
        const QSignalBlocker blocker(ui->tabWidget);
        // While there is at least a tab in the tab widget...
        while (ui->tabWidget->count() > 0) {

            // Get the reference of the tab in position 0
            const auto tab = ui->tabWidget->widget(0);
            if (!m_tabPages.isEmpty()) {
                m_tabPages.removeFirst();
            }

            // Remove the tab
            ui->tabWidget->removeTab(0);

            // Delete the object
            delete tab;
        }

        m_tabPages.clear();
    }

    /*
 * initTabs
 * Add tabs, then set the current index
 */
    void Settings::initTabs(const int currentIndex) {
        m_rebuildingTabs = true;

        // Remove tabs if present
        removeTabs();

        m_tabPages = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
        for (const auto &tabTitle : {tr("Main"), tr("Comfort"), tr("Connection"), tr("Signal K"), tr("Units"), tr("Applications"), tr("System")}) {
            auto *container = new QWidget(ui->tabWidget);
            auto *layout = new QVBoxLayout(container);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setSpacing(0);
            ui->tabWidget->addTab(container, tabTitle);
        }

        // Set the current tab index
        const int resolvedIndex = std::clamp(currentIndex, 0, ui->tabWidget->count() - 1);
        ensureTabCreated(resolvedIndex);
        ui->tabWidget->setCurrentIndex(resolvedIndex);
        m_rebuildingTabs = false;
    }

    QWidget *Settings::createTabWidget(const int index) {
        switch (index) {
            case 0:
                return new Main(this);
            case 1:
                return new Comfort(this);
            case 2:
                return new Connection(this);
            case 3:
                return new SignalK(this);
            case 4:
                return new Units(this);
            case 5:
                return new Apps(this);
            case 6:
                return new System(this);
            default:
                return new QWidget(this);
        }
    }

    void Settings::ensureTabCreated(const int index) {
        if (index < 0 || index >= ui->tabWidget->count()) {
            return;
        }

        if (index >= m_tabPages.size()) {
            m_tabPages.resize(ui->tabWidget->count());
        }

        if (m_tabPages.at(index)) {
            return;
        }

        QWidget *page = createTabWidget(index);
        m_tabPages[index] = page;
        QWidget *container = ui->tabWidget->widget(index);
        if (!container) {
            return;
        }
        if (!container->layout()) {
            auto *layout = new QVBoxLayout(container);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setSpacing(0);
        }
        container->layout()->addWidget(page);
    }

    void Settings::onTabChanged(const int index) {
        if (m_rebuildingTabs || !ui || !ui->tabWidget) {
            return;
        }

        ensureTabCreated(index);
        QWidget *page = index >= 0 && index < m_tabPages.size() ? m_tabPages.at(index).data() : nullptr;
        if (auto *appsTab = qobject_cast<Apps *>(page)) {
            appsTab->refreshFromConfiguration();
        }
    }

    /*
     * onClicked
     * Invoked when a push button in the button bock is clicked
     */
    void Settings::onClicked(QAbstractButton *button) {

        Q_UNUSED(button);
    }

    /*
     * getCurrentWidget
     * Return the current widget
     */
    QWidget *Settings::getCurrentWidget() {
        return m_currentWidget;
    }

    void Settings::setCurrentWidget(QWidget *currentWidget) {
        m_currentWidget = currentWidget;
    }

    bool Settings::hasPendingChanges() {
        return m_hasPendingUiChanges || m_configuration.getRoot() != m_currentConfiguration->getRoot();
    }

    void Settings::markDirty(const quint32 runtimeChanges, const int delayMs) {
        m_hasPendingUiChanges = true;
        m_pendingRuntimeChanges |= (runtimeChanges == 0 ? FairWindSK::RuntimeAll : runtimeChanges);

        const auto configurationAsJson = m_configuration.getRoot();
        m_configuration.save();
        if (m_currentConfiguration) {
            m_currentConfiguration->setRoot(configurationAsJson);
            m_currentConfiguration->save();
        }

        scheduleApplyConfiguration(delayMs >= 0 ? delayMs : kLiveApplyDelayMs);
    }

    void Settings::saveChanges() {
        if (m_applyTimer) {
            m_applyTimer->stop();
        }
        applyConfiguration();
    }

    void Settings::discardChanges() {
        if (m_applyTimer) {
            m_applyTimer->stop();
        }
        FairWindSK::getInstance()->applyUiPreferences(m_currentConfiguration);
        m_hasPendingUiChanges = false;
        m_pendingRuntimeChanges = 0;
        resetFromCurrentConfiguration(ui->tabWidget->currentIndex());
    }

    void Settings::resetFromCurrentConfiguration(const int currentIndex) {
        const int resolvedIndex = currentIndex >= 0 ? currentIndex : ui->tabWidget->currentIndex();
        m_configuration.setFilename(m_currentConfiguration->getFilename());
        m_configuration.setRoot(m_currentConfiguration->getRoot());
        if (m_applyTimer) {
            m_applyTimer->stop();
        }
        m_hasPendingUiChanges = false;
        m_pendingRuntimeChanges = 0;
        initTabs(std::max(0, resolvedIndex));
    }

    void Settings::scheduleApplyConfiguration(const int delayMs) {
        if (m_rebuildingTabs || !m_applyTimer) {
            return;
        }

        m_applyTimer->start(std::max(0, delayMs));
    }

    void Settings::resetToCurrentConfiguration() {
        resetFromCurrentConfiguration(ui->tabWidget->currentIndex());
        FairWindSK::getInstance()->applyUiPreferences(&m_configuration);
    }

    void Settings::restoreDefaultConfiguration() {
        m_configuration.setDefault();
        const auto currentIndex = ui->tabWidget->currentIndex();
        initTabs(currentIndex);
        FairWindSK::getInstance()->applyUiPreferences(&m_configuration);
        markDirty(FairWindSK::RuntimeAll, 0);
    }

    void Settings::restartApplication() {
        applyConfiguration();
        QTimer::singleShot(0, qApp, []() {
            QCoreApplication::exit(1);
        });
    }

    void Settings::quitApplication() {
        applyConfiguration();
        QApplication::exit(0);
    }

    /*
     * getConfiguration
     * Get a pointer  to the local configuration
     */
    Configuration *Settings::getConfiguration() {
        return &m_configuration;
    }

    /*
     * ~Settings
     * Class destructor
     */
    Settings::~Settings() {
        m_rebuildingTabs = true;
        if (m_applyTimer) {
            m_applyTimer->stop();
        }

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



} // fairwindsk::ui
