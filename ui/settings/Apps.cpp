//
// Created by Raffaele Montella on 06/05/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_Apps.h" resolved

#include <QMessageBox>
#include "Apps.hpp"
#include "ui_Apps.h"
#include "AppItem.hpp"
#include "FairWindSK.hpp"

namespace fairwindsk::ui::settings {
    Apps::Apps(QWidget *parent) :
            QWidget(parent), ui(new Ui::Apps) {
        ui->setupUi(this);

        m_appsEditMode = false;

        auto fairWindSK = FairWindSK::getInstance();

        // Order by order value
        QMap<int, QPair<AppItem *, QString>> map;

        // Populate the inverted list
        for (auto &hash : fairWindSK->getAppsHashes()) {
            // Get the hash value
            auto app = fairWindSK->getAppItemByHash(hash);
            auto position = app->getOrder();

            map[position] = QPair<AppItem *, QString>(app, hash);
        }

        // Iterate on the available apps' hash values
        for (const auto& item: map) {
            // Get the hash value
            auto appItem = item.first;
            auto name = item.second;
            auto listWidgetItem = new QListWidgetItem(appItem->getDisplayName());
            listWidgetItem->setIcon(QIcon(appItem->getIcon()));
            listWidgetItem->setToolTip(appItem->getDescription());
            listWidgetItem->setData(Qt::UserRole, name);
            if (appItem->getActive()) {
                listWidgetItem->setCheckState(Qt::Checked);
            } else {
                listWidgetItem->setCheckState(Qt::Unchecked);
            }
            ui->listWidget_Apps_List->addItem(listWidgetItem);
        }

        connect(ui->listWidget_Apps_List,&QListWidget::itemSelectionChanged, this, &Apps::onAppsListSelectionChanged);

        connect(ui->pushButton_Apps_EditSave, &QPushButton::clicked,this,&Apps::onAppsEditSaveClicked);

        connect(ui->lineEdit_Apps_Name, &QLineEdit::textChanged, this, &Apps::onAppsDetailsFieldsTextChanged);
        connect(ui->lineEdit_Apps_Description, &QLineEdit::textChanged, this, &Apps::onAppsDetailsFieldsTextChanged);
        connect(ui->lineEdit_Apps_DisplayName, &QLineEdit::textChanged, this, &Apps::onAppsDetailsFieldsTextChanged);
        connect(ui->lineEdit_Apps_AppIcon, &QLineEdit::textChanged, this, &Apps::onAppsDetailsFieldsTextChanged);
    }

    void Apps::saveAppsDetails() {

        // Reset the apps edit changed flag
        m_appsEditChanged = false;
    }

    void Apps::setAppsEditMode(bool editMode) {

        // Check if in edit mode
        if (m_appsEditMode) {

            // Check if any change has been made
            if (m_appsEditChanged) {

                QMessageBox msgBox;
                msgBox.setWindowTitle("Save changes");
                msgBox.setText("Do you want save application definition edits?");
                msgBox.setStandardButtons(QMessageBox::Yes);
                msgBox.addButton(QMessageBox::No);
                msgBox.setDefaultButton(QMessageBox::No);
                if (msgBox.exec() == QMessageBox::Yes) {
                    // Save details
                    saveAppsDetails();
                }
            }
        }

        // Set the dit mode
        m_appsEditMode = editMode;

        // Set the edit save button text
        if (m_appsEditMode)
            ui->pushButton_Apps_EditSave->setText("Save");
        else
            ui->pushButton_Apps_EditSave->setText("Edit");


        // Set the read only or write status
        ui->lineEdit_Apps_Name->setReadOnly(!m_appsEditMode);
        ui->lineEdit_Apps_Description->setReadOnly(!m_appsEditMode);
        ui->lineEdit_Apps_DisplayName->setReadOnly(!m_appsEditMode);
        ui->lineEdit_Apps_AppIcon->setReadOnly(!m_appsEditMode);

        // Enable/disable the app icon browser
        ui->pushButton_Apps_AppIcon_Browse->setEnabled(m_appsEditMode);

        // Reset the changes flag
        m_appsEditChanged = false;
    }

    void Apps::onAppsEditSaveClicked() {
        // Check if in edit mode
        if (m_appsEditMode) {
            saveAppsDetails();
            setAppsEditMode(false);
        } else {
            setAppsEditMode(true);
        }
    }
    void Apps::onAppsListSelectionChanged() {
        auto fairWindSk = FairWindSK::getInstance();

        setAppsEditMode(false);

        auto name = ui->listWidget_Apps_List->currentItem()->data(Qt::UserRole).toString();
        auto appItem = fairWindSk->getAppItemByHash(name);

        ui->label_Apps_Url_Text->setText(appItem->getUrl());
        ui->label_Apps_Version_Text->setText(appItem->getVersion());
        ui->label_Apps_Author_Text->setText(appItem->getAuthor());
        ui->label_Apps_Vendor_Text->setText(appItem->getVendor());
        ui->label_Apps_Copyright_Text->setText(appItem->getCopyright());
        ui->label_Apps_License_Text->setText(appItem->getLicense());

        ui->lineEdit_Apps_Name->setText(name);
        ui->lineEdit_Apps_Description->setText(appItem->getDescription());
        ui->lineEdit_Apps_DisplayName->setText(appItem->getDisplayName());
        ui->lineEdit_Apps_AppIcon->setText(appItem->getAppIcon());


        auto icon = appItem->getIcon();

        // Scale the icon
        icon = icon.scaled(128, 128);

        ui->label_Apps_Icon->setPixmap(icon);



    }

    void Apps::onAppsDetailsFieldsTextChanged(const QString &text) {
        m_appsEditChanged = true;
    }

    Apps::~Apps() {
        delete ui;
    }
} // fairwindsk::ui::settings