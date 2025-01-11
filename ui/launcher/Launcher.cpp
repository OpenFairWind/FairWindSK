//
// Created by __author__ on 21/01/2022.
//

// You may need to build the project (run Qt uic code generator) to get "ui_MainPage.h" resolved

#include <QScreen>
#include <QRect>
#include <QScrollBar>
#include <QGridLayout>
#include <QNetworkReply>
#include <QtCore/qjsonarray.h>
#include "Launcher.hpp"

#include "AppItem.hpp"

namespace fairwindsk::ui::launcher {
    Launcher::Launcher(QWidget *parent) : QWidget(parent), ui(new Ui::Launcher) {

        ui->setupUi((QWidget *)this);

        m_cols = 4;
        m_rows = 2;
        m_iconSize = 256;

        // Create a new grid layout
        m_layout = new QGridLayout(ui->scrollAreaWidgetContents);
        if (m_layout) {
            m_layout->setContentsMargins(0,0,0,0);


            // Set the UI scroll area with the newly created layout
            ui->scrollAreaWidgetContents->setLayout(m_layout);
            ui->scrollArea->horizontalScrollBar()->setVisible(false);


            // Right scroll
            connect(ui->toolButton_Right, &QToolButton::clicked, this, &Launcher::onScrollRight);

            // Left scroll
            connect(ui->toolButton_Left, &QToolButton::clicked, this, &Launcher::onScrollLeft);

            // Order by order value
            QMap<int, QPair<AppItem *, QString>> map;
            int row = 0, col = 0;

            // Get the FairWind singleton
            auto fairWindSK = fairwindsk::FairWindSK::getInstance();

            //qDebug() << "appsHashes: " << fairWindSK->getAppsHashes();

            // Populate the inverted list
            for (auto &hash : fairWindSK->getAppsHashes()) {
                // Get the hash value
                auto app = fairWindSK->getAppItemByHash(hash);
                auto position = app->getOrder();

                //qDebug() << "---------AppItem: "  << app->getName() << " IsActive: " << app->getActive() << " order: " << app->getOrder();

                // Check if the app is active
                if (app->getActive()) {
                    map[position] = QPair<AppItem *, QString>(app, hash);
                }

            }

            // Iterate on the available apps' hash values
            for (const auto& item: map) {
                // Get the hash value
                auto appItem = item.first;
                auto name = item.second;

                // Create a new button
                auto *button = new QToolButton();

                // Set the app's hash value as the 's object name
                button->setObjectName("toolbutton_" + name);

                // Set the app's name as the button's text
                button->setText(appItem->getDisplayName());

                // Set the tool tip
                button->setToolTip(appItem->getDescription());

                // Get the application icon
                QPixmap pixmap = appItem->getIcon();

                // Check if the icon is available
                if (!pixmap.isNull()) {
                    // Scale the icon
                    pixmap = pixmap.scaled(m_iconSize, m_iconSize);

                    // Set the app's icon as the button's icon
                    button->setIcon(pixmap);

                    // Give the button's icon a fixed square
                    button->setIconSize(QSize(m_iconSize, m_iconSize));
                }

                // Set the button's style to have an icon and some text beneath it
                button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

                // Launch the app when the button is clicked
                connect(button, &QToolButton::released, this, &Launcher::toolButton_App_released);

                // Add the newly created button to the grid layout as a widget
                m_layout->addWidget(button, row, col);

                // Store in buttons in a map
                m_buttons[name] = button;

                row++;
                if (row == m_rows) {
                    row = 0;
                    col++;
                }

                // qDebug()<< name;
            }

            // Resize the icons
            resize();
        }
    }

    Launcher::~Launcher() {

        // Delete the application icons
        for (const auto button: m_buttons) {

            // Delete the button
            delete button;
        }

        if (m_layout) {
            delete m_layout;
            m_layout = nullptr;
        }

        if (ui) {
            delete ui;
            ui = nullptr;
        }
    }

    /*
 * toolButton_App_released
 * Method called when the user wants to launch an app
 */
    void Launcher::toolButton_App_released() {

        // Get the sender button
        QWidget *buttonWidget = qobject_cast<QWidget *>(sender());

        // Check if the sender is valid
        if (!buttonWidget) {

            // The sender is not a widget
            return;
        }

        // Get the app's hash value from the button's object name
        QString hash = buttonWidget->objectName().replace("toolbutton_", "");

        // Check if the debug is active
        if (FairWindSK::getInstance()->isDebug()) {

            // Write a message
            qDebug() << "Apps - hash:" << hash;
        }

        // Emit the signal to tell the MainWindow to update the UI and show the app with that particular hash value
        emit foregroundAppChanged(hash);
    }

    void Launcher::resizeEvent(QResizeEvent *event) {
        QWidget::resizeEvent(event);

        resize();
    }

    void Launcher::resize() {

        int iconSize;
        iconSize = height() / m_rows;
        iconSize = iconSize - 48*m_rows;

        auto *layout = (QGridLayout *)ui->scrollAreaWidgetContents->layout();

        // Iterate on the columns
        for (int col = 0; col < m_cols; col++) {
            // Set the column width for each column
            layout->setColumnMinimumWidth(col, iconSize);
        }

        // Iterate on the rows
        for (int row = 0; row < m_rows; row++) {
            // Set the row height for each row
            layout->setRowMinimumHeight(row, iconSize);
        }

        for(auto button:m_buttons) {
            // Give the button's icon a fixed square
            button->setIconSize(QSize(iconSize, iconSize));
        }
    }

    void Launcher::onScrollLeft() {
        if (!m_buttons.empty()) {
            ui->scrollArea->horizontalScrollBar()->setValue(
                    ui->scrollArea->horizontalScrollBar()->value() - m_buttons.first()->iconSize().width() * 2
            );
        }
    }

    void Launcher::onScrollRight() {
        if (!m_buttons.empty()) {
            ui->scrollArea->horizontalScrollBar()->setValue(
                    ui->scrollArea->horizontalScrollBar()->value() + m_buttons.first()->iconSize().width() * 2
            );
        }
    }

} // fairwindsk::ui::launcher
