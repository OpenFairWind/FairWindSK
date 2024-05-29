//
// Created by Raffaele Montella on 06/05/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_Main.h" resolved

#include <QtWidgets/QComboBox>
#include "Main.hpp"
#include "ui_Main.h"
#include "FairWindSK.hpp"
#include "Units.hpp"

namespace fairwindsk::ui::settings {
    Main::Main(Settings *settings, QWidget *parent) :
            QWidget(parent), ui(new Ui::Main) {

        m_settings = settings;

        ui->setupUi(this);

        if (m_settings->getConfiguration()->getVirtualKeyboard()) {
            ui->checkBox_virtualkeboard->setCheckState(Qt::Checked);
        } else {
            ui->checkBox_virtualkeboard->setCheckState(Qt::Unchecked);
        }

        auto units = Units::getInstance()->getUnits();
        if (units.contains("measures") && units["measures"].is_object()) {
            int row = 1;
            for (const auto& measureItem: units["measures"].items()) {

                auto currentUnit = m_settings->getConfiguration()->getRoot()["units"][measureItem.key()].get<std::string>();

                auto text = QString::fromStdString(units["measures"][measureItem.key()]["text"].get<std::string>());
                auto type = units["measures"][measureItem.key()]["type"].get<std::string>();

                auto textLabel = new QLabel();
                textLabel->setText(text);
                auto comboBox = new QComboBox();
                comboBox->setObjectName(QString::fromStdString(measureItem.key()));

                ui->gridLayout_Measures->addWidget(textLabel, row, 1);
                ui->gridLayout_Measures->addWidget(comboBox, row, 2);
                row ++;
                connect(comboBox,&QComboBox::currentIndexChanged,this, &Main::onCurrentIndexChanged);



                if (units.contains("types") && units["types"].is_object()) {

                    int currentIndex;
                    int idx = 0;
                    for (const auto &typeItem: units["types"].items()) {
                        if (units["types"][typeItem.key()]["type"] == type) {

                            auto typeLabel =
                                    units["types"][typeItem.key()]["label"].get<std::string>();

                            auto typeText =
                                    units["types"][typeItem.key()]["text"].get<std::string>() +
                                    " (" + typeLabel + ")";

                            comboBox->addItem(QString::fromStdString(typeText), QString::fromStdString(typeItem.key()));

                            if (currentUnit == typeItem.key()) {
                                currentIndex = idx;
                            }
                            idx++;
                        }
                    }
                    comboBox->setCurrentIndex(currentIndex);
                }
            }
        }
    }

    void Main::onVirtualKeyboard(int state) {

        auto fairWindSK = FairWindSK::getInstance();

        if (state == Qt::Checked) {
            m_settings->getConfiguration()->setVirtualKeyboard(true);
        } else {
            m_settings->getConfiguration()->setVirtualKeyboard(false);
        }
    }

    Main::~Main() {
        delete ui;
    }

    void Main::onCurrentIndexChanged(int index) {
        // get sender
        auto comboBox = qobject_cast<QComboBox*>(sender());

        m_settings->getConfiguration()->getRoot()["units"][comboBox->objectName().toStdString()] = comboBox->currentData().toString().toStdString();
    }
} // fairwindsk::ui::settings
