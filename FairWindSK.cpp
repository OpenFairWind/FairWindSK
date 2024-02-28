//
// Created by Raffaele Montella on 03/04/21.
//

#include <QPluginLoader>
#include <QDir>
#include <QCoreApplication>
#include <QSettings>
#include <utility>
#include <QJsonArray>

#include <FairWindSK.hpp>

namespace fairwindsk {
/*
 * FairWind
 * Private constructor - called by getInstance in order to ensure
 * the singleton design pattern
 */
    FairWindSK::FairWindSK() {
        qDebug() << "FairWindSK constructor";

    }

    void FairWindSK::setMainWindow(QMainWindow *mainWindow) {
        m_mainWindow = mainWindow;
    }

    QMainWindow * FairWindSK::getMainWindow() {
        return m_mainWindow;
    }

/*
 * getInstance
 * Either returns the available instance or creates a new one
 */
    FairWindSK *FairWindSK::getInstance() {
        if (m_instance == nullptr) {
            m_instance = new FairWindSK();
        }
        return m_instance;
    }


/*
 * getConfig
 * Returns the configuration infos
 */
    QJsonObject FairWindSK::getConfig() {

        // Define the object
        QJsonObject config;

        // Return the config object
        return config;
    }
}
