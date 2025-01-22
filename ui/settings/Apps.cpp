//
// Created by Raffaele Montella on 06/05/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_Apps.h" resolved

#include <QMessageBox>
#include <QFileDialog>
#include <QDomDocument>
#include "Apps.hpp"
#include "ui_Apps.h"
#include "AppItem.hpp"
#include "FairWindSK.hpp"
#include "PList.hpp"


namespace fairwindsk::ui::settings {
    Apps::Apps(Settings *settings, QWidget *parent) :
            QWidget(parent), ui(new Ui::Apps) {
        ui->setupUi(this);

        m_settings = settings;
        m_appsEditMode = false;
        m_appsEditChanged = false;

        // Order by order value
        QMap<int, QPair<AppItem *, QString>> map;

        // Get the configuration json data root
        auto jsonData = m_settings->getConfiguration()->getRoot();

        qDebug() << "Apps::Apps";

        // Check if the configuration has an apps element and if it is an array
        if (jsonData.contains("apps") && jsonData["apps"].is_array()) {

            // For each item of the apps array...
            for (const auto& app: jsonData["apps"].items()) {

                // Get the application data
                auto jsonApp = app.value();

                // Create an application object
                auto appItem = new AppItem(jsonApp);

                qDebug() << "App: " << appItem->getName() << " active: " << appItem->getActive() << " order: " << appItem->getOrder();

                // Get the order and set the position
                auto position = appItem->getOrder();

                // Map the application in the right application
                map[position] = QPair<AppItem *, QString>(appItem, appItem->getName());


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
        }
        connect(ui->listWidget_Apps_List,&QListWidget::itemSelectionChanged, this, &Apps::onAppsListSelectionChanged);
        connect( ui->listWidget_Apps_List, &QListWidget::itemChanged, this, &Apps::onAppsListItemChanged);

        connect(ui->pushButton_Apps_EditSave, &QPushButton::clicked,this,&Apps::onAppsEditSaveClicked);

        connect(ui->lineEdit_Apps_Name, &QLineEdit::textChanged, this, &Apps::onAppsDetailsFieldsTextChanged);
        connect(ui->lineEdit_Apps_Description, &QLineEdit::textChanged, this, &Apps::onAppsDetailsFieldsTextChanged);
        connect(ui->lineEdit_Apps_DisplayName, &QLineEdit::textChanged, this, &Apps::onAppsDetailsFieldsTextChanged);
        connect(ui->lineEdit_Apps_AppIcon, &QLineEdit::textChanged, this, &Apps::onAppsDetailsFieldsTextChanged);

        connect(ui->pushButton_Apps_AppIcon_Browse, &QPushButton::clicked,this,&Apps::onAppsAppIconBrowse);
	connect(ui->pushButton_Apps_Name_Browse, &QPushButton::clicked,this,&Apps::onAppsNameBrowse);

        connect(ui->toolButton_Add, &QToolButton::clicked,this,&Apps::onAppsAddClicked);
        connect(ui->toolButton_Remove, &QToolButton::clicked,this,&Apps::onAppsRemoveClicked);
        connect(ui->toolButton_Up, &QToolButton::clicked,this,&Apps::onAppsUpClicked);
        connect(ui->toolButton_Down, &QToolButton::clicked,this,&Apps::onAppsDownClicked);

        ui->listWidget_Apps_List->installEventFilter(this);
        if (ui->listWidget_Apps_List->count()>0) {
            ui->listWidget_Apps_List->setCurrentRow(0);
        }
    }

    void Apps::saveAppsDetails() {

        // Get the application name
        auto appName =  ui->listWidget_Apps_List->currentItem()->data(Qt::UserRole).toString();

        // Get the index of the application within the apps array
        int idx = m_settings->getConfiguration()->findApp(appName);

        // Check if the app is present
        if (idx != -1) {


            // Set the current app json object
            auto appJsonObject = m_settings->getConfiguration()->getRoot()["apps"].at(idx);

            // Update the configuration
            appJsonObject["name"] = ui->lineEdit_Apps_Name->text().toStdString();
            appJsonObject["description"] = ui->lineEdit_Apps_Description->text().toStdString();
            appJsonObject["signalk"]["displayName"] = ui->lineEdit_Apps_DisplayName->text().toStdString();
            appJsonObject["signalk"]["appIcon"] = ui->lineEdit_Apps_AppIcon->text().toStdString();

            m_settings->getConfiguration()->getRoot()["apps"].at(idx) = appJsonObject;

            auto appItem = new AppItem(appJsonObject);

            auto listWidgetItem =  ui->listWidget_Apps_List->currentItem();
            listWidgetItem->setText(appItem->getDisplayName());
            listWidgetItem->setIcon(QIcon(appItem->getIcon()));
            listWidgetItem->setToolTip(appItem->getDescription());
            listWidgetItem->setData(Qt::UserRole, appItem->getName());

            delete appItem;

            qDebug() << "Updated (from edits): " << QString::fromStdString(m_settings->getConfiguration()->getRoot()["apps"].at(idx).dump(2));

        }

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

        // Enable/disable the browser buttons
        ui->pushButton_Apps_AppIcon_Browse->setEnabled(m_appsEditMode);
        ui->pushButton_Apps_Name_Browse->setEnabled(m_appsEditMode);

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


        setAppsEditMode(false);

        if (ui->listWidget_Apps_List->currentRow()!=-1) {
            auto appName = ui->listWidget_Apps_List->currentItem()->data(Qt::UserRole).toString();

            // Get the index of the application within the apps array
            int idx = m_settings->getConfiguration()->findApp(appName);

            // Check if the app is present
            if (idx != -1) {
                auto appItem = new AppItem(m_settings->getConfiguration()->getRoot()["apps"].at(idx));

                ui->label_Apps_Url_Text->setText(appItem->getUrl());
                ui->label_Apps_Version_Text->setText(appItem->getVersion());
                ui->label_Apps_Author_Text->setText(appItem->getAuthor());
                ui->label_Apps_Vendor_Text->setText(appItem->getVendor());
                ui->label_Apps_Copyright_Text->setText(appItem->getCopyright());
                ui->label_Apps_License_Text->setText(appItem->getLicense());

                ui->lineEdit_Apps_Name->setText(appName);
                ui->lineEdit_Apps_Description->setText(appItem->getDescription());
                ui->lineEdit_Apps_DisplayName->setText(appItem->getDisplayName());
                ui->lineEdit_Apps_AppIcon->setText(appItem->getAppIcon());


                auto icon = appItem->getIcon();

                // Scale the icon
                icon = icon.scaled(128, 128);

                ui->label_Apps_Icon->setPixmap(icon);

                delete appItem;
            }
        } else {
            ui->label_Apps_Url_Text->setText("");
            ui->label_Apps_Version_Text->setText("");
            ui->label_Apps_Author_Text->setText("");
            ui->label_Apps_Vendor_Text->setText("");
            ui->label_Apps_Copyright_Text->setText("");
            ui->label_Apps_License_Text->setText("");

            ui->lineEdit_Apps_Name->setText("");
            ui->lineEdit_Apps_Description->setText("");
            ui->lineEdit_Apps_DisplayName->setText("");
            ui->lineEdit_Apps_AppIcon->setText("");

            QPixmap pixmap = QPixmap::fromImage(QImage(":/resources/images/icons/webapp-256x256.png"));
            ui->label_Apps_Icon->setPixmap(pixmap);
        }

    }

    void Apps::onAppsDetailsFieldsTextChanged(const QString &text) {
        m_appsEditChanged = true;
    }

    void Apps::onAppsAppIconBrowse() {


        auto appIcon = QFileDialog::getOpenFileName(this,tr("Open Image"), "./icons", tr("Image Files (*.png)"));
        if (!appIcon.isEmpty()) {

            QPixmap icon;

            icon.load(appIcon);

            appIcon = "file://" + appIcon;

            ui->lineEdit_Apps_AppIcon->setText(appIcon);


            // Scale the icon
            icon = icon.scaled(128, 128);

            ui->label_Apps_Icon->setPixmap(icon);

            m_appsEditChanged = true;

        }

    }

    void Apps::onAppsNameBrowse() {
        auto name = QFileDialog::getOpenFileName(this,tr("Select executable"));
        if (!name.isEmpty()) {

            qDebug() << name;

            // Check if the application name is a Mac application
            if (name.endsWith(".app")) {

                // Get the file information
                const QFileInfo appPath(name);

                // Check if the name is a path
                if (appPath.isDir()) {

                    // Get the package information file
                    const QFileInfo fileInfo(name+"/Contents/Info.plist");

                    // Check if the file exists
                    if (fileInfo.isFile()) {

                        QFile file(name+"/Contents/Info.plist");
                        if (file.open(QIODevice::ReadOnly)) {
                            auto infoPlist = PList(&file);
                            auto map = infoPlist.toMap();

                            if (map.contains("CFBundleExecutable")) {
                                name = name + "/Contents/MacOS/" + map["CFBundleExecutable"].toString();
                            }
                            file.close();
                        }
                    }
                }
            }

            name = "file://" + name;

            ui->lineEdit_Apps_Name->setText(name);

            m_appsEditChanged = true;

        }
    }



    void Apps::onAppsListItemChanged(QListWidgetItem* listWidgetItem) {

        // Get the application name
        auto appName = listWidgetItem->data(Qt::UserRole).toString();

        // Check the state
        bool active = listWidgetItem->checkState();

        // Get the index of the application within the apps array
        int idx = m_settings->getConfiguration()->findApp(appName);

        // Check if the app is present
        if (idx != -1) {
            // Update the configuration
            m_settings->getConfiguration()->getRoot()["apps"].at(idx)["fairwind"]["active"] = active;

            qDebug() << "Updated (active): " << QString::fromStdString(m_settings->getConfiguration()->getRoot()["apps"].at(idx).dump(2));

        }
    }

    bool Apps::eventFilter(QObject *object, QEvent *event) {


        if ( object == ui->listWidget_Apps_List &&  ( event->type() == QEvent::ChildRemoved )  ) {

            m_appsEditChanged = true;
            for(int row=0; row < ui->listWidget_Apps_List->count();row++){

                // Get the list widget item
                auto listWidgetItem = ui->listWidget_Apps_List->item(row);

                // Get the application name
                auto appName = listWidgetItem->data(Qt::UserRole).toString();

                // Get the index of the application within the apps array
                int idx = m_settings->getConfiguration()->findApp(appName);

                // Check if the app is present
                if (idx != -1) {

                    // Update the configuration
                    m_settings->getConfiguration()->getRoot()["apps"].at(idx)["fairwind"]["order"] = 1+row;

                    qDebug() << "Updated (order): " << QString::fromStdString(m_settings->getConfiguration()->getRoot()["apps"].at(idx).dump(2));

                }
            }
        }

        //return false;

        return QObject::eventFilter(object, event);
    }

    void Apps::onAppsAddClicked() {
        // Get the current item index
        auto pos = ui->listWidget_Apps_List->currentRow();


        // Create an application object
        auto appItem = new AppItem();
        appItem->setName("new_app");

        // Add to the configuration
        m_settings->getConfiguration()->getRoot()["apps"].push_back(appItem->asJson());

        auto listWidgetItem = new QListWidgetItem(appItem->getDisplayName());
        listWidgetItem->setIcon(QIcon(appItem->getIcon()));
        listWidgetItem->setToolTip(appItem->getDescription());
        listWidgetItem->setData(Qt::UserRole, appItem->getName());
        listWidgetItem->setCheckState(Qt::Unchecked);

        // Check if the index is valid
        if (pos >= 0) {
            ui->listWidget_Apps_List->insertItem(pos,listWidgetItem);
            ui->listWidget_Apps_List->setCurrentRow(pos);
        } else {
            ui->listWidget_Apps_List->addItem(listWidgetItem);
            ui->listWidget_Apps_List->setCurrentRow(0);
        }
    }

    void Apps::onAppsRemoveClicked() {

        // Get the current item index
        auto pos = ui->listWidget_Apps_List->currentRow();

        // Check if the index is valid
        if (pos >= 0 && pos < ui->listWidget_Apps_List->count()) {

            auto listWidgetItem = ui->listWidget_Apps_List->item(pos);

            // Get the application name
            auto appName = listWidgetItem->data(Qt::UserRole).toString();

            // Get the index of the application within the apps array
            int idx = m_settings->getConfiguration()->findApp(appName);

            // Check if the app is present
            if (idx != -1) {

                // Update the configuration
                m_settings->getConfiguration()->getRoot()["apps"].erase(idx);
            }

            // Remove the item from the widget list
            ui->listWidget_Apps_List->takeItem(pos);
        }
    }

    void Apps::onAppsUpClicked() {

        // Get the current item index
        auto pos = ui->listWidget_Apps_List->currentRow();

        // Check if the index is valid
        if (pos > 0) {
            auto listWidgetItem = ui->listWidget_Apps_List->takeItem(pos);
            ui->listWidget_Apps_List->insertItem(pos-1,listWidgetItem);
            ui->listWidget_Apps_List->setCurrentRow(pos-1);
        }
    }

    void Apps::onAppsDownClicked() {
        // Get the current item index
        auto pos = ui->listWidget_Apps_List->currentRow();

        // Check if the index is valid
        if (pos != -1 && pos < ui->listWidget_Apps_List->count()-1) {
            auto listWidgetItem = ui->listWidget_Apps_List->takeItem(pos);
            ui->listWidget_Apps_List->insertItem(pos+1,listWidgetItem);
            ui->listWidget_Apps_List->setCurrentRow(pos+1);
        }
    }

    Apps::~Apps() {
        delete ui;
    }




} // fairwindsk::ui::settings
