//
// Created by Codex on 05/04/26.
//

#include "TouchColorPicker.hpp"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QSlider>
#include <QToolButton>
#include <QVBoxLayout>
#include <functional>

namespace fairwindsk::ui::widgets {
    namespace {
        class PreviewFrame final : public QFrame {
        public:
            using QFrame::QFrame;
            std::function<void()> onActivated;

        protected:
            void mouseDoubleClickEvent(QMouseEvent *event) override {
                if (event->button() == Qt::LeftButton) {
                    if (onActivated) {
                        onActivated();
                    }
                    event->accept();
                    return;
                }
                QFrame::mouseDoubleClickEvent(event);
            }
        };

        class SwatchButton final : public QToolButton {
        public:
            explicit SwatchButton(const QColor &color, QWidget *parent = nullptr)
                : QToolButton(parent),
                  m_color(color) {
            }
            std::function<void(const QColor &, bool)> onChosen;

        protected:
            void mouseReleaseEvent(QMouseEvent *event) override {
                QToolButton::mouseReleaseEvent(event);
                if (event->button() == Qt::LeftButton && rect().contains(event->pos()) && onChosen) {
                    onChosen(m_color, false);
                }
            }

            void mouseDoubleClickEvent(QMouseEvent *event) override {
                if (event->button() == Qt::LeftButton && onChosen) {
                    onChosen(m_color, true);
                    event->accept();
                    return;
                }
                QToolButton::mouseDoubleClickEvent(event);
            }

        private:
            QColor m_color;
        };

        QString sliderStyleSheet() {
            return QStringLiteral(
                "QSlider::groove:horizontal {"
                " height: 16px;"
                " border-radius: 8px;"
                " background: rgba(255, 255, 255, 0.25);"
                " }"
                "QSlider::handle:horizontal {"
                " width: 34px;"
                " margin: -14px 0;"
                " border-radius: 17px;"
                " border: 1px solid rgba(12, 18, 28, 0.75);"
                " background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #fdfefe, stop:1 #d7dfe8);"
                " }");
        }

        QString normalizedHexInput(const QString &text, const bool alphaEnabled) {
            QString value = text.trimmed();
            if (!value.startsWith('#')) {
                value.prepend('#');
            }

            const QRegularExpression expression(alphaEnabled
                                                    ? QStringLiteral("^#([0-9A-Fa-f]{6}|[0-9A-Fa-f]{8})$")
                                                    : QStringLiteral("^#[0-9A-Fa-f]{6}$"));
            return expression.match(value).hasMatch() ? value : QString();
        }
    }

