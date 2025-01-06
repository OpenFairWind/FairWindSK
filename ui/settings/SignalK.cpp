//
// Created by Raffaele Montella on 06/05/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_SignalK.h" resolved

#include <QFile>
#include <QtWidgets/QLabel>
#include <QLineEdit>
#include "SignalK.hpp"
#include "ui_SignalK.h"
#include "FairWindSK.hpp"

namespace fairwindsk::ui::settings {
    SignalK::SignalK(Settings *settings, QWidget *parent) :
            QWidget(parent), ui(new Ui::SignalK) {

        m_settings = settings;

        ui->setupUi(this);

        QString data;
        QString fileName(":/resources/json/signalk.json");

        QFile file(fileName);
        if(file.open(QIODevice::ReadOnly)) {

            data = file.readAll();
            m_signalk = nlohmann::json::parse(data.toStdString());

            if (m_signalk.contains("paths") && m_signalk["paths"].is_object()) {

                auto signalkPaths =  m_settings->getConfiguration()->getRoot()["signalk"];

                int row = 1;
                for (const auto &pathItem: m_signalk["paths"].items()) {

                    auto key = pathItem.key();


                    if (signalkPaths.contains(key) && signalkPaths[key].is_string()) {

                        auto currentPath = signalkPaths[key].get<std::string>();

                        if (m_signalk["paths"].contains(key) && m_signalk["paths"][key].is_string()) {

                            auto text = QString::fromStdString(m_signalk["paths"][key].get<std::string>());

                            const auto textLabel = new QLabel();
                            textLabel->setText(text);

                            const auto lineEdit = new QLineEdit();
                            lineEdit->setObjectName(QString::fromStdString(key));
                            lineEdit->setText(QString::fromStdString(currentPath));

                            ui->gridLayout_Paths->addWidget(textLabel, row, 1);
                            ui->gridLayout_Paths->addWidget(lineEdit, row, 2);

                            row++;

                            connect(lineEdit, &QLineEdit::textChanged, this, &SignalK::onTextChanged);
                        }
                    }
                }
            }
        }

        file.close();
    }

    SignalK::~SignalK() {
        delete ui;
    }

    void SignalK::onTextChanged(const QString &text) {
        // get sender
        auto lineEdit = qobject_cast<QLineEdit*>(sender());

        qDebug() << "onTextChanged: " << lineEdit->objectName() << " --> " << lineEdit->text();

        m_settings->getConfiguration()->getRoot()["signalk"][lineEdit->objectName().toStdString()] = lineEdit->text().toStdString();
    }
} // fairwindsk::ui::settings
