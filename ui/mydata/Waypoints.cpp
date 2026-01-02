//
// Created by Raffaele Montella on 15/02/25.
//

// You may need to build the project (run Qt uic code generator) to get "ui_Waypoints.h" resolved

#include "Waypoints.hpp"


#include "Files.hpp"
#include "ui_Waypoints.h"


namespace fairwindsk::ui::mydata {

    Waypoints::Waypoints(MyData *myData, QWidget *parent) :
            QWidget(parent), ui(new Ui::Waypoints) {

        m_myData = myData;

        ui->setupUi(this);

        m_waypointsModel = new WaypointsModel();


        ui->tableView_Waypoints->setModel(m_waypointsModel);
        //ui->tableView_Waypoints->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        ui->tableView_Waypoints->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

        connect(ui->toolButton_Search, &QToolButton::clicked, this, &Waypoints::onSearchClicked);
        connect(ui->lineEdit_Search,&QLineEdit::returnPressed, this, &Waypoints::onSearchClicked);

        connect(ui->toolButton_Import, &QToolButton::clicked, this, &Waypoints::onImportClicked);
        connect(ui->toolButton_Export, &QToolButton::clicked, this, &Waypoints::onExportClicked);

        connect(ui->toolButton_NavigateTo, &QToolButton::clicked, this, &Waypoints::onNavigateToClicked);

        connect(ui->toolButton_Add, &QToolButton::clicked, this, &Waypoints::onAddClicked);
        connect(ui->toolButton_Edit, &QToolButton::clicked, this, &Waypoints::onEditClicked);
        connect(ui->toolButton_Delete, &QToolButton::clicked, this, &Waypoints::onDeleteClicked);

        ui->tableView_Waypoints->sortByColumn(WaypointsModel::Columns::Name,Qt::AscendingOrder);

    }

    void Waypoints::onSearchClicked() {
        qDebug() << "WaypointsModel::onSearchClicked";
    }

    void Waypoints::onImportClicked() {
        qDebug() << "Waypoints::onImportClicked";
    }

    void Waypoints::onExportClicked() {
        qDebug() << "Waypoints::onExportClicked";
    }

    void Waypoints::onNavigateToClicked() {
        qDebug() << "Waypoints::onNavigateToClicked";
    }

    void Waypoints::onAddClicked() {
        qDebug() << "Waypoints::onAddClicked";

    }

    void Waypoints::onEditClicked() {
        qDebug() << "WaypointsModel::onEditClicked";
        ui->tableView_Waypoints->edit(ui->tableView_Waypoints->currentIndex());

    }

    void Waypoints::onDeleteClicked() {
        qDebug() << "WaypointsModel::onDeleteClicked";
        m_waypointsModel->removeRow(ui->tableView_Waypoints->currentIndex().row());

    }

    Waypoints::~Waypoints() {
        if (m_waypointsModel) {
            delete m_waypointsModel;
            m_waypointsModel = nullptr;
        }

        delete ui;
    }
} // fairwindsk::ui::mydata
