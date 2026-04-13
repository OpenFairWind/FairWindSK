//
// Created by Codex on 01/04/26.
//

#include "TouchSpinBox.hpp"

#include <QtMath>
#include <QEvent>
#include <QLabel>
#include <QPalette>

#include "FairWindSK.hpp"
#include "ui/IconUtils.hpp"
#include "ui_TouchSpinBox.h"

namespace fairwindsk::ui::widgets {
    namespace {
        struct TouchSpinBoxColors {
            QColor buttonTop;
            QColor buttonMid;
            QColor buttonBottom;
            QColor pressedTop;
            QColor pressedBottom;
            QColor border;
            QColor text;
            QColor disabled;
            QColor disabledText;
            QColor disabledBorder;
            QColor valueBackground;
            QColor valueBorder;
            QColor valueText;
        };

        TouchSpinBoxColors effectiveColors(const QPalette &palette) {
            TouchSpinBoxColors colors;
            auto *fairWindSK = fairwindsk::FairWindSK::getInstance();
            const auto *configuration = fairWindSK ? fairWindSK->getConfiguration() : nullptr;
            const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset(configuration) : QStringLiteral("default");

            colors.buttonTop = palette.color(QPalette::Button).lighter(145);
            colors.buttonMid = palette.color(QPalette::Button).lighter(118);
            colors.buttonBottom = palette.color(QPalette::Button).darker(120);
            colors.pressedTop = palette.color(QPalette::Button).darker(115);
            colors.pressedBottom = palette.color(QPalette::Button).lighter(118);
            colors.border = palette.color(QPalette::Mid);
            colors.text = fairwindsk::ui::comfortIconColor(
                configuration,
                preset,
                fairwindsk::ui::bestContrastingColor(
                    palette.color(QPalette::Button),
                    {palette.color(QPalette::Text),
                     palette.color(QPalette::ButtonText),
                     palette.color(QPalette::WindowText)}));
            colors.disabled = palette.color(QPalette::AlternateBase);
            colors.disabledText = palette.color(QPalette::Disabled, QPalette::ButtonText);
            colors.disabledBorder = palette.color(QPalette::Disabled, QPalette::Mid);
            colors.valueBackground = palette.color(QPalette::Base);
            colors.valueBorder = palette.color(QPalette::Mid);
            colors.valueText = palette.color(QPalette::Text);
            return colors;
        }

        QString touchButtonStyle(const TouchSpinBoxColors &colors) {
            
            return QStringLiteral(
                "QPushButton {"
                " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                " stop:0 %1, stop:0.45 %2, stop:1 %3);"
                " color: %4;"
                " border: 1px solid %5;"
                " border-top-color: %6;"
                " border-bottom-color: %7;"
                " border-radius: 8px;"
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
                .arg(colors.buttonTop.name(), colors.buttonMid.name(), colors.buttonBottom.name(), colors.text.name(), colors.border.name(),
                     colors.buttonTop.darker(108).name(), colors.buttonBottom.name(),
                     colors.buttonTop.lighter(110).name(), colors.buttonMid.lighter(108).name(), colors.buttonBottom.lighter(108).name(),
                     colors.pressedTop.name(), colors.pressedBottom.name(), colors.disabled.name(),
                     colors.disabledText.name(), colors.disabledBorder.name());
        }

        QString touchValueStyle(const TouchSpinBoxColors &colors) {
            return QStringLiteral(
                "QLabel {"
                " min-height: 44px;"
                " padding: 0 12px;"
                " border: 1px solid %1;"
                " border-radius: 10px;"
                " background: %2;"
                " color: %3;"
                " font-weight: 600;"
                " }"
                "QLabel:disabled {"
                " background: %4;"
                " color: %5;"
                " border-color: %6;"
                " }")
                .arg(colors.valueBorder.name(),
                     colors.valueBackground.name(),
                     colors.valueText.name(),
                     colors.disabled.name(),
                     colors.disabledText.name(),
                     colors.disabledBorder.name());
        }
    }

    TouchSpinBox::TouchSpinBox(QWidget *parent)
        : QWidget(parent),
          ui(new Ui::TouchSpinBox) {
        ui->setupUi(this);

        ui->pushButtonMinus->setObjectName(QStringLiteral("pushButton_touchSpinBoxMinus"));
        ui->pushButtonPlus->setObjectName(QStringLiteral("pushButton_touchSpinBoxPlus"));
        m_valueLabel = ui->labelValue;
        m_valueLabel->setObjectName(QStringLiteral("label_touchSpinBoxValue"));
        m_valueLabel->setAlignment(m_alignment);

        connect(ui->pushButtonMinus, &QPushButton::clicked, this, &TouchSpinBox::stepDown);
        connect(ui->pushButtonPlus, &QPushButton::clicked, this, &TouchSpinBox::stepUp);

        applyTouchStyle();
        setRange(0.0, 99.0);
        setSingleStep(1.0);
        setDecimals(0);
        setValue(0.0);
    }

