//
// Created by Codex on 01/04/26.
//

#include "TouchCheckBox.hpp"

#include <QEvent>
#include <QPushButton>

#include "ui_TouchCheckBox.h"

namespace fairwindsk::ui::widgets {
    namespace {
        const QString kTouchToggleStyle = QStringLiteral(
            "QPushButton {"
            " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
            " stop:0 #fcfcfd, stop:0.45 #eef2f7, stop:1 #d7dde6);"
            " border: 1px solid #7b8794;"
            " border-top-color: #aeb8c4;"
            " border-bottom-color: #5d6875;"
            " border-radius: 10px;"
            " padding: 4px;"
            " }"
            "QPushButton:hover {"
            " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
            " stop:0 #ffffff, stop:0.45 #f4f7fb, stop:1 #dfe5ee);"
            " }"
            "QPushButton:pressed, QPushButton:checked {"
            " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
            " stop:0 #c8d1dc, stop:0.5 #e3e8ef, stop:1 #f7f9fb);"
            " border-top-color: #596473;"
            " border-bottom-color: #a9b3bf;"
            " padding-top: 5px;"
            " padding-bottom: 3px;"
            " }"
            "QPushButton:disabled {"
            " background: #d9dde3;"
            " color: #9aa3ad;"
            " border-color: #aab3bc;"
            " }");
    }

    TouchCheckBox::TouchCheckBox(QWidget *parent)
        : QWidget(parent),
          ui(new Ui::TouchCheckBox) {
        ui->setupUi(this);

        ui->pushButtonToggle->setObjectName(QStringLiteral("pushButton_touchCheckBox"));
        ui->pushButtonToggle->setCheckable(true);
        ui->pushButtonToggle->setFixedSize(44, 44);
        ui->pushButtonToggle->setStyleSheet(kTouchToggleStyle);
        ui->pushButtonToggle->installEventFilter(this);

        connect(ui->pushButtonToggle, &QPushButton::clicked, this, &TouchCheckBox::onButtonClicked);
        connect(ui->pushButtonToggle, &QPushButton::pressed, this, &TouchCheckBox::onButtonPressed);
        connect(ui->pushButtonToggle, &QPushButton::released, this, &TouchCheckBox::onButtonReleased);

        refreshAppearance();
    }

    TouchCheckBox::~TouchCheckBox() {
        delete ui;
        ui = nullptr;
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
        ui->pushButtonToggle->setIcon(QIcon(iconPath));
    }
}
