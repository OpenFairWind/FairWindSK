//
// Created by Codex on 01/04/26.
//

#include "TouchComboBox.hpp"

#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QMouseEvent>
#include <QPushButton>
#include <QScreen>
#include <QVBoxLayout>

namespace fairwindsk::ui::widgets {
    TouchComboBox::TouchComboBox(QWidget *parent) : QWidget(parent) {
        m_layout = new QHBoxLayout(this);
        m_layout->setContentsMargins(0, 0, 0, 0);
        m_layout->setSpacing(4);

        m_editor = new QLineEdit(this);
        m_editor->setObjectName(QStringLiteral("lineEdit_touchComboBox"));
        m_editor->setReadOnly(true);
        m_editor->setMinimumHeight(44);
        m_editor->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_editor->installEventFilter(this);

        m_button = new QPushButton(this);
        m_button->setObjectName(QStringLiteral("pushButton_touchComboBox"));
        m_button->setIcon(QIcon(QStringLiteral(":/resources/svg/OpenBridge/arrow-down-google.svg")));
        m_button->setMinimumSize(44, 44);
        m_button->setIconSize(QSize(22, 22));

        m_layout->addWidget(m_editor, 1);
        m_layout->addWidget(m_button);

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
        m_listWidget->setUniformItemSizes(true);
        m_listWidget->setSpacing(2);
        popupLayout->addWidget(m_listWidget);

        connect(m_button, &QPushButton::clicked, this, &TouchComboBox::togglePopup);
        connect(m_listWidget, &QListWidget::itemClicked, this, &TouchComboBox::handleItemClicked);

        setFocusProxy(m_editor);
        updateDisplay();
    }

    TouchComboBox::~TouchComboBox() {
        delete m_popup;
        m_popup = nullptr;
    }

    void TouchComboBox::addItem(const QString &text, const QVariant &userData) {
        auto *item = new QListWidgetItem(text, m_listWidget);
        item->setData(Qt::UserRole, userData);
        item->setSizeHint(QSize(item->sizeHint().width(), 44));
        m_listWidget->addItem(item);

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

    void TouchComboBox::setEnabled(const bool enabled) {
        QWidget::setEnabled(enabled);
        if (m_editor) {
            m_editor->setEnabled(enabled);
        }
        if (m_button) {
            m_button->setEnabled(enabled);
        }
        if (!enabled && m_popup) {
            m_popup->hide();
        }
    }

    bool TouchComboBox::eventFilter(QObject *watched, QEvent *event) {
        if (watched == m_editor && event && event->type() == QEvent::MouseButtonRelease) {
            const auto *mouseEvent = static_cast<QMouseEvent *>(event);
            if (mouseEvent->button() == Qt::LeftButton && isEnabled()) {
                togglePopup();
                return true;
            }
        }

        if (watched == m_popup && event && event->type() == QEvent::Hide) {
            m_button->setDown(false);
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
        if (m_currentIndex >= 0 && m_currentIndex < m_listWidget->count()) {
            m_listWidget->setCurrentRow(m_currentIndex);
            m_listWidget->scrollToItem(m_listWidget->item(m_currentIndex), QAbstractItemView::PositionAtCenter);
        }
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
        m_editor->setText(item ? item->text() : QString());
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
        const int visibleRows = qMin(qMax(1, m_listWidget->count()), 6);
        const int rowHeight = 46;
        const int popupHeight = qMax(rowHeight, visibleRows * rowHeight + 4);

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
}
