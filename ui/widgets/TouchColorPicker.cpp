//
// Created by Codex on 05/04/26.
//

#include "TouchColorPicker.hpp"

#include <algorithm>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSettings>
#include <QSignalBlocker>
#include <QSlider>
#include <QToolButton>
#include <QVBoxLayout>
#include <functional>
#include <memory>

#include "Configuration.hpp"
#include "ui/DrawerDialogHost.hpp"

namespace fairwindsk::ui::widgets {
    class TouchColorShadeSelector final : public QWidget {
    public:
        explicit TouchColorShadeSelector(QWidget *parent = nullptr)
            : QWidget(parent) {
            setMinimumSize(220, 180);
            setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            setAttribute(Qt::WA_OpaquePaintEvent, true);
        }

        void setHue(const int hue) {
            const int normalizedHue = std::clamp(hue, 0, 359);
            if (m_hue == normalizedHue) {
                return;
            }
            m_hue = normalizedHue;
            m_cache = QImage();
            update();
        }

        void setSelection(const int saturation, const int value) {
            m_saturation = std::clamp(saturation, 0, 255);
            m_value = std::clamp(value, 0, 255);
            update();
        }

        std::function<void(int saturation, int value)> onSelectionChanged;

    protected:
        void paintEvent(QPaintEvent *event) override {
            Q_UNUSED(event);
            ensureCache();

            QPainter painter(this);
            painter.setRenderHint(QPainter::Antialiasing, true);
            painter.drawImage(rect(), m_cache);

            const QRectF usableRect = rect().adjusted(1, 1, -1, -1);
            const qreal x = usableRect.left() + usableRect.width() * (qreal(m_saturation) / 255.0);
            const qreal y = usableRect.top() + usableRect.height() * (1.0 - (qreal(m_value) / 255.0));

            painter.setPen(QPen(Qt::black, 3));
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(QPointF(x, y), 10.0, 10.0);
            painter.setPen(QPen(Qt::white, 2));
            painter.drawEllipse(QPointF(x, y), 10.0, 10.0);
        }

        void resizeEvent(QResizeEvent *event) override {
            QWidget::resizeEvent(event);
            m_cache = QImage();
        }

        void mousePressEvent(QMouseEvent *event) override {
            if (event->button() == Qt::LeftButton) {
                updateSelectionFromPosition(event->position().toPoint());
                event->accept();
                return;
            }
            QWidget::mousePressEvent(event);
        }

        void mouseMoveEvent(QMouseEvent *event) override {
            if (event->buttons().testFlag(Qt::LeftButton)) {
                updateSelectionFromPosition(event->position().toPoint());
                event->accept();
                return;
            }
            QWidget::mouseMoveEvent(event);
        }

    private:
        void ensureCache() {
            if (!m_cache.isNull() && m_cache.size() == size()) {
                return;
            }

            const QSize targetSize = size().boundedTo(QSize(640, 640));
            m_cache = QImage(targetSize, QImage::Format_ARGB32_Premultiplied);
            for (int y = 0; y < targetSize.height(); ++y) {
                const int value = 255 - ((255 * y) / std::max(1, targetSize.height() - 1));
                QRgb *line = reinterpret_cast<QRgb *>(m_cache.scanLine(y));
                for (int x = 0; x < targetSize.width(); ++x) {
                    const int saturation = (255 * x) / std::max(1, targetSize.width() - 1);
                    line[x] = QColor::fromHsv(m_hue, saturation, value).rgba();
                }
            }
        }

        void updateSelectionFromPosition(const QPoint &position) {
            const QRect usableRect = rect().adjusted(1, 1, -1, -1);
            const int clampedX = std::clamp(position.x(), usableRect.left(), usableRect.right());
            const int clampedY = std::clamp(position.y(), usableRect.top(), usableRect.bottom());

            const int saturation = int((qreal(clampedX - usableRect.left()) / std::max(1, usableRect.width())) * 255.0);
            const int value = 255 - int((qreal(clampedY - usableRect.top()) / std::max(1, usableRect.height())) * 255.0);
            setSelection(saturation, value);
            if (onSelectionChanged) {
                onSelectionChanged(m_saturation, m_value);
            }
        }