    TouchColorPicker::TouchColorPicker(QWidget *parent)
        : QWidget(parent) {
        auto *rootLayout = new QVBoxLayout(this);
        rootLayout->setContentsMargins(0, 0, 0, 0);
        rootLayout->setSpacing(16);

        auto *headerLayout = new QHBoxLayout();
        headerLayout->setContentsMargins(0, 0, 0, 0);
        headerLayout->setSpacing(14);
        rootLayout->addLayout(headerLayout);

        auto *preview = new PreviewFrame(this);
        preview->setMinimumSize(168, 132);
        preview->setFrameShape(QFrame::StyledPanel);
        preview->onActivated = [this]() {
            emit colorActivated(m_color);
        };
        m_preview = preview;
        headerLayout->addWidget(m_preview, 0);

        auto *summaryLayout = new QVBoxLayout();
        summaryLayout->setContentsMargins(0, 0, 0, 0);
        summaryLayout->setSpacing(8);
        headerLayout->addLayout(summaryLayout, 1);

        auto *hexLabel = new QLabel(tr("Hex"), this);
        summaryLayout->addWidget(hexLabel);

        m_hexEdit = new QLineEdit(this);
        m_hexEdit->setPlaceholderText(QStringLiteral("#RRGGBB"));
        m_hexEdit->setMinimumHeight(48);
        summaryLayout->addWidget(m_hexEdit);

        m_rgbLabel = new QLabel(this);
        summaryLayout->addWidget(m_rgbLabel);

        m_hsvLabel = new QLabel(this);
        summaryLayout->addWidget(m_hsvLabel);
        summaryLayout->addStretch(1);

        auto *swatchesLayout = new QHBoxLayout();
        swatchesLayout->setContentsMargins(0, 0, 0, 0);
        swatchesLayout->setSpacing(10);
        rootLayout->addLayout(swatchesLayout);

        const QList<QColor> swatches = {
            QColor(QStringLiteral("#ffffff")),
            QColor(QStringLiteral("#f7f2d9")),
            QColor(QStringLiteral("#f0d26f")),
            QColor(QStringLiteral("#d05b3f")),
            QColor(QStringLiteral("#2d5ea6")),
            QColor(QStringLiteral("#1f447a")),
            QColor(QStringLiteral("#10233a")),
            QColor(QStringLiteral("#000000"))
        };

        for (const QColor &swatch : swatches) {
            auto *button = new SwatchButton(swatch, this);
            button->setAutoRaise(false);
            button->setMinimumSize(58, 58);
            button->setStyleSheet(QStringLiteral(
                                      "QToolButton {"
                                      " border: 1px solid %1;"
                                      " border-radius: 10px;"
                                      " background: %2;"
                                      " }")
                                      .arg(swatch.darker(135).name(), swatch.name(QColor::HexArgb)));
            button->onChosen = [this](const QColor &color, const bool activate) {
                setColorInternal(color, true);
                if (activate) {
                    emit colorActivated(m_color);
                }
            };
            swatchesLayout->addWidget(button);
        }
        swatchesLayout->addStretch(1);

        auto *slidersLayout = new QGridLayout();
        slidersLayout->setContentsMargins(0, 0, 0, 0);
        slidersLayout->setHorizontalSpacing(12);
        slidersLayout->setVerticalSpacing(10);
        rootLayout->addLayout(slidersLayout);

        addSliderRow(tr("Hue"), &m_hueSlider, &m_hueValueLabel);
        addSliderRow(tr("Saturation"), &m_saturationSlider, &m_saturationValueLabel);
        addSliderRow(tr("Brightness"), &m_valueSlider, &m_valueValueLabel);
        addSliderRow(tr("Red"), &m_redSlider, &m_redValueLabel);
        addSliderRow(tr("Green"), &m_greenSlider, &m_greenValueLabel);
        addSliderRow(tr("Blue"), &m_blueSlider, &m_blueValueLabel);
        addSliderRow(tr("Opacity"), &m_alphaSlider, &m_alphaValueLabel);

        const QList<QSlider *> sliders = {
            m_hueSlider, m_saturationSlider, m_valueSlider,
            m_redSlider, m_greenSlider, m_blueSlider, m_alphaSlider
        };
        for (QSlider *slider : sliders) {
            slider->setStyleSheet(sliderStyleSheet());
        }

        m_hueSlider->setRange(0, 359);
        m_saturationSlider->setRange(0, 255);
        m_valueSlider->setRange(0, 255);
        m_redSlider->setRange(0, 255);
        m_greenSlider->setRange(0, 255);
        m_blueSlider->setRange(0, 255);
        m_alphaSlider->setRange(0, 255);

        connect(m_hueSlider, &QSlider::valueChanged, this, [this]() { syncColorFromHsvSliders(); });
        connect(m_saturationSlider, &QSlider::valueChanged, this, [this]() { syncColorFromHsvSliders(); });
        connect(m_valueSlider, &QSlider::valueChanged, this, [this]() { syncColorFromHsvSliders(); });
        connect(m_redSlider, &QSlider::valueChanged, this, [this]() { syncColorFromRgbSliders(); });
        connect(m_greenSlider, &QSlider::valueChanged, this, [this]() { syncColorFromRgbSliders(); });
        connect(m_blueSlider, &QSlider::valueChanged, this, [this]() { syncColorFromRgbSliders(); });
        connect(m_alphaSlider, &QSlider::valueChanged, this, [this]() { syncColorFromRgbSliders(); });
        connect(m_hexEdit, &QLineEdit::editingFinished, this, [this]() {
            const QString hex = normalizedHexInput(m_hexEdit->text(), m_alphaEnabled);
            const QColor color(hex);
            if (color.isValid()) {
                setColorInternal(color, true);
            } else {
                updatePreview();
            }
        });
        setAlphaEnabled(false);
        setCurrentColor(QColor(QStringLiteral("#ffffff")));
    }

