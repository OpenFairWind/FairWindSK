//
// Created by __author__ on 21/01/2022.
//

// You may need to build the project (run Qt uic code generator) to get "ui_MainPage.h" resolved

#include <QScreen>
#include <QRect>
#include <QScrollBar>
#include <QGridLayout>

#include "Launcher.hpp"

#include "AppItem.hpp"

namespace fairwindsk::ui {
    Launcher::Launcher(QWidget *parent) :
    QWidget(parent), ui(new Ui::Launcher) {

        ui->setupUi((QWidget *)this);

        mCols = 4;
        mRows = 2;

        // Create a new grid layout
        auto *layout = new QGridLayout(ui->scrollAreaWidgetContents);
        layout->setContentsMargins(0,0,0,0);


        // Set the UI scroll area with the newly created layout
        ui->scrollAreaWidgetContents->setLayout(layout);
        ui->scrollArea->horizontalScrollBar()->setVisible(false);


        int row = 0, col = 0;

        // Get the FairWind singleton
        auto fairWindSK = fairwindsk::FairWindSK::getInstance();

        // Order by order value
        QMap<double, QPair<AppItem *, QString>> map;

        // Populate the inverted list
        /*
        for (auto &hash : fairWindSK->getExtensionsHashes()) {
            // Get the hash value
            auto app = fairWindSK->getAppItemByHash(hash);
            auto position = app->getOrder();

            // Check if the app is active
            if (app->getActive()) {
                map[position] = QPair<AppItem *, QString>(app, hash);
            }

        }
        */

        // Iterate on the available apps' hash values
        for (auto item: map) {
            // Get the hash value
            auto app = item.first;
            auto hash = item.second;

            // Create a new button
            auto *button = new QToolButton();

            // Set the app's hash value as the button's object name
            button->setObjectName("toolbutton_" + hash);

            // Set the app's name as the button's text
            button->setText(app->getName());

            // Get the app's icon
            QImage icon = app->getIcon();

            // Set the app's icon as the button's icon
            button->setIcon(QPixmap::fromImage(icon));

            // Give the button's icon a fixed square
            //button->setIconSize(QSize(iconSize, iconSize));

            // Set the button's style to have an icon and some text beneath it
            button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

            // Launch the app when the button is clicked
            connect(button, &QToolButton::released, this, &Launcher::toolButton_App_released);

            // Add the newly created button to the grid layout as a widget
            layout->addWidget(button, row, col);

            // Store in buttons in a map
            mButtons[hash] = button;

            row++;
            if (row == mRows) {
                row = 0;
                col++;
            }
        }

        // Right scroll
        connect(ui->toolButton_Right, &QToolButton::clicked, this, &Launcher::onScrollRight);

        // Left scroll
        connect(ui->toolButton_Left, &QToolButton::clicked, this, &Launcher::onScrollLeft);

        // Resize the icons
        resize();
    }

    Launcher::~Launcher() {
        delete ui;
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
            return;
        }

        // Get the app's hash value from the button's object name
        QString hash = buttonWidget->objectName().replace("toolbutton_", "");
        qDebug() << "Apps - hash:" << hash;
        // Emit the signal to tell the MainWindow to update the UI and show the app with that particular hash value
        emit foregroundAppChanged(hash);
    }

    void Launcher::resizeEvent(QResizeEvent *event) {
        QWidget::resizeEvent(event);

        resize();
    }

    void Launcher::resize() {

        int iconSize;
        iconSize = height() / mRows;
        iconSize = iconSize - 48*mRows;

        auto *layout = (QGridLayout *)ui->scrollAreaWidgetContents->layout();

        // Iterate on the columns
        for (int col = 0; col < mCols; col++) {
            // Set the column width for each column
            layout->setColumnMinimumWidth(col, iconSize);
        }

        // Iterate on the rows
        for (int row = 0; row < mRows; row++) {
            // Set the row height for each row
            layout->setRowMinimumHeight(row, iconSize);
        }

        for(auto button:mButtons) {
            // Give the button's icon a fixed square
            button->setIconSize(QSize(iconSize, iconSize));
        }
    }

    void Launcher::onScrollLeft() {
        if (mButtons.size()>0) {
            ui->scrollArea->horizontalScrollBar()->setValue(
                    ui->scrollArea->horizontalScrollBar()->value() - mButtons.first()->iconSize().width() * 2
            );
        }
    }

    void Launcher::onScrollRight() {
        if (mButtons.size()>0) {
            ui->scrollArea->horizontalScrollBar()->setValue(
                    ui->scrollArea->horizontalScrollBar()->value() + mButtons.first()->iconSize().width() * 2
            );
        }
    }

} // fairwindsk::apps::launcherax10m
