//
// Created by Codex on 01/04/26.
//

#include "TouchComboBox.hpp"

#include <QEvent>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMouseEvent>
#include <QPalette>
#include <QPixmap>
#include <QPushButton>
#include <QScreen>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>
#include <QVBoxLayout>

#include "ui/IconUtils.hpp"
#include "ui_TouchComboBox.h"

namespace fairwindsk::ui::widgets {
    namespace {
        constexpr int kRawIconRole = Qt::UserRole + 1;
        constexpr int kComboIconSize = 30;
        constexpr int kSelectedIconSize = 34;
        constexpr int kSelectedIconPadding = 10;
        constexpr int kMinimumComboItemHeight = 60;

        QString touchButtonStyle(const QPalette &palette, const bool accentButton) {
            const QColor base = accentButton ? palette.color(QPalette::Highlight) : palette.color(QPalette::Button);
            const QColor text = accentButton ? palette.color(QPalette::HighlightedText) : palette.color(QPalette::ButtonText);
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
                " min-height: 52px;"
                " padding: 8px 12px;"
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

        QColor comboForegroundColor(const QPalette &palette) {
            return fairwindsk::ui::bestContrastingColor(
                palette.color(QPalette::Base),
                {palette.color(QPalette::Text),
                 palette.color(QPalette::ButtonText),
                 palette.color(QPalette::WindowText)});
        }

        int comboItemHeightForFont(const QFont &font) {
            const QFontMetrics metrics(font);
            return qMax(kMinimumComboItemHeight, metrics.height() + 26);
        }

        class PopupItemDelegate final : public QStyledItemDelegate {
        public:
            explicit PopupItemDelegate(QObject *parent = nullptr)
                : QStyledItemDelegate(parent) {
            }

            void setPopupFont(const QFont &font) {
                m_popupFont = font;
            }

            [[nodiscard]] QSize sizeHint(const QStyleOptionViewItem &option,
                                         const QModelIndex &index) const override {
                QSize hint = QStyledItemDelegate::sizeHint(option, index);
                hint.setHeight(qMax(hint.height(), comboItemHeightForFont(m_popupFont)));
                return hint;
            }

        protected:
            void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override {
                QStyledItemDelegate::initStyleOption(option, index);
                option->font = m_popupFont;
                option->fontMetrics = QFontMetrics(m_popupFont);
            }

        private:
            QFont m_popupFont;
        };
    }