    QColor TouchColorPicker::currentColor() const {
        return m_color;
    }

    void TouchColorPicker::setCurrentColor(const QColor &color) {
        setColorInternal(color, false);
    }

    bool TouchColorPicker::alphaEnabled() const {
        return m_alphaEnabled;
    }

    void TouchColorPicker::setAlphaEnabled(const bool enabled) {
        m_alphaEnabled = enabled;
        if (m_alphaRow) {
            m_alphaRow->setVisible(enabled);
        }
        m_hexEdit->setPlaceholderText(enabled ? QStringLiteral("#AARRGGBB") : QStringLiteral("#RRGGBB"));
        syncControlsFromColor();
    }

    void TouchColorPicker::addSliderRow(const QString &labelText, QSlider **sliderPtr, QLabel **valueLabelPtr) {
        auto *rowWidget = new QWidget(this);
        auto *rowLayout = new QHBoxLayout(rowWidget);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(12);

        auto *label = new QLabel(labelText, this);
        label->setMinimumWidth(94);
        rowLayout->addWidget(label);

        auto *slider = new QSlider(Qt::Horizontal, this);
        slider->setPageStep(8);
        slider->setSingleStep(1);
        slider->setMinimumHeight(58);
        rowLayout->addWidget(slider, 1);

        auto *valueLabel = new QLabel(this);
        valueLabel->setMinimumWidth(44);
        valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        rowLayout->addWidget(valueLabel);

        static_cast<QVBoxLayout *>(layout())->addWidget(rowWidget);

        if (sliderPtr == &m_alphaSlider) {
            m_alphaRow = rowWidget;
        }

        *sliderPtr = slider;
        *valueLabelPtr = valueLabel;
    }

    void TouchColorPicker::syncControlsFromColor() {
        const QSignalBlocker hueBlocker(m_hueSlider);
        const QSignalBlocker saturationBlocker(m_saturationSlider);
        const QSignalBlocker valueBlocker(m_valueSlider);
        const QSignalBlocker redBlocker(m_redSlider);
        const QSignalBlocker greenBlocker(m_greenSlider);
        const QSignalBlocker blueBlocker(m_blueSlider);
        const QSignalBlocker alphaBlocker(m_alphaSlider);

        int hue = 0;
        int saturation = 0;
        int value = 0;
        int alpha = 255;
        m_color.getHsv(&hue, &saturation, &value, &alpha);
        if (hue < 0) {
            hue = 0;
        }

        m_hueSlider->setValue(hue);
        m_saturationSlider->setValue(saturation);
        m_valueSlider->setValue(value);
        m_redSlider->setValue(m_color.red());
        m_greenSlider->setValue(m_color.green());
        m_blueSlider->setValue(m_color.blue());
        m_alphaSlider->setValue(alpha);
        updatePreview();
    }

    void TouchColorPicker::syncColorFromRgbSliders() {
        if (m_updating) {
            return;
        }

        m_updating = true;
        m_color = QColor(
            m_redSlider->value(),
            m_greenSlider->value(),
            m_blueSlider->value(),
            m_alphaEnabled ? m_alphaSlider->value() : 255);
        syncControlsFromColor();
        m_updating = false;
        emit currentColorChanged(m_color);
    }

    void TouchColorPicker::syncColorFromHsvSliders() {
        if (m_updating) {
            return;
        }

        m_updating = true;
        m_color = QColor::fromHsv(
            m_hueSlider->value(),
            m_saturationSlider->value(),
            m_valueSlider->value(),
            m_alphaEnabled ? m_alphaSlider->value() : 255);
        syncControlsFromColor();
        m_updating = false;
        emit currentColorChanged(m_color);
    }

