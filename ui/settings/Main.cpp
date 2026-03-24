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
    void Main::setWindowGeometryFieldsEnabled(const QString &windowMode) const {
        const bool isWindowed = windowMode == "windowed";
        const bool isCentered = windowMode == "centered";

        ui->lineEdit_left->setEnabled(isWindowed);
        ui->lineEdit_top->setEnabled(isWindowed);
        ui->lineEdit_width->setEnabled(isWindowed || isCentered);
        ui->lineEdit_height->setEnabled(isWindowed || isCentered);
    }

    void Main::setUiScaleFieldsEnabled(const bool automatic) const {
        ui->comboBox_uiScalePreset->setEnabled(!automatic);
    }

    void Main::applyUiPreview() const {
        FairWindSK::getInstance()->applyUiPreferences(m_settings->getConfiguration());
    }

    Main::Main(Settings *settings, QWidget *parent) :
            QWidget(parent), ui(new Ui::Main) {

        m_settings = settings;

        ui->setupUi(this);

        ui->comboBox_windowMode->addItem(tr("Windowed"));
        ui->comboBox_windowMode->addItem(tr("Centered"));
        ui->comboBox_windowMode->addItem(tr("Maximized"));
        ui->comboBox_windowMode->addItem(tr("Full Screen"));

        ui->comboBox_uiScalePreset->addItem(tr("Small"), "small");
        ui->comboBox_uiScalePreset->addItem(tr("Normal"), "normal");
        ui->comboBox_uiScalePreset->addItem(tr("Large"), "large");
        ui->comboBox_uiScalePreset->addItem(tr("Extra Large"), "xlarge");

        int windowModeIndex = 0;
        const auto windowMode = m_settings->getConfiguration()->getWindowMode();

        if (windowMode == "windowed") {
            windowModeIndex=0;
        } else if (windowMode == "centered") {
            windowModeIndex=1;
        } else if (windowMode == "maximized") {
            windowModeIndex=2;
        } else if (windowMode == "fullscreen") {
            windowModeIndex=3;
        }

        ui->comboBox_windowMode->setCurrentIndex(windowModeIndex);

        const auto uiScalePreset = m_settings->getConfiguration()->getUiScalePreset();
        const auto uiScaleIndex = ui->comboBox_uiScalePreset->findData(uiScalePreset);
        ui->comboBox_uiScalePreset->setCurrentIndex(uiScaleIndex >= 0 ? uiScaleIndex : 1);

        const bool automaticUiScale = m_settings->getConfiguration()->getUiScaleMode() == "auto";
        ui->checkBox_autoUiScale->setCheckState(automaticUiScale ? Qt::Checked : Qt::Unchecked);
        setUiScaleFieldsEnabled(automaticUiScale);

        if (m_settings->getConfiguration()->getVirtualKeyboard()) {
            ui->checkBox_virtualkeboard->setCheckState(Qt::Checked);
        } else {
            ui->checkBox_virtualkeboard->setCheckState(Qt::Unchecked);
        }

        connect(ui->checkBox_virtualkeboard, SIGNAL(stateChanged(int)),
                this, SLOT(onVirtualKeyboardStateChanged(int)));
        connect(ui->checkBox_autoUiScale, SIGNAL(stateChanged(int)),
                this, SLOT(onUiScaleModeStateChanged(int)));

        setWindowGeometryFieldsEnabled(windowMode);

        QScreen *screen = QGuiApplication::primaryScreen();
        auto  screenGeometry = screen->geometry();

        ui->lineEdit_left->setValidator( new QIntValidator(0, screenGeometry.width(), this) );
        ui->lineEdit_top->setValidator( new QIntValidator(0, screenGeometry.height(), this) );
        ui->lineEdit_width->setValidator( new QIntValidator(1, screenGeometry.width(), this) );
        ui->lineEdit_height->setValidator( new QIntValidator(1, screenGeometry.height(), this) );

        ui->lineEdit_left->setText(QString::number(m_settings->getConfiguration()->getWindowLeft()));
        ui->lineEdit_top->setText(QString::number(m_settings->getConfiguration()->getWindowTop()));
        ui->lineEdit_width->setText(QString::number(m_settings->getConfiguration()->getWindowWidth()));
        ui->lineEdit_height->setText(QString::number(m_settings->getConfiguration()->getWindowHeight()));

        connect(ui->comboBox_windowMode,&QComboBox::currentIndexChanged,this, &Main::onWindowModeChanged);

        connect(ui->lineEdit_left,&QLineEdit::textChanged,this, &Main::onWindowLeftTextChanged);
        connect(ui->lineEdit_top,&QLineEdit::textChanged,this, &Main::onWindowTopTextChanged);
        connect(ui->lineEdit_width,&QLineEdit::textChanged,this, &Main::onWindowWidthTextChanged);
        connect(ui->lineEdit_height,&QLineEdit::textChanged,this, &Main::onWindowHeightTextChanged);
        connect(ui->comboBox_uiScalePreset, &QComboBox::currentIndexChanged, this, &Main::onUiScalePresetChanged);

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

                    int currentIndex = 0;
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

    void Main::onUiScaleModeStateChanged(const int state) {
        const bool automatic = state == Qt::Checked;
        m_settings->getConfiguration()->setUiScaleMode(automatic ? "auto" : "manual");
        setUiScaleFieldsEnabled(automatic);
        applyUiPreview();
    }

    void Main::onUiScalePresetChanged(const int index) {
        Q_UNUSED(index);

        m_settings->getConfiguration()->setUiScalePreset(ui->comboBox_uiScalePreset->currentData().toString());

        if (ui->checkBox_autoUiScale->checkState() != Qt::Checked) {
            applyUiPreview();
        }
    }

    void Main::onWindowModeChanged(const int index) {

        QString windowMode="";
        switch (index) {
            case 1:
                windowMode = "centered";
            break;
            case 2:
                windowMode = "maximized";
                break;
            case 3:
                windowMode = "fullscreen";
                break;
            default:
                windowMode = "windowed";
        }
        setWindowGeometryFieldsEnabled(windowMode);
        m_settings->getConfiguration()->setWindowMode(windowMode);


    }

    void Main::onWindowLeftTextChanged() {
        m_settings->getConfiguration()->setWindowLeft(ui->lineEdit_left->text().toInt());
    }

    void Main::onWindowTopTextChanged() {
        m_settings->getConfiguration()->setWindowTop(ui->lineEdit_top->text().toInt());
    }

    void Main::onWindowWidthTextChanged() {
        m_settings->getConfiguration()->setWindowWidth(ui->lineEdit_width->text().toInt());
    }

    void Main::onWindowHeightTextChanged() {
        m_settings->getConfiguration()->setWindowHeight(ui->lineEdit_height->text().toInt());
    }



    void Main::onCurrentIndexChanged(int index) {
        Q_UNUSED(index);
        // get sender
        const auto comboBox = qobject_cast<QComboBox*>(sender());
        if (!comboBox) {
            return;
        }

        m_settings->getConfiguration()->getRoot()["units"][comboBox->objectName().toStdString()] = comboBox->currentData().toString().toStdString();
    }

    Main::~Main() {
        delete ui;
    }

} // fairwindsk::ui::settings
