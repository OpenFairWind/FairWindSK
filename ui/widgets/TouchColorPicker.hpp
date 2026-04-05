//
// Created by Codex on 05/04/26.
//

#ifndef FAIRWINDSK_UI_WIDGETS_TOUCHCOLORPICKER_HPP
#define FAIRWINDSK_UI_WIDGETS_TOUCHCOLORPICKER_HPP

#include <QColor>
#include <QDialog>
#include <QWidget>

class QFrame;
class QLineEdit;
class QLabel;
class QSlider;
class QWidget;

namespace fairwindsk::ui::widgets {
    class TouchColorPicker : public QWidget {
        Q_OBJECT

    public:
        explicit TouchColorPicker(QWidget *parent = nullptr);

        QColor currentColor() const;
        void setCurrentColor(const QColor &color);

        bool alphaEnabled() const;
        void setAlphaEnabled(bool enabled);

    signals:
        void currentColorChanged(const QColor &color);
        void colorActivated(const QColor &color);

    private:
        void addSliderRow(const QString &labelText, QSlider **sliderPtr, QLabel **valueLabelPtr);
        void syncControlsFromColor();
        void syncColorFromRgbSliders();
        void syncColorFromHsvSliders();
        void updatePreview();
        void setColorInternal(const QColor &color, bool emitSignal);

        QColor m_color = QColor(QStringLiteral("#ffffff"));
        bool m_alphaEnabled = false;
        bool m_updating = false;

        QFrame *m_preview = nullptr;
        QLineEdit *m_hexEdit = nullptr;
        QLabel *m_rgbLabel = nullptr;
        QLabel *m_hsvLabel = nullptr;

        QSlider *m_hueSlider = nullptr;
        QSlider *m_saturationSlider = nullptr;
        QSlider *m_valueSlider = nullptr;
        QSlider *m_redSlider = nullptr;
        QSlider *m_greenSlider = nullptr;
        QSlider *m_blueSlider = nullptr;
        QSlider *m_alphaSlider = nullptr;
        QWidget *m_alphaRow = nullptr;

        QLabel *m_hueValueLabel = nullptr;
        QLabel *m_saturationValueLabel = nullptr;
        QLabel *m_valueValueLabel = nullptr;
        QLabel *m_redValueLabel = nullptr;
        QLabel *m_greenValueLabel = nullptr;
        QLabel *m_blueValueLabel = nullptr;
        QLabel *m_alphaValueLabel = nullptr;
    };

    class TouchColorPickerDialog : public QDialog {
        Q_OBJECT

    public:
        explicit TouchColorPickerDialog(QWidget *parent = nullptr);

        void setTitleText(const QString &title);
        void setCurrentColor(const QColor &color);
        QColor currentColor() const;
        void setAlphaEnabled(bool enabled);
        void openNear(QWidget *anchor);

        static QColor getColor(QWidget *parent,
                               const QString &title,
                               const QColor &initialColor,
                               bool *accepted = nullptr,
                               bool alphaEnabled = false);

    private:
        QLabel *m_titleLabel = nullptr;
        TouchColorPicker *m_picker = nullptr;
    };
}

#endif // FAIRWINDSK_UI_WIDGETS_TOUCHCOLORPICKER_HPP
