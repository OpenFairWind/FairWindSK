//
// Created by Codex on 01/04/26.
//

#include "TouchComboBox.hpp"

#include <QEvent>
#include <QFrame>
#include <QAction>
#include <QLineEdit>
#include <QListWidget>
#include <QMouseEvent>
#include <QPalette>
#include <QPushButton>
#include <QScreen>
#include <QVBoxLayout>

#include "ui_TouchComboBox.h"

namespace fairwindsk::ui::widgets {
    namespace {
        QString touchButtonStyle(const QPalette &palette) {
            const QColor base = palette.color(QPalette::Button);
            const QColor text = palette.color(QPalette::ButtonText);
            const QColor border = palette.color(QPalette::Mid);
            const QColor light = base.lighter(145);
            const QColor mid = base.lighter(118);
            const QColor dark = base.darker(120);
            const QColor disabled = palette.color(QPalette::AlternateBase);

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
                .arg(light.name(), mid.name(), dark.name(), text.name(), border.name(),
                     light.darker(108).name(), dark.name(),
                     light.lighter(110).name(), mid.lighter(108).name(), dark.lighter(108).name(),
                     base.darker(115).name(), base.lighter(120).name(), disabled.name(),
                     palette.color(QPalette::Disabled, QPalette::ButtonText).name(),
                     palette.color(QPalette::Disabled, QPalette::Mid).name());
        }

        QString popupStyle(const QPalette &palette) {
            return QStringLiteral(
                "QFrame#frame_touchComboPopup {"
                " background: %1;"
                " border: 1px solid %2;"
                " border-radius: 8px;"
                " }"
                "QListWidget#listWidget_touchComboBox {"
                " background: %3;"
                " color: %4;"
                " border: none;"
                " outline: none;"
                " }"
                "QListWidget#listWidget_touchComboBox::item {"
                " min-height: 44px;"
                " padding: 6px 10px;"
                " }"
                "QListWidget#listWidget_touchComboBox::item:selected {"
                " background: %5;"
                " color: %6;"
                " }")
                .arg(palette.color(QPalette::Window).name(),
                     palette.color(QPalette::Mid).name(),
                     palette.color(QPalette::Base).name(),
                     palette.color(QPalette::Text).name(),
                     palette.color(QPalette::Highlight).name(),
                     palette.color(QPalette::HighlightedText).name());
        }
    }

    TouchComboBox::TouchComboBox(QWidget *parent)
        : QWidget(parent),
          ui(new Ui::TouchComboBox) {
        ui->setupUi(this);

        m_editor = ui->lineEditValue;
        m_editor->setObjectName(QStringLiteral("lineEdit_touchComboBox"));
        m_editor->installEventFilter(this);
        m_iconAction = m_editor->addAction(QIcon(), QLineEdit::LeadingPosition);
        m_iconAction->setVisible(false);

        ui->pushButtonPopup->setObjectName(QStringLiteral("pushButton_touchComboBox"));

        m_popup = new QFrame(nullptr, Qt::Popup | Qt::FramelessWindowHint);
        m_popup->setObjectName(QStringLiteral("frame_touchComboPopup"));
        m_popup->setFrameShape(QFrame::StyledPanel);
        m_popup->setLineWidth(1);
        m_popup->installEventFilter(this);

        auto *popupLayout = new QVBoxLayout(m_popup);
        popupLayout->setContentsMargins(0, 0, 0, 0);
        popupLayout->setSpacing(0);

        m_listWidget = new QListWidget(m_popup);
        m_listWidget->setObjectName(QStringLiteral("listWidget_touchComboBox"));
        m_listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
        m_listWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        m_listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_listWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        m_listWidget->setSpacing(2);
        popupLayout->addWidget(m_listWidget);

        connect(ui->pushButtonPopup, &QPushButton::clicked, this, &TouchComboBox::togglePopup);
        connect(m_listWidget, &QListWidget::itemClicked, this, &TouchComboBox::handleItemClicked);
        connect(m_editor, &QLineEdit::textChanged, this, &TouchComboBox::editTextChanged);

        setFocusProxy(m_editor);
        applyTouchStyle();
        updateDisplay();
    }

    TouchComboBox::~TouchComboBox() {
        delete m_popup;
        m_popup = nullptr;
        delete ui;
        ui = nullptr;
    }