    void TouchColorPicker::updatePreview() {
        m_preview->setStyleSheet(QStringLiteral(
                                     "QFrame {"
                                     " border: 1px solid %1;"
                                     " border-radius: 12px;"
                                     " background: %2;"
                                     " }")
                                     .arg(m_color.darker(150).name(), m_color.name(QColor::HexArgb)));
        m_hexEdit->setText(m_alphaEnabled ? m_color.name(QColor::HexArgb).toUpper() : m_color.name(QColor::HexRgb).toUpper());
        m_rgbLabel->setText(tr("RGB: %1, %2, %3").arg(m_color.red()).arg(m_color.green()).arg(m_color.blue()));

        int hue = 0;
        int saturation = 0;
        int value = 0;
        int alpha = 255;
        m_color.getHsv(&hue, &saturation, &value, &alpha);
        if (hue < 0) {
            hue = 0;
        }

        m_hsvLabel->setText(tr("HSV: %1, %2, %3").arg(hue).arg(saturation).arg(value));
        m_hueValueLabel->setText(QString::number(m_hueSlider->value()));
        m_saturationValueLabel->setText(QString::number(m_saturationSlider->value()));
        m_valueValueLabel->setText(QString::number(m_valueSlider->value()));
        m_redValueLabel->setText(QString::number(m_redSlider->value()));
        m_greenValueLabel->setText(QString::number(m_greenSlider->value()));
        m_blueValueLabel->setText(QString::number(m_blueSlider->value()));
        m_alphaValueLabel->setText(QString::number(m_alphaSlider->value()));
    }

    void TouchColorPicker::setColorInternal(const QColor &color, const bool emitSignal) {
        if (!color.isValid()) {
            return;
        }

        m_color = QColor(color.red(), color.green(), color.blue(), m_alphaEnabled ? color.alpha() : 255);
        m_updating = true;
        syncControlsFromColor();
        m_updating = false;
        if (emitSignal) {
            emit currentColorChanged(m_color);
        }
    }

    TouchColorPickerDialog::TouchColorPickerDialog(QWidget *parent)
        : QDialog(parent, Qt::Popup | Qt::FramelessWindowHint) {
        setAttribute(Qt::WA_DeleteOnClose, false);
        setModal(true);

        auto *rootLayout = new QVBoxLayout(this);
        rootLayout->setContentsMargins(16, 16, 16, 16);
        rootLayout->setSpacing(12);

        m_titleLabel = new QLabel(this);
        rootLayout->addWidget(m_titleLabel);

        m_picker = new TouchColorPicker(this);
        rootLayout->addWidget(m_picker);

        connect(m_picker, &TouchColorPicker::colorActivated, this, [this](const QColor &) {
            accept();
        });
    }

    void TouchColorPickerDialog::setTitleText(const QString &title) {
        if (m_titleLabel) {
            m_titleLabel->setText(title);
        }
    }

    void TouchColorPickerDialog::setCurrentColor(const QColor &color) {
        if (m_picker) {
            m_picker->setCurrentColor(color);
        }
    }

    QColor TouchColorPickerDialog::currentColor() const {
        return m_picker ? m_picker->currentColor() : QColor();
    }

    void TouchColorPickerDialog::setAlphaEnabled(const bool enabled) {
        if (m_picker) {
            m_picker->setAlphaEnabled(enabled);
        }
    }

    void TouchColorPickerDialog::openNear(QWidget *anchor) {
        QWidget *windowWidget = anchor ? anchor->window() : nullptr;
        if (!windowWidget) {
            return;
        }

        const QSize targetSize(qMax(720, windowWidget->width() - 24), qMax(640, windowWidget->height() / 2 + 80));
        resize(targetSize);
        const QPoint topLeft = windowWidget->mapToGlobal(
            QPoint((windowWidget->width() - width()) / 2, qMax(12, windowWidget->height() - height() - 12)));
        move(topLeft);
    }

    QColor TouchColorPickerDialog::getColor(QWidget *parent,
                                            const QString &title,
                                            const QColor &initialColor,
                                            bool *accepted,
                                            const bool alphaEnabled) {
        TouchColorPickerDialog dialog(parent);
        dialog.setTitleText(title);
        dialog.setAlphaEnabled(alphaEnabled);
        dialog.setCurrentColor(initialColor);
        dialog.openNear(parent);

        const bool ok = dialog.exec() == QDialog::Accepted;
        if (accepted) {
            *accepted = ok;
        }
        return ok ? dialog.currentColor() : initialColor;
    }
}
