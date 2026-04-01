//
// Created by Codex on 01/04/26.
//

#include "TouchSpinBox.hpp"

#include <QtMath>
#include <QDoubleValidator>
#include <QLineEdit>
#include <QSignalBlocker>

#include "ui_TouchSpinBox.h"

namespace fairwindsk::ui::widgets {
    TouchSpinBox::TouchSpinBox(QWidget *parent)
        : QWidget(parent),
          ui(new Ui::TouchSpinBox) {
        ui->setupUi(this);

        ui->pushButtonMinus->setObjectName(QStringLiteral("pushButton_touchSpinBoxMinus"));
        ui->pushButtonPlus->setObjectName(QStringLiteral("pushButton_touchSpinBoxPlus"));
        m_editor = ui->lineEditValue;
        m_editor->setObjectName(QStringLiteral("lineEdit_touchSpinBox"));
        m_editor->setAlignment(m_alignment);

        m_validator = new QDoubleValidator(this);
        m_validator->setNotation(QDoubleValidator::StandardNotation);
        m_editor->setValidator(m_validator);

        connect(ui->pushButtonMinus, &QPushButton::clicked, this, &TouchSpinBox::stepDown);
        connect(ui->pushButtonPlus, &QPushButton::clicked, this, &TouchSpinBox::stepUp);
        connect(m_editor, &QLineEdit::editingFinished, this, &TouchSpinBox::applyEditedValue);

        setRange(0.0, 99.0);
        setSingleStep(1.0);
        setDecimals(0);
        setValue(0.0);
    }

    TouchSpinBox::~TouchSpinBox() {
        delete ui;
        ui = nullptr;
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
        refreshValidator();
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
        refreshValidator();
        setValue(m_value);
    }

    void TouchSpinBox::setAlignment(const Qt::Alignment alignment) {
        m_alignment = alignment;
        if (m_editor) {
            m_editor->setAlignment(alignment);
        }
    }

    void TouchSpinBox::stepUp() {
        setValue(m_value + m_singleStep);
    }

    void TouchSpinBox::stepDown() {
        setValue(m_value - m_singleStep);
    }

    void TouchSpinBox::applyEditedValue() {
        bool ok = false;
        const double editedValue = parsedEditorValue(&ok);
        setValue(ok ? editedValue : m_value);
        emit editingFinished();
    }

    void TouchSpinBox::refreshText() {
        if (!m_editor) {
            return;
        }

        const QSignalBlocker blocker(m_editor);
        if (m_decimals == 0) {
            m_editor->setText(QLocale().toString(qRound64(m_value)));
        } else {
            m_editor->setText(QLocale().toString(m_value, 'f', m_decimals));
        }
    }

    void TouchSpinBox::refreshButtonState() {
        if (!ui || !ui->pushButtonMinus || !ui->pushButtonPlus) {
            return;
        }

        ui->pushButtonMinus->setEnabled(isEnabled() && (m_value > m_minimum || !qFuzzyCompare(m_value + 1.0, m_minimum + 1.0)));
        ui->pushButtonPlus->setEnabled(isEnabled() && (m_value < m_maximum || !qFuzzyCompare(m_value + 1.0, m_maximum + 1.0)));
    }

    void TouchSpinBox::refreshValidator() {
        if (!m_validator) {
            return;
        }

        m_validator->setRange(m_minimum, m_maximum, m_decimals);
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

    double TouchSpinBox::parsedEditorValue(bool *ok) const {
        bool parsed = false;
        const double localeValue = QLocale().toDouble(m_editor ? m_editor->text() : QString(), &parsed);
        if (ok) {
            *ok = parsed;
        }
        if (parsed) {
            return localeValue;
        }

        const double fallbackValue = (m_editor ? m_editor->text().toDouble(&parsed) : 0.0);
        if (ok) {
            *ok = parsed;
        }
        return fallbackValue;
    }
}