    TouchSpinBox::~TouchSpinBox() {
        delete ui;
        ui = nullptr;
    }

    bool TouchSpinBox::event(QEvent *event) {
        if (event) {
            if (event->type() == QEvent::PaletteChange || event->type() == QEvent::ApplicationPaletteChange) {
                applyTouchStyle();
            } else if (event->type() == QEvent::EnabledChange) {
                refreshButtonState();
                if (m_valueLabel) {
                    m_valueLabel->setEnabled(isEnabled());
                }
            }
        }
        return QWidget::event(event);
    }

    double TouchSpinBox::minimum() const {
        return m_minimum;
    }

    double TouchSpinBox::maximum() const {
        return m_maximum;
    }

    double TouchSpinBox::value() const {
        return m_value;
    }

    double TouchSpinBox::singleStep() const {
        return m_singleStep;
    }

    int TouchSpinBox::decimals() const {
        return m_decimals;
    }

    Qt::Alignment TouchSpinBox::alignment() const {
        return m_alignment;
    }

    void TouchSpinBox::setMinimum(const double minimum) {
        setRange(minimum, m_maximum);
    }

    void TouchSpinBox::setMaximum(const double maximum) {
        setRange(m_minimum, maximum);
    }

    void TouchSpinBox::setRange(double minimum, double maximum) {
        if (minimum > maximum) {
            std::swap(minimum, maximum);
        }

        m_minimum = minimum;
        m_maximum = maximum;
        setValue(m_value);
    }

    void TouchSpinBox::setValue(const int value) {
        setValue(static_cast<double>(value));
    }

    void TouchSpinBox::setValue(const double value) {
        const double normalized = normalizedValue(value);
        const bool changed = !qFuzzyCompare(m_value + 1.0, normalized + 1.0);
        const int previousIntValue = qRound(m_value);

        m_value = normalized;
        refreshText();
        refreshButtonState();

        if (changed) {
            emit valueChanged(m_value);
            const int currentIntValue = qRound(m_value);
            if (currentIntValue != previousIntValue || m_decimals == 0) {
                emit valueChanged(currentIntValue);
            }
        }
    }

    void TouchSpinBox::setSingleStep(const double singleStep) {
        m_singleStep = singleStep > 0.0 ? singleStep : 1.0;
        refreshButtonState();
    }

    void TouchSpinBox::setDecimals(const int decimals) {
        m_decimals = qMax(0, decimals);
        setValue(m_value);
    }

    void TouchSpinBox::setAlignment(const Qt::Alignment alignment) {
        m_alignment = alignment;
        if (m_valueLabel) {
            m_valueLabel->setAlignment(alignment);
        }
    }

    void TouchSpinBox::stepUp() {
        setValue(m_value + m_singleStep);
    }

    void TouchSpinBox::stepDown() {
        setValue(m_value - m_singleStep);
    }

    void TouchSpinBox::applyTouchStyle() {
        const QPalette activePalette = palette();
        const TouchSpinBoxColors colors = effectiveColors(activePalette);
        const QString buttonStyle = touchButtonStyle(colors);
        if (m_buttonStyleSheet != buttonStyle) {
            m_buttonStyleSheet = buttonStyle;
            ui->pushButtonMinus->setStyleSheet(m_buttonStyleSheet);
            ui->pushButtonPlus->setStyleSheet(m_buttonStyleSheet);
        }

        if (m_valueLabel) {
            m_valueLabel->setStyleSheet(touchValueStyle(colors));
        }

        fairwindsk::ui::applyTintedButtonIcon(ui->pushButtonMinus, colors.text);
        fairwindsk::ui::applyTintedButtonIcon(ui->pushButtonPlus, colors.text);
    }

    void TouchSpinBox::refreshText() {
        if (!m_valueLabel) {
            return;
        }

        if (m_decimals == 0) {
            m_valueLabel->setText(QLocale().toString(qRound64(m_value)));
        } else {
            m_valueLabel->setText(QLocale().toString(m_value, 'f', m_decimals));
        }
    }

    void TouchSpinBox::refreshButtonState() {
        if (!ui || !ui->pushButtonMinus || !ui->pushButtonPlus) {
            return;
        }

        ui->pushButtonMinus->setEnabled(isEnabled() && (m_value > m_minimum || !qFuzzyCompare(m_value + 1.0, m_minimum + 1.0)));
        ui->pushButtonPlus->setEnabled(isEnabled() && (m_value < m_maximum || !qFuzzyCompare(m_value + 1.0, m_maximum + 1.0)));
    }

    double TouchSpinBox::normalizedValue(const double value) const {
        double normalized = qBound(m_minimum, value, m_maximum);
        if (m_decimals == 0) {
            normalized = std::round(normalized);
        } else {
            const double factor = std::pow(10.0, m_decimals);
            normalized = std::round(normalized * factor) / factor;
        }
        return normalized;
    }
}
