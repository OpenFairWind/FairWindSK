//
// Created by Raffaele Montella on 06/05/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_Main.h" resolved

#include <QScreen>
#include <QIntValidator>

#include "Main.hpp"
#include "ui_Main.h"
#include "FairWindSK.hpp"
#include "ui/GeoCoordinateUtils.hpp"
#include "ui/widgets/TouchCheckBox.hpp"
#include "ui/widgets/TouchComboBox.hpp"
#include "ui/widgets/TouchSpinBox.hpp"

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
        for (const auto &option : fairwindsk::ui::geo::coordinateFormatOptions()) {
            ui->comboBox_coordinateFormat->addItem(option.label, option.id);
        }

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
        ui->spinBox_launcherRows->setValue(m_settings->getConfiguration()->getLauncherRows());
        ui->spinBox_launcherColumns->setValue(m_settings->getConfiguration()->getLauncherColumns());
        const int coordinateFormatIndex = ui->comboBox_coordinateFormat->findData(m_settings->getConfiguration()->getCoordinateFormat());
        ui->comboBox_coordinateFormat->setCurrentIndex(coordinateFormatIndex >= 0 ? coordinateFormatIndex : 0);

        if (m_settings->getConfiguration()->getVirtualKeyboard()) {
            ui->checkBox_virtualkeboard->setCheckState(Qt::Checked);
        } else {
            ui->checkBox_virtualkeboard->setCheckState(Qt::Unchecked);
        }

        connect(ui->checkBox_virtualkeboard,
                &fairwindsk::ui::widgets::TouchCheckBox::stateChanged,
                this,
                &Main::onVirtualKeyboardStateChanged);
        connect(ui->checkBox_autoUiScale,
                &fairwindsk::ui::widgets::TouchCheckBox::stateChanged,
                this,
                &Main::onUiScaleModeStateChanged);

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

        connect(ui->comboBox_windowMode,
                qOverload<int>(&fairwindsk::ui::widgets::TouchComboBox::currentIndexChanged),
                this,
                &Main::onWindowModeChanged);

        connect(ui->lineEdit_left,&QLineEdit::textChanged,this, &Main::onWindowLeftTextChanged);
        connect(ui->lineEdit_top,&QLineEdit::textChanged,this, &Main::onWindowTopTextChanged);
        connect(ui->lineEdit_width,&QLineEdit::textChanged,this, &Main::onWindowWidthTextChanged);
        connect(ui->lineEdit_height,&QLineEdit::textChanged,this, &Main::onWindowHeightTextChanged);
        connect(ui->comboBox_uiScalePreset,
                qOverload<int>(&fairwindsk::ui::widgets::TouchComboBox::currentIndexChanged),
                this,
                &Main::onUiScalePresetChanged);
        connect(ui->spinBox_launcherRows, qOverload<int>(&fairwindsk::ui::widgets::TouchSpinBox::valueChanged), this, &Main::onLauncherRowsValueChanged);
        connect(ui->spinBox_launcherColumns, qOverload<int>(&fairwindsk::ui::widgets::TouchSpinBox::valueChanged), this, &Main::onLauncherColumnsValueChanged);
        connect(ui->comboBox_coordinateFormat,
                qOverload<int>(&fairwindsk::ui::widgets::TouchComboBox::currentIndexChanged),
                this,
                &Main::onCoordinateFormatChanged);

    }

    void Main::onVirtualKeyboardStateChanged(const int state) {

        auto value = false;

        if (state == Qt::Checked) {
            value = true;
        }

        m_settings->getConfiguration()->setVirtualKeyboard(value);
        m_settings->markDirty(FairWindSK::RuntimeUi, 0);
    }

    void Main::onUiScaleModeStateChanged(const int state) {
        const bool automatic = state == Qt::Checked;
        m_settings->getConfiguration()->setUiScaleMode(automatic ? "auto" : "manual");
        setUiScaleFieldsEnabled(automatic);
        m_settings->markDirty(FairWindSK::RuntimeUi, 0);
    }

    void Main::onUiScalePresetChanged(const int index) {
        Q_UNUSED(index);

        m_settings->getConfiguration()->setUiScalePreset(ui->comboBox_uiScalePreset->currentData().toString());
        m_settings->markDirty(FairWindSK::RuntimeUi, 0);
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
        m_settings->markDirty(FairWindSK::RuntimeUi, 0);


    }

    void Main::onWindowLeftTextChanged() {
        m_settings->getConfiguration()->setWindowLeft(ui->lineEdit_left->text().toInt());
        m_settings->markDirty(FairWindSK::RuntimeUi, 300);
    }

    void Main::onWindowTopTextChanged() {
        m_settings->getConfiguration()->setWindowTop(ui->lineEdit_top->text().toInt());
        m_settings->markDirty(FairWindSK::RuntimeUi, 300);
    }

    void Main::onWindowWidthTextChanged() {
        m_settings->getConfiguration()->setWindowWidth(ui->lineEdit_width->text().toInt());
        m_settings->markDirty(FairWindSK::RuntimeUi, 300);
    }

    void Main::onWindowHeightTextChanged() {
        m_settings->getConfiguration()->setWindowHeight(ui->lineEdit_height->text().toInt());
        m_settings->markDirty(FairWindSK::RuntimeUi, 300);
    }

    void Main::onLauncherRowsValueChanged(const int value) {
        m_settings->getConfiguration()->setLauncherRows(value);
        m_settings->markDirty(FairWindSK::RuntimeUi, 0);
    }

    void Main::onLauncherColumnsValueChanged(const int value) {
        m_settings->getConfiguration()->setLauncherColumns(value);
        m_settings->markDirty(FairWindSK::RuntimeUi, 0);
    }

    void Main::onCoordinateFormatChanged(const int) {
        m_settings->getConfiguration()->setCoordinateFormat(ui->comboBox_coordinateFormat->currentData().toString());
        m_settings->markDirty(FairWindSK::RuntimeUi, 0);
    }
    Main::~Main() {
        delete ui;
    }

} // fairwindsk::ui::settings
