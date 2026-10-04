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

                // Read the configured mappings without creating the node as a side effect.
                const auto &root = m_settings->getConfiguration()->getRoot();
                const auto signalkPaths = root.contains("signalk") && root["signalk"].is_object()
                                              ? root["signalk"]
                                              : nlohmann::json::object();

                int row = 1;
                for (const auto &pathItem: m_signalk["paths"].items()) {

                    const auto &key = pathItem.key();

                    // Only catalog entries with a readable description become a row.
                    if (!pathItem.value().is_string()) {
                        continue;
                    }

                    // A mapping missing from an older configuration file is shown empty,
                    // so it can still be filled in from this page.
                    const auto currentPath = signalkPaths.contains(key) && signalkPaths[key].is_string()
                                                 ? signalkPaths[key].get<std::string>()
                                                 : std::string();

                    const auto text = QString::fromStdString(pathItem.value().get<std::string>());

                    const auto textLabel = new QLabel(this);
                    textLabel->setText(text);

                    const auto lineEdit = new QLineEdit(this);
                    lineEdit->setObjectName(QString::fromStdString(key));
                    lineEdit->setText(QString::fromStdString(currentPath));
                    // Signal K paths are identifiers: keep keyboards from "correcting" them.
                    lineEdit->setInputMethodHints(Qt::ImhNoAutoUppercase | Qt::ImhNoPredictiveText);
                    lineEdit->setAccessibleName(text);
                    textLabel->setBuddy(lineEdit);

                    ui->gridLayout_Paths->addWidget(textLabel, row, 1);
                    ui->gridLayout_Paths->addWidget(lineEdit, row, 2);

                    row++;

                    connect(lineEdit, &QLineEdit::textChanged, this, &SignalK::onTextChanged);
                }
            }
        }

        file.close();
    }

    SignalK::~SignalK() {
        delete ui;
    }

    void SignalK::onTextChanged(const QString &text) {
        // Identify the edited mapping from the line edit that emitted the signal.
        const auto lineEdit = qobject_cast<QLineEdit*>(sender());
        if (!lineEdit) {
            return;
        }

        // Repair a missing or malformed "signalk" node before writing into it.
        auto &root = m_settings->getConfiguration()->getRoot();
        if (!root.contains("signalk") || !root["signalk"].is_object()) {
            root["signalk"] = nlohmann::json::object();
        }

        // Stray spaces would silently break the subscription path.
        root["signalk"][lineEdit->objectName().toStdString()] = text.trimmed().toStdString();
        m_settings->markDirty(FairWindSK::RuntimeSignalKPaths, 400);
    }
} // fairwindsk::ui::settings