        int m_hue = 0;
        int m_saturation = 255;
        int m_value = 255;
        QImage m_cache;
    };

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
                setCheckable(true);
                setMinimumSize(64, 64);
                setIconSize(QSize(28, 28));
                setAutoRaise(false);
            }

            QColor color() const {
                return m_color;
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
                " height: 18px;"
                " border-radius: 9px;"
                " background: rgba(255, 255, 255, 0.25);"
                " }"
                "QSlider::handle:horizontal {"
                " width: 36px;"
                " margin: -14px 0;"
                " border-radius: 18px;"
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

        QString customColorsKey() {
            return QStringLiteral("touchColorPicker/customColors");
        }

        QColor normalizedStoredColor(const QColor &color) {
            return QColor(color.red(), color.green(), color.blue(), color.alpha());
        }

        bool sameStoredColor(const QColor &left, const QColor &right) {
            return normalizedStoredColor(left).rgba() == normalizedStoredColor(right).rgba();
        }

        constexpr int kSwatchColumns = 4;
    }

    TouchColorPicker::TouchColorPicker(QWidget *parent)
        : QWidget(parent) {
        auto *rootLayout = new QVBoxLayout(this);
        rootLayout->setContentsMargins(0, 0, 0, 0);
        rootLayout->setSpacing(12);

        auto *hintLabel = new QLabel(
            tr("Tap or drag in the shade area, use the sliders, type a hex value, or manage custom swatches for quick touch selection."),
            this);
        hintLabel->setWordWrap(true);
        rootLayout->addWidget(hintLabel);

        auto *headerLayout = new QHBoxLayout();
        headerLayout->setContentsMargins(0, 0, 0, 0);
        headerLayout->setSpacing(12);
        rootLayout->addLayout(headerLayout);

        auto *preview = new PreviewFrame(this);
        preview->setMinimumSize(132, 108);
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

        auto *shadeLabel = new QLabel(tr("Shade"), this);
        rootLayout->addWidget(shadeLabel);

        m_shadeSelector = new TouchColorShadeSelector(this);
        rootLayout->addWidget(m_shadeSelector);

        auto *quickColorsLabel = new QLabel(tr("Quick Colors"), this);
        rootLayout->addWidget(quickColorsLabel);

        auto *quickSwatchesHost = new QWidget(this);
        m_quickSwatchesLayout = new QGridLayout(quickSwatchesHost);
        m_quickSwatchesLayout->setContentsMargins(0, 0, 0, 0);
        m_quickSwatchesLayout->setHorizontalSpacing(10);
        m_quickSwatchesLayout->setVerticalSpacing(10);
        rootLayout->addWidget(quickSwatchesHost);

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

        for (int i = 0; i < swatches.size(); ++i) {
            const QColor &swatch = swatches.at(i);
            auto *button = new SwatchButton(swatch, this);
            button->setStyleSheet(QStringLiteral(
                                      "QToolButton {"
                                      " border: 2px solid %1;"
                                      " border-radius: 12px;"
                                      " background: %2;"
                                      " }"
                                      "QToolButton:checked {"
                                      " border-width: 4px;"
                                      " border-color: #ffffff;"
                                      " }")
                                      .arg(swatch.darker(135).name(), swatch.name(QColor::HexArgb)));
            button->onChosen = [this](const QColor &color, const bool activate) {
                setColorInternal(color, true);
                if (activate) {
                    emit colorActivated(m_color);
                }
            };
            m_quickSwatchesLayout->addWidget(button, i / kSwatchColumns, i % kSwatchColumns);
        }

        auto *customHeaderLayout = new QHBoxLayout();
        customHeaderLayout->setContentsMargins(0, 0, 0, 0);
        customHeaderLayout->setSpacing(8);
        rootLayout->addLayout(customHeaderLayout);

        auto *customColorsLabel = new QLabel(tr("Custom Colors"), this);
        customHeaderLayout->addWidget(customColorsLabel);

        customHeaderLayout->addStretch(1);

        m_addCustomColorButton = new QPushButton(tr("Add current"), this);
        m_addCustomColorButton->setMinimumHeight(42);
        customHeaderLayout->addWidget(m_addCustomColorButton);

        m_removeCustomColorButton = new QPushButton(tr("Remove current"), this);
        m_removeCustomColorButton->setMinimumHeight(42);
        customHeaderLayout->addWidget(m_removeCustomColorButton);

        m_customSwatchesHost = new QWidget(this);
        m_customSwatchesLayout = new QGridLayout(m_customSwatchesHost);
        m_customSwatchesLayout->setContentsMargins(0, 0, 0, 0);
        m_customSwatchesLayout->setHorizontalSpacing(10);
        m_customSwatchesLayout->setVerticalSpacing(10);
        rootLayout->addWidget(m_customSwatchesHost);

        auto *hsvLabel = new QLabel(tr("Color Balance"), this);
        rootLayout->addWidget(hsvLabel);

        addSliderRow(tr("Hue"), &m_hueSlider, &m_hueValueLabel);
        addSliderRow(tr("Saturation"), &m_saturationSlider, &m_saturationValueLabel);
        addSliderRow(tr("Brightness"), &m_valueSlider, &m_valueValueLabel);

        auto *rgbLabel = new QLabel(tr("RGB Fine Tuning"), this);
        rootLayout->addWidget(rgbLabel);

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
        m_shadeSelector->onSelectionChanged = [this](const int saturation, const int value) {
            syncColorFromShade(saturation, value);
        };
        connect(m_addCustomColorButton, &QPushButton::clicked, this, [this]() {
            const QColor storedColor = normalizedStoredColor(m_color);
            for (const QColor &color : m_customColors) {
                if (sameStoredColor(color, storedColor)) {
                    return;
                }
            }
            m_customColors.prepend(storedColor);
            while (m_customColors.size() > 14) {
                m_customColors.removeLast();
            }
            saveCustomColors();
            rebuildCustomSwatches();
        });
        connect(m_removeCustomColorButton, &QPushButton::clicked, this, [this]() {
            for (int i = 0; i < m_customColors.size(); ++i) {
                if (sameStoredColor(m_customColors.at(i), m_color)) {
                    m_customColors.removeAt(i);
                    saveCustomColors();
                    rebuildCustomSwatches();
                    return;
                }
            }
        });

        loadCustomColors();
        rebuildCustomSwatches();
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

    void TouchColorPicker::loadCustomColors() {
        QSettings settings(Configuration::settingsFilename(), QSettings::IniFormat);
        const QStringList values = settings.value(customColorsKey()).toStringList();
        m_customColors.clear();
        for (const QString &value : values) {
            const QColor color(value);
            if (color.isValid()) {
                m_customColors.append(color);
            }
        }
    }

    void TouchColorPicker::saveCustomColors() const {
        QStringList values;
        values.reserve(m_customColors.size());
        for (const QColor &color : m_customColors) {
            values.append(color.name(QColor::HexArgb));
        }

        QSettings settings(Configuration::settingsFilename(), QSettings::IniFormat);
        settings.setValue(customColorsKey(), values);
        settings.sync();
    }

    void TouchColorPicker::rebuildCustomSwatches() {
        if (!m_customSwatchesLayout) {
            return;
        }

        while (QLayoutItem *item = m_customSwatchesLayout->takeAt(0)) {
            if (item->widget()) {
                item->widget()->deleteLater();
            }
            delete item;
        }

        for (int i = 0; i < m_customColors.size(); ++i) {
            const QColor &swatch = m_customColors.at(i);
            auto *button = new SwatchButton(swatch, m_customSwatchesHost);
            button->setChecked(sameStoredColor(swatch, m_color));
            button->setStyleSheet(QStringLiteral(
                                      "QToolButton {"
                                      " border: 2px solid %1;"
                                      " border-radius: 12px;"
                                      " background: %2;"
                                      " }"
                                      "QToolButton:checked {"
                                      " border-width: 4px;"
                                      " border-color: #ffffff;"
                                      " }")
                                      .arg(swatch.darker(135).name(), swatch.name(QColor::HexArgb)));
            button->onChosen = [this](const QColor &color, const bool activate) {
                setColorInternal(color, true);
                if (activate) {
                    emit colorActivated(m_color);
                }
            };
            m_customSwatchesLayout->addWidget(button, i / kSwatchColumns, i % kSwatchColumns);
        }
        updatePreview();
    }

    void TouchColorPicker::updateShadeSelector() {
        int hue = 0;
        int saturation = 0;
        int value = 0;
        int alpha = 255;
        m_color.getHsv(&hue, &saturation, &value, &alpha);
        if (hue < 0) {
            hue = m_hueSlider ? m_hueSlider->value() : 0;
        }

        if (m_shadeSelector) {
            m_shadeSelector->setHue(hue);
            m_shadeSelector->setSelection(saturation, value);
        }
    }

    void TouchColorPicker::addSliderRow(const QString &labelText, QSlider **sliderPtr, QLabel **valueLabelPtr) {
        auto *rowWidget = new QWidget(this);
        auto *rowLayout = new QHBoxLayout(rowWidget);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(10);

        auto *label = new QLabel(labelText, this);
        label->setMinimumWidth(82);
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
        updateShadeSelector();
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

    void TouchColorPicker::syncColorFromShade(const int saturation, const int value) {
        if (m_updating) {
            return;
        }

        m_updating = true;
        m_color = QColor::fromHsv(
            m_hueSlider->value(),
            saturation,
            value,
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

        bool isCustomColor = false;
        for (const QColor &color : m_customColors) {
            if (sameStoredColor(color, m_color)) {
                isCustomColor = true;
                break;
            }
        }
        if (m_removeCustomColorButton) {
            m_removeCustomColorButton->setEnabled(isCustomColor);
        }
    }

    void TouchColorPicker::setColorInternal(const QColor &color, const bool emitSignal) {
        if (!color.isValid()) {
            return;
        }

        m_color = QColor(color.red(), color.green(), color.blue(), m_alphaEnabled ? color.alpha() : 255);
        m_updating = true;
        syncControlsFromColor();
        m_updating = false;
        rebuildCustomSwatches();
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

        auto *headerLayout = new QHBoxLayout();
        headerLayout->setContentsMargins(0, 0, 0, 0);
        headerLayout->setSpacing(8);
        rootLayout->addLayout(headerLayout);

        m_titleLabel = new QLabel(this);
        headerLayout->addWidget(m_titleLabel, 1);

        m_closeButton = new QPushButton(this);
        m_closeButton->setIcon(QIcon(QStringLiteral(":/resources/svg/OpenBridge/close-google.svg")));
        m_closeButton->setToolTip(tr("Use current color"));
        m_closeButton->setMinimumSize(46, 46);
        m_closeButton->setIconSize(QSize(28, 28));
        m_closeButton->setStyleSheet(QStringLiteral(
            "QPushButton {"
            " border: 1px solid rgba(255, 255, 255, 0.20);"
            " border-radius: 10px;"
            " background: transparent;"
            " padding: 4px;"
            " }"
            "QPushButton:hover, QPushButton:pressed {"
            " background: rgba(255, 255, 255, 0.16);"
            " border-color: rgba(255, 255, 255, 0.32);"
            " }"));
        headerLayout->addWidget(m_closeButton, 0, Qt::AlignTop);

        m_picker = new TouchColorPicker(this);
        rootLayout->addWidget(m_picker, 1);

        connect(m_picker, &TouchColorPicker::colorActivated, this, [this](const QColor &) {
            accept();
        });
        connect(m_closeButton, &QPushButton::clicked, this, [this]() {
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

        const int horizontalMargin = 20;
        const int verticalMargin = 12;
        const int availableWidth = qMax(360, windowWidget->width() - (horizontalMargin * 2));
        const int availableHeight = qMax(320, windowWidget->height() - (verticalMargin * 3));
        const QSize targetSize(qMin(760, availableWidth), qMin(availableHeight, qMax(420, (windowWidget->height() * 2) / 3)));
        resize(targetSize);
        const QPoint topLeft = windowWidget->mapToGlobal(
            QPoint((windowWidget->width() - width()) / 2, qMax(verticalMargin, windowWidget->height() - height() - verticalMargin)));
        move(topLeft);
    }

    QColor TouchColorPickerDialog::getColor(QWidget *parent,
                                            const QString &title,
                                            const QColor &initialColor,
                                            bool *accepted,
                                            const bool alphaEnabled) {
        auto *picker = new TouchColorPicker();
        picker->setProperty("drawerFillCenterArea", true);
        picker->setAlphaEnabled(alphaEnabled);
        picker->setCurrentColor(initialColor);
        auto selectedColor = std::make_shared<QColor>(picker->currentColor());
        connect(picker, &TouchColorPicker::currentColorChanged, picker, [selectedColor](const QColor &color) {
            *selectedColor = color;
        });

        constexpr int kResultCancel = 0;
        constexpr int kResultApply = 1;
        const int result = fairwindsk::ui::drawer::execDrawer(
            parent,
            title,
            picker,
            {
                {QObject::tr("Cancel"), kResultCancel, false},
                {QObject::tr("Apply"), kResultApply, true}
            },
            kResultCancel);

        const bool ok = result == kResultApply;
        if (accepted) {
            *accepted = ok;
        }
        return ok ? *selectedColor : initialColor;
    }
}
