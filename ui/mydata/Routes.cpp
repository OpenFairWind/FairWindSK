//
// Created by Codex on 16/04/26.
//

#include "Routes.hpp"

#include "ui_Routes.h"

namespace fairwindsk::ui::mydata {

    Routes::Routes(QWidget *parent)
        : ResourceCollectionPageBase(ResourceKind::Route, parent),
          ui(new Ui::Routes) {
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

    Routes::~Routes() {
        delete ui;
    }

    QString Routes::pageTitle() const {
        return tr("Routes");
    }

    QString Routes::searchPlaceholderText() const {
        return tr("Search routes by name, description, geometry, or timestamp");
    }
}
