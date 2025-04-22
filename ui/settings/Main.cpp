//
// Created by Raffaele Montella on 06/05/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_Main.h" resolved

#include <QtWidgets/QComboBox>
#include <QScreen>

#include "Main.hpp"
#include "ui_Main.h"
#include "FairWindSK.hpp"
#include "Units.hpp"

namespace fairwindsk::ui::settings {
    Main::Main(Settings *settings, QWidget *parent) :
            QWidget(parent), ui(new Ui::Main) {

        m_settings = settings;

        ui->setupUi(this);

        ui->comboBox_windowMode->addItem(tr("Windowed"));
        ui->comboBox_windowMode->addItem(tr("Maximized"));
        ui->comboBox_windowMode->addItem(tr("Full Screen"));
        ui->comboBox_windowMode->setCurrentIndex(m_settings->getConfiguration()->getMode());

        if (m_settings->getConfiguration()->getVirtualKeyboard()) {
            ui->checkBox_virtualkeboard->setCheckState(Qt::Checked);
        } else {
            ui->checkBox_virtualkeboard->setCheckState(Qt::Unchecked);
        }

        connect(ui->checkBox_virtualkeboard,&QCheckBox::stateChanged,this, &Main::onVirtualKeyboardStateChanged);


        if (m_settings->getConfiguration()->getMode()) {

            ui->lineEdit_left->setEnabled(false);
            ui->lineEdit_top->setEnabled(false);
            ui->lineEdit_width->setEnabled(false);
            ui->lineEdit_height->setEnabled(false);
        } else {

            ui->lineEdit_left->setEnabled(true);
            ui->lineEdit_top->setEnabled(true);
            ui->lineEdit_width->setEnabled(true);
            ui->lineEdit_height->setEnabled(true);
        }

        QScreen *screen = QGuiApplication::primaryScreen();
        auto  screenGeometry = screen->geometry();

        ui->lineEdit_left->setValidator( new QIntValidator(0, screenGeometry.width(), this) );
        ui->lineEdit_top->setValidator( new QIntValidator(0, screenGeometry.height(), this) );
        ui->lineEdit_width->setValidator( new QIntValidator(0, screenGeometry.width(), this) );
        ui->lineEdit_height->setValidator( new QIntValidator(0, screenGeometry.height(), this) );

        ui->lineEdit_left->setText(QString::number(m_settings->getConfiguration()->getWindowLeft()));
        ui->lineEdit_top->setText(QString::number(m_settings->getConfiguration()->getWindowTop()));
        ui->lineEdit_width->setText(QString::number(m_settings->getConfiguration()->getWindowWidth()));
        ui->lineEdit_height->setText(QString::number(m_settings->getConfiguration()->getWindowHeight()));

        connect(ui->comboBox_windowMode,&QComboBox::currentIndexChanged,this, &Main::onWindowModeChanged);

        connect(ui->lineEdit_left,&QLineEdit::textChanged,this, &Main::onWindowLeftTextChanged);
        connect(ui->lineEdit_top,&QLineEdit::textChanged,this, &Main::onWindowTopTextChanged);
        connect(ui->lineEdit_width,&QLineEdit::textChanged,this, &Main::onWindowWidthTextChanged);
        connect(ui->lineEdit_height,&QLineEdit::textChanged,this, &Main::onWindowHeightTextChanged);

        if (auto units = Units::getInstance()->getUnits(); units.contains("measures") && units["measures"].is_object()) {
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

    void Main::onVirtualKeyboardStateChanged(const int state) {

        auto value = false;

        if (state == Qt::Checked) {
            value = true;
        }

        m_settings->getConfiguration()->setVirtualKeyboard(value);
    }

    void Main::onWindowModeChanged(const int index) {

        auto value = false;

        if (index == 0) {
            value = true;
        }

        m_settings->getConfiguration()->setMode(index);

        ui->lineEdit_left->setEnabled(!value);
        ui->lineEdit_top->setEnabled(!value);
        ui->lineEdit_width->setEnabled(!value);
        ui->lineEdit_height->setEnabled(!value);
    }

    void Main::onWindowLeftTextChanged() {
        m_settings->getConfiguration()->getRoot()["main"]["windowLeft"] = ui->lineEdit_left->text().toInt();
    }

    void Main::onWindowTopTextChanged() {
        m_settings->getConfiguration()->getRoot()["main"]["windowTop"] = ui->lineEdit_top->text().toInt();
    }

    void Main::onWindowWidthTextChanged() {
        m_settings->getConfiguration()->getRoot()["main"]["windowWidth"] = ui->lineEdit_width->text().toInt();
    }

    void Main::onWindowHeightTextChanged() {
        m_settings->getConfiguration()->getRoot()["main"]["windowHeight"] = ui->lineEdit_height->text().toInt();
    }



    void Main::onCurrentIndexChanged(int index) {
        // get sender
        const auto comboBox = qobject_cast<QComboBox*>(sender());

        m_settings->getConfiguration()->getRoot()["units"][comboBox->objectName().toStdString()] = comboBox->currentData().toString().toStdString();
    }

    Main::~Main() {
        delete ui;
    }

} // fairwindsk::ui::settings