    bool TouchComboBox::event(QEvent *event) {
        if (event && (event->type() == QEvent::PaletteChange || event->type() == QEvent::ApplicationPaletteChange)) {
            applyTouchStyle();
        }
        return QWidget::event(event);
    }

    void TouchComboBox::addItem(const QString &text, const QVariant &userData) {
        addItem(QIcon(), text, userData);
    }

    void TouchComboBox::addItem(const QIcon &icon, const QString &text, const QVariant &userData) {
        auto *item = new QListWidgetItem(icon, text, m_listWidget);
        item->setData(Qt::UserRole, userData);
        item->setSizeHint(QSize(item->sizeHint().width(), 44));

        if (m_currentIndex < 0) {
            setCurrentIndex(0);
        }
    }

    void TouchComboBox::clear() {
        m_listWidget->clear();
        m_currentIndex = -1;
        updateDisplay();
    }

    int TouchComboBox::count() const {
        return m_listWidget->count();
    }

    int TouchComboBox::currentIndex() const {
        return m_currentIndex;
    }

    QString TouchComboBox::currentText() const {
        const auto *item = m_listWidget->item(m_currentIndex);
        return item ? item->text() : QString();
    }

    QVariant TouchComboBox::currentData(const int role) const {
        const auto *item = m_listWidget->item(m_currentIndex);
        return item ? item->data(role) : QVariant();
    }

    QVariant TouchComboBox::itemData(const int index, const int role) const {
        const auto *item = m_listWidget->item(index);
        return item ? item->data(role) : QVariant();
    }

    int TouchComboBox::findData(const QVariant &data, const int role) const {
        for (int i = 0; i < m_listWidget->count(); ++i) {
            if (m_listWidget->item(i)->data(role) == data) {
                return i;
            }
        }
        return -1;
    }

    int TouchComboBox::findText(const QString &text, const Qt::MatchFlags flags) const {
        for (int i = 0; i < m_listWidget->count(); ++i) {
            const QString itemText = m_listWidget->item(i)->text();
            if ((flags & Qt::MatchCaseSensitive) != 0) {
                if (itemText == text) {
                    return i;
                }
            } else {
                if (QString::compare(itemText, text, Qt::CaseInsensitive) == 0) {
                    return i;
                }
            }
        }
        return -1;
    }

    bool TouchComboBox::isEditable() const {
        return m_editable;
    }

    void TouchComboBox::setCurrentIndex(const int index) {
        if (index < 0 || index >= m_listWidget->count()) {
            if (m_currentIndex != -1) {
                m_currentIndex = -1;
                updateDisplay();
                emit currentIndexChanged(-1);
                emit currentTextChanged(QString());
            }
            return;
        }

        if (m_currentIndex == index) {
            updateDisplay();
            return;
        }

        m_currentIndex = index;
        updateDisplay();
        emit currentIndexChanged(m_currentIndex);
        emit currentTextChanged(currentText());
    }

    void TouchComboBox::setCurrentText(const QString &text) {
        const int existingIndex = findText(text, Qt::MatchExactly);
        if (existingIndex >= 0) {
            setCurrentIndex(existingIndex);
            return;
        }

        if (m_editable) {
            if (m_currentIndex != -1) {
                m_currentIndex = -1;
                emit currentIndexChanged(-1);
            }
            if (m_editor->text() != text) {
                m_editor->setText(text);
            }
            emit currentTextChanged(text);
        }
    }

    void TouchComboBox::setEditable(const bool editable) {
        m_editable = editable;
        if (m_editor) {
            m_editor->setReadOnly(!editable);
            m_editor->setFocusPolicy(editable ? Qt::StrongFocus : Qt::ClickFocus);
        }
    }

    void TouchComboBox::setEnabled(const bool enabled) {
        QWidget::setEnabled(enabled);
        if (m_editor) {
            m_editor->setEnabled(enabled);
        }
        if (ui && ui->pushButtonPopup) {
            ui->pushButtonPopup->setEnabled(enabled);
        }
        if (!enabled && m_popup) {
            m_popup->hide();
        }
    }

    void TouchComboBox::removeItem(const int index) {
        if (index < 0 || index >= m_listWidget->count()) {
            return;
        }

        delete m_listWidget->takeItem(index);

        if (m_listWidget->count() == 0) {
            setCurrentIndex(-1);
            return;
        }

        if (m_currentIndex == index) {
            setCurrentIndex(qMin(index, m_listWidget->count() - 1));
        } else if (m_currentIndex > index) {
            --m_currentIndex;
            updateDisplay();
        }
    }

