//
// Created by Codex on 16/04/26.
//

#include "Regions.hpp"

#include "ui_Regions.h"

namespace fairwindsk::ui::mydata {

    Regions::Regions(QWidget *parent)
        : ResourceCollectionPageBase(ResourceKind::Region, parent),
          ui(new Ui::Regions) {
        ui->setupUi(this);
        bindPageUi(ui->labelTitle,
                   ui->lineEditSearch,
                   ui->tableWidget,
                   ui->toolButtonOpen,
                   ui->toolButtonEdit,
                   ui->toolButtonAdd,
                   ui->toolButtonDelete,
                   ui->toolButtonImport,
                   ui->toolButtonExport,
                   ui->toolButtonRefresh);
    }

    Regions::~Regions() {
        delete ui;
    }

    QString Regions::pageTitle() const {
        return tr("Regions");
    }

    QString Regions::searchPlaceholderText() const {
        return tr("Search regions by name, description, geometry, or timestamp");
    }
}