    TouchComboBox::TouchComboBox(QWidget *parent)
        : QWidget(parent),
          ui(new Ui::TouchComboBox) {
        ui->setupUi(this);

        m_editor = ui->lineEditValue;
        m_editor->setObjectName(QStringLiteral("lineEdit_touchComboBox"));
        m_editor->installEventFilter(this);
        m_iconLabel = new QLabel(m_editor);
        m_iconLabel->setFixedSize(kSelectedIconSize, kSelectedIconSize);
        m_iconLabel->setAlignment(Qt::AlignCenter);
        m_iconLabel->hide();

        ui->pushButtonPopup->setObjectName(QStringLiteral("pushButton_touchComboBox"));

        m_popup = new QFrame(this);
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
        m_listWidget->setIconSize(QSize(kComboIconSize, kComboIconSize));
        m_listWidget->setSpacing(4);
        m_popupDelegate = new PopupItemDelegate(m_listWidget);
        m_listWidget->setItemDelegate(m_popupDelegate);
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
        if (event && (event->type() == QEvent::PaletteChange
                      || event->type() == QEvent::ApplicationPaletteChange
                      || event->type() == QEvent::FontChange
                      || event->type() == QEvent::ApplicationFontChange)) {
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
        item->setData(kRawIconRole, icon);
        const QFont popupFont = m_editor ? m_editor->font() : font();
        item->setFont(popupFont);
        item->setSizeHint(QSize(item->sizeHint().width(), comboItemHeightForFont(popupFont)));

        if (m_currentIndex < 0) {
            setCurrentIndex(0);
        } else {
            applyTouchStyle();
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
        if (item) {
            return item->text();
        }

        return m_editable && m_editor ? m_editor->text() : QString();
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

    bool TouchComboBox::accentButton() const {
        return m_accentButton;
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

    void TouchComboBox::setAccentButton(const bool accent) {
        if (m_accentButton == accent) {
            return;
        }
        m_accentButton = accent;
        applyTouchStyle();
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
        const QString buttonStyle = touchButtonStyle(activePalette, m_accentButton);
        if (m_buttonStyleSheet != buttonStyle) {
            m_buttonStyleSheet = buttonStyle;
            ui->pushButtonPopup->setStyleSheet(m_buttonStyleSheet);
        }
        fairwindsk::ui::applyTintedButtonIcon(
            ui->pushButtonPopup,
            fairwindsk::ui::bestContrastingColor(
                m_accentButton ? activePalette.color(QPalette::Highlight) : activePalette.color(QPalette::Button),
                {m_accentButton ? activePalette.color(QPalette::HighlightedText) : activePalette.color(QPalette::Text),
                 activePalette.color(QPalette::ButtonText),
                 activePalette.color(QPalette::WindowText)}));

        if (m_popup) {
            const QString popupStyleSheet = popupStyle(activePalette);
            if (m_popupStyleSheet != popupStyleSheet) {
                m_popupStyleSheet = popupStyleSheet;
                m_popup->setStyleSheet(m_popupStyleSheet);
            }
        }

        const QFont popupFont = m_editor ? m_editor->font() : font();
        m_listWidget->setFont(popupFont);
        m_listWidget->setIconSize(QSize(kComboIconSize, kComboIconSize));
        if (auto *delegate = static_cast<PopupItemDelegate *>(m_popupDelegate)) {
            delegate->setPopupFont(popupFont);
        }

        const QColor iconColor = comboForegroundColor(activePalette);
        for (int i = 0; i < m_listWidget->count(); ++i) {
            auto *item = m_listWidget->item(i);
            if (!item) {
                continue;
            }

            const QIcon rawIcon = qvariant_cast<QIcon>(item->data(kRawIconRole));
            item->setFont(popupFont);
            item->setSizeHint(QSize(item->sizeHint().width(), comboItemHeightForFont(popupFont)));
            if (!rawIcon.isNull()) {
                item->setIcon(fairwindsk::ui::tintedIcon(rawIcon, iconColor, QSize(kComboIconSize, kComboIconSize)));
            }
        }

        updateDisplay();
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
        if (m_editor && m_iconLabel) {
            const int y = qMax(0, (m_editor->height() - m_iconLabel->height()) / 2);
            m_iconLabel->move(kSelectedIconPadding, y);
        }
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
        if (m_iconLabel) {
            const QPalette activePalette = palette();
            const QColor iconColor = comboForegroundColor(activePalette);
            const QIcon icon = item
                ? fairwindsk::ui::tintedIcon(
                    qvariant_cast<QIcon>(item->data(kRawIconRole)),
                    iconColor,
                    QSize(kSelectedIconSize, kSelectedIconSize))
                : QIcon();
            if (!icon.isNull()) {
                m_iconLabel->setPixmap(icon.pixmap(QSize(kSelectedIconSize, kSelectedIconSize)));
                m_iconLabel->show();
                m_editor->setTextMargins(kSelectedIconSize + (2 * kSelectedIconPadding), 0, 8, 0);
            } else {
                m_iconLabel->clear();
                m_iconLabel->hide();
                m_editor->setTextMargins(8, 0, 8, 0);
            }
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

        QWidget *host = window();
        if (!host) {
            host = parentWidget();
        }
        if (!host) {
            host = this;
        }
        if (m_popup->parentWidget() != host) {
            m_popup->setParent(host);
        }

        const QPoint hostBottomLeft = host->mapFromGlobal(mapToGlobal(rect().bottomLeft()));
        const QRect hostGeometry = host->rect().isValid() ? host->rect() : QRect(hostBottomLeft, QSize(width(), 240));
        const int popupWidth = width();
        const int popupHeight = qMin(popupHeightForItems(), hostGeometry.height());

        int x = hostBottomLeft.x();
        int y = hostBottomLeft.y();

        if ((x + popupWidth) > hostGeometry.right()) {
            x = hostGeometry.right() - popupWidth;
        }

        if ((y + popupHeight) > hostGeometry.bottom()) {
            y = host->mapFromGlobal(mapToGlobal(rect().topLeft())).y() - popupHeight;
        }

        m_popup->setGeometry(qMax(hostGeometry.left(), x),
                             qMax(hostGeometry.top(), y),
                             popupWidth,
                             popupHeight);
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