    void TouchComboBox::applyTouchStyle() {
        const QPalette activePalette = palette();
        ui->pushButtonPopup->setStyleSheet(touchButtonStyle(activePalette));
        if (m_popup) {
            m_popup->setStyleSheet(popupStyle(activePalette));
        }
    }

    bool TouchComboBox::eventFilter(QObject *watched, QEvent *event) {
        if (watched == m_editor && event && event->type() == QEvent::MouseButtonRelease) {
            const auto *mouseEvent = static_cast<QMouseEvent *>(event);
            if (mouseEvent->button() == Qt::LeftButton && isEnabled() && !m_editable) {
                togglePopup();
                return true;
            }
        }

        if (watched == m_popup && event && event->type() == QEvent::Hide) {
            ui->pushButtonPopup->setDown(false);
        }

        return QWidget::eventFilter(watched, event);
    }

    void TouchComboBox::resizeEvent(QResizeEvent *event) {
        QWidget::resizeEvent(event);
        if (m_popup && m_popup->isVisible()) {
            positionPopup();
        }
    }

    void TouchComboBox::togglePopup() {
        if (!m_popup || !m_listWidget || !isEnabled() || m_listWidget->count() == 0) {
            return;
        }

        if (m_popup->isVisible()) {
            m_popup->hide();
            return;
        }

        positionPopup();
        m_popup->show();
        m_popup->raise();
        ensureCurrentItemVisible();
    }

    void TouchComboBox::handleItemClicked(QListWidgetItem *item) {
        if (!item) {
            return;
        }

        const int clickedIndex = m_listWidget->row(item);
        setCurrentIndex(clickedIndex);
        emit activated(clickedIndex);
        m_popup->hide();
    }

    void TouchComboBox::updateDisplay() {
        if (!m_editor || !m_listWidget) {
            return;
        }

        const auto *item = m_listWidget->item(m_currentIndex);
        if (!m_editable || item) {
            m_editor->setText(item ? item->text() : QString());
        }
        if (m_iconAction) {
            const QIcon icon = item ? item->icon() : QIcon();
            m_iconAction->setIcon(icon);
            m_iconAction->setVisible(!icon.isNull());
        }
        if (item) {
            m_listWidget->setCurrentRow(m_currentIndex);
        } else {
            m_listWidget->clearSelection();
        }
    }

    void TouchComboBox::positionPopup() {
        if (!m_popup || !m_listWidget) {
            return;
        }

        const QPoint globalBottomLeft = mapToGlobal(rect().bottomLeft());
        const QRect screenGeometry = screen() ? screen()->availableGeometry() : QRect(globalBottomLeft, QSize(width(), 240));
        const int popupWidth = width();
        const int popupHeight = qMin(popupHeightForItems(), screenGeometry.height());

        int x = globalBottomLeft.x();
        int y = globalBottomLeft.y();

        if ((x + popupWidth) > screenGeometry.right()) {
            x = screenGeometry.right() - popupWidth;
        }

        if ((y + popupHeight) > screenGeometry.bottom()) {
            y = mapToGlobal(rect().topLeft()).y() - popupHeight;
        }

        m_popup->setGeometry(x, y, popupWidth, popupHeight);
    }

    void TouchComboBox::ensureCurrentItemVisible() const {
        if (m_currentIndex >= 0 && m_currentIndex < m_listWidget->count()) {
            m_listWidget->setCurrentRow(m_currentIndex);
            m_listWidget->scrollToItem(m_listWidget->item(m_currentIndex), QAbstractItemView::PositionAtCenter);
        }
    }

    int TouchComboBox::popupHeightForItems() const {
        if (!m_listWidget) {
            return 0;
        }

        int height = 0;
        for (int i = 0; i < m_listWidget->count(); ++i) {
            int rowHeight = m_listWidget->sizeHintForRow(i);
            if (rowHeight <= 0) {
                const auto *item = m_listWidget->item(i);
                rowHeight = item ? qMax(44, item->sizeHint().height()) : 44;
            }
            height += rowHeight;
        }

        const int frame = (m_popup ? m_popup->frameWidth() * 2 : 0);
        const QMargins margins = m_listWidget->contentsMargins();
        const int spacing = qMax(0, m_listWidget->count() - 1) * m_listWidget->spacing();
        return qMax(44, height + spacing + margins.top() + margins.bottom() + frame);
    }
}
