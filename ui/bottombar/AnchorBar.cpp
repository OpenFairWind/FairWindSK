//
// Created by Raffaele Montella on 08/12/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_AnchorBar.h" resolved


#include <QPushButton>

#include "AnchorBar.hpp"

#include "FairWindSK.hpp"
#include "ui_AnchorBar.h"


namespace fairwindsk::ui::bottombar {
    AnchorBar::AnchorBar(QWidget *parent) :
            QWidget(parent), ui(new Ui::AnchorBar) {
        ui->setupUi(this);

        // Get the FairWind singleton
        const auto fairWindSK = fairwindsk::FairWindSK::getInstance();

        // Get configuration
        const auto configuration = fairWindSK->getConfiguration();

        // Get the configuration json object

        // Check if the configuration object contains the key 'signalk' with an object value
        if (auto configurationJsonObject = configuration->getRoot(); configurationJsonObject.contains("signalk") && configurationJsonObject["signalk"].is_object()) {

            // Get the signal k paths object
            m_signalkPaths = configurationJsonObject["signalk"];
        }

        // Not visible by default
        QWidget::setVisible(false);

        // emit a signal when the MyData tool button from the UI is clicked
        connect(ui->toolButton_Hide, &QPushButton::clicked, this, &AnchorBar::onHideClicked);

        connect(ui->toolButton_Reset, &QPushButton::clicked, this, &AnchorBar::onResetClicked);
        connect(ui->toolButton_Up, &QPushButton::clicked, this, &AnchorBar::onUpClicked);
        connect(ui->toolButton_Raise, &QPushButton::clicked, this, &AnchorBar::onRaiseClicked);
        connect(ui->toolButton_RadiusDec, &QPushButton::clicked, this, &AnchorBar::onRadiusDecClicked);
        connect(ui->toolButton_RadiusInc, &QPushButton::clicked, this, &AnchorBar::onRadiusIncClicked);
        connect(ui->toolButton_Drop, &QPushButton::clicked, this, &AnchorBar::onDropClicked);
        connect(ui->toolButton_Down, &QPushButton::clicked, this, &AnchorBar::onDownClicked);
        connect(ui->toolButton_Release, &QPushButton::clicked, this, &AnchorBar::onReleaseClicked);

    }

    void AnchorBar::onHideClicked() {
        setVisible(false);
        emit hidden();
    }

    void AnchorBar::onResetClicked() {

        emit resetCounter();
    }

    void AnchorBar::onUpClicked() {

        emit chainUp();
    }

    void AnchorBar::onRaiseClicked() {

        // Check if the Options object has the rsa key and if it is a string
        if (m_signalkPaths.contains("anchor.actions.raise") && m_signalkPaths["anchor.actions.raise"].is_string())
        {
            // Get the FairWind singleton
            const auto fairWindSK = fairwindsk::FairWindSK::getInstance();

            // Get the Signal K client
            const auto signalKClient = fairWindSK->getSignalKClient();

            const auto url = signalKClient->server().toString() + "/" +
                QString::fromStdString(
                    m_signalkPaths["anchor.actions.raise"].get<std::string>()).replace(".","/"
                        );

            qDebug() << url;

            // Rise the alarm
            auto result = signalKClient->signalkPost(QUrl(url));

            qDebug() << result;

            emit raiseAnchor();
        }
    }

    void AnchorBar::onRadiusDecClicked() {

        emit radiusDec();
    }

    void AnchorBar::onRadiusIncClicked() {

        emit radiusInc();
    }

    void AnchorBar::onDropClicked() {

        // Check if the Options object has the rsa key and if it is a string
        if (m_signalkPaths.contains("anchor.actions.drop") && m_signalkPaths["anchor.actions.drop"].is_string())
        {
            // Get the FairWind singleton
            const auto fairWindSK = fairwindsk::FairWindSK::getInstance();

            // Get the Signal K client
            const auto signalKClient = fairWindSK->getSignalKClient();

            const auto url = signalKClient->server().toString() + "/" +
                QString::fromStdString(
                    m_signalkPaths["anchor.actions.drop"].get<std::string>()).replace(".","/"
                        );

            qDebug() << url;

            // Rise the alarm
            auto result = signalKClient->signalkPost(QUrl(url));

            qDebug() << result;

            emit dropAnchor();
        }
    }

    void AnchorBar::onDownClicked() {

        emit chainDown();
    }

    void AnchorBar::onReleaseClicked() {

        emit chainRelease();
    }

    AnchorBar::~AnchorBar() {
        delete ui;
    }
} // fairwindsk::ui::bottombar
