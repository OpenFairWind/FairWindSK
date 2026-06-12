//
// Created by Codex on 01/04/26.
//

#include "TouchCheckBox.hpp"

#include <QEvent>
#include <QPalette>
#include <QPushButton>
#include <QSizePolicy>

#include "FairWindSK.hpp"
#include "ui/IconUtils.hpp"
#include "ui_TouchCheckBox.h"

namespace fairwindsk::ui::widgets {
    namespace {
        QString touchToggleStyle(const fairwindsk::ui::ComfortChromeColors &colors) {
            const QColor base = colors.buttonBackground;
            const QColor text = colors.buttonText;
            const QColor border = colors.border;
            const QColor light = base.lighter(145);
            const QColor mid = base.lighter(118);
            const QColor dark = base.darker(120);

            return QStringLiteral(
                "QPushButton {"
                " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                " stop:0 %1, stop:0.45 %2, stop:1 %3);"
                " color: %4;"
                " border: 1px solid %5;"
                " border-top-color: %6;"
                " border-bottom-color: %7;"
                " border-radius: 10px;"
                " padding: 4px;"
                " }"
                "QPushButton:hover {"
                " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                " stop:0 %8, stop:0.45 %9, stop:1 %10);"
                " }"
                "QPushButton:pressed, QPushButton:checked {"
                " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                " stop:0 %11, stop:0.5 %2, stop:1 %12);"
                " border-top-color: %7;"
                " border-bottom-color: %6;"
                " padding-top: 5px;"
                " padding-bottom: 3px;"
                " }"
                "QPushButton:disabled {"
                " background: %13;"
                " color: %14;"
                " border-color: %15;"
                " }")
                .arg(light.name(), mid.name(), dark.name(), text.name(), border.name(),
                     light.darker(108).name(), dark.name(),
                     colors.hoverBackground.name(QColor::HexArgb), mid.lighter(108).name(), dark.lighter(108).name(),
                     colors.accentBottom.darker(108).name(), colors.accentTop.name(),
                     fairwindsk::ui::comfortAlpha(colors.buttonBackground, 46).name(QColor::HexArgb),
                     colors.disabledText.name(),
                     colors.disabledText.darker(125).name());
        }

        fairwindsk::ui::ComfortChromeColors checkboxColors(const QPalette &palette) {
            auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
            const auto *configuration = fairWindSK ? fairWindSK->getConfiguration() : nullptr;
            const QString preset = fairWindSK
                ? fairWindSK->getActiveComfortViewPreset(configuration)
                : QStringLiteral("default");
            return fairwindsk::ui::resolveComfortChromeColors(configuration, preset, palette, false);
        }
    }

    TouchCheckBox::TouchCheckBox(QWidget *parent)
        : QWidget(parent),
          ui(new Ui::TouchCheckBox) {
        ui->setupUi(this);

        ui->pushButtonToggle->setObjectName(QStringLiteral("pushButton_touchCheckBox"));
        ui->pushButtonToggle->setCheckable(true);
        ui->pushButtonToggle->setMinimumSize(QSize(56, 52));
        ui->pushButtonToggle->setMaximumSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));
        ui->pushButtonToggle->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        ui->pushButtonToggle->installEventFilter(this);
        setMinimumHeight(52);
        setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

        connect(ui->pushButtonToggle, &QPushButton::clicked, this, &TouchCheckBox::onButtonClicked);
        connect(ui->pushButtonToggle, &QPushButton::pressed, this, &TouchCheckBox::onButtonPressed);
        connect(ui->pushButtonToggle, &QPushButton::released, this, &TouchCheckBox::onButtonReleased);

        applyTouchStyle();
        refreshAppearance();
    }

    TouchCheckBox::~TouchCheckBox() {
        delete ui;
        ui = nullptr;
    }

    bool TouchCheckBox::event(QEvent *event) {
        if (event && (event->type() == QEvent::PaletteChange || event->type() == QEvent::ApplicationPaletteChange)) {
            applyTouchStyle();
        }
        return QWidget::event(event);
    }

    QString TouchCheckBox::text() const {
        return ui->pushButtonToggle->text();
    }

    bool TouchCheckBox::isChecked() const {
        return m_checkState == Qt::Checked;
    }

    Qt::CheckState TouchCheckBox::checkState() const {
        return m_checkState;
    }

    bool TouchCheckBox::isTristate() const {
        return m_tristate;
    }

    QSize TouchCheckBox::iconSize() const {
        return ui->pushButtonToggle->iconSize();
    }

    QPushButton *TouchCheckBox::button() const {
        return ui->pushButtonToggle;
    }

    void TouchCheckBox::setText(const QString &text) {
        ui->pushButtonToggle->setText(text);
        ui->pushButtonToggle->setMinimumWidth(text.trimmed().isEmpty() ? 56 : 118);
        updateGeometry();
    }

    void TouchCheckBox::setChecked(const bool checked) {
        setCheckState(checked ? Qt::Checked : Qt::Unchecked);
    }

    void TouchCheckBox::setCheckState(const Qt::CheckState state) {
        applyState(state, true);
    }

    void TouchCheckBox::setTristate(const bool tristate) {
        m_tristate = tristate;
    }

    void TouchCheckBox::setIconSize(const QSize &size) {
        ui->pushButtonToggle->setIconSize(size);
    }

    void TouchCheckBox::toggle() {
        advanceState();
    }

    void TouchCheckBox::click() {
        ui->pushButtonToggle->click();
    }

    void TouchCheckBox::animateClick() {
        ui->pushButtonToggle->animateClick();
    }

    void TouchCheckBox::setEnabled(const bool enabled) {
        QWidget::setEnabled(enabled);
        ui->pushButtonToggle->setEnabled(enabled);
    }

    void TouchCheckBox::applyTouchStyle() {
        const QString style = touchToggleStyle(checkboxColors(palette()));
        if (m_toggleStyleSheet == style) {
            refreshAppearance();
            return;
        }

        m_toggleStyleSheet = style;
        ui->pushButtonToggle->setStyleSheet(m_toggleStyleSheet);
        refreshAppearance();
    }

    bool TouchCheckBox::eventFilter(QObject *watched, QEvent *event) {
        if (watched == ui->pushButtonToggle && event && event->type() == QEvent::EnabledChange) {
            refreshAppearance();
        }

        return QWidget::eventFilter(watched, event);
    }

    void TouchCheckBox::onButtonClicked() {
        advanceState();
        emit clicked(isChecked());
    }

    void TouchCheckBox::onButtonPressed() {
        emit pressed();
    }

    void TouchCheckBox::onButtonReleased() {
        emit released();
    }

    void TouchCheckBox::advanceState() {
        Qt::CheckState nextState = Qt::Unchecked;
        if (m_tristate) {
            switch (m_checkState) {
                case Qt::Unchecked:
                    nextState = Qt::PartiallyChecked;
                    break;
                case Qt::PartiallyChecked:
                    nextState = Qt::Checked;
                    break;
                case Qt::Checked:
                default:
                    nextState = Qt::Unchecked;
                    break;
            }
        } else {
            nextState = (m_checkState == Qt::Checked) ? Qt::Unchecked : Qt::Checked;
        }

        applyState(nextState, true);
    }

    void TouchCheckBox::applyState(const Qt::CheckState state, const bool emitSignals) {
        const Qt::CheckState normalizedState =
            (!m_tristate && state == Qt::PartiallyChecked) ? Qt::Checked : state;
        const bool changed = normalizedState != m_checkState;

        m_checkState = normalizedState;
        refreshAppearance();

        if (changed && emitSignals) {
            emit toggled(isChecked());
            emit stateChanged(static_cast<int>(m_checkState));
        }
    }

    void TouchCheckBox::refreshAppearance() {
        QString iconPath = QStringLiteral(":/resources/svg/OpenBridge/touch-checkbox-unchecked.svg");
        if (m_checkState == Qt::Checked) {
            iconPath = QStringLiteral(":/resources/svg/OpenBridge/touch-checkbox-checked.svg");
        } else if (m_checkState == Qt::PartiallyChecked) {
            iconPath = QStringLiteral(":/resources/svg/OpenBridge/touch-checkbox-partial.svg");
        }

        ui->pushButtonToggle->setChecked(m_checkState != Qt::Unchecked);
        const auto colors = checkboxColors(palette());
        ui->pushButtonToggle->setIcon(
            fairwindsk::ui::tintedIcon(
                QIcon(iconPath),
                m_checkState == Qt::Unchecked ? colors.buttonText : colors.accentText,
                ui->pushButtonToggle->iconSize()));
    }
}
