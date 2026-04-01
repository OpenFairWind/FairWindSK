//
// Created by Codex on 01/04/26.
//

#ifndef FAIRWINDSK_TOUCHSPINBOX_HPP
#define FAIRWINDSK_TOUCHSPINBOX_HPP

#include <QWidget>

class QDoubleValidator;
class QLineEdit;

namespace fairwindsk::ui::widgets {
    QT_BEGIN_NAMESPACE
    namespace Ui { class TouchSpinBox; }
    QT_END_NAMESPACE

    class TouchSpinBox : public QWidget {
    Q_OBJECT
    Q_PROPERTY(double minimum READ minimum WRITE setMinimum)
    Q_PROPERTY(double maximum READ maximum WRITE setMaximum)
    Q_PROPERTY(double value READ value WRITE setValue)
    Q_PROPERTY(double singleStep READ singleStep WRITE setSingleStep)
    Q_PROPERTY(int decimals READ decimals WRITE setDecimals)
    Q_PROPERTY(Qt::Alignment alignment READ alignment WRITE setAlignment)

    public:
        explicit TouchSpinBox(QWidget *parent = nullptr);
        ~TouchSpinBox() override;

        double minimum() const;
        double maximum() const;
        double value() const;
        double singleStep() const;
        int decimals() const;
        Qt::Alignment alignment() const;

    public slots:
        void setMinimum(double minimum);
        void setMaximum(double maximum);
        void setRange(double minimum, double maximum);
        void setValue(double value);
        void setValue(int value);
        void setSingleStep(double singleStep);
        void setDecimals(int decimals);
        void setAlignment(Qt::Alignment alignment);
        void stepUp();
        void stepDown();

    signals:
        void valueChanged(double value);
        void valueChanged(int value);
        void editingFinished();

    private slots:
        void applyEditedValue();

    private:
        void refreshText();
        void refreshButtonState();
        void refreshValidator();
        double normalizedValue(double value) const;
        double parsedEditorValue(bool *ok = nullptr) const;

        Ui::TouchSpinBox *ui = nullptr;
        QLineEdit *m_editor = nullptr;
        QDoubleValidator *m_validator = nullptr;
        double m_minimum = 0.0;
        double m_maximum = 99.0;
        double m_value = 0.0;
        double m_singleStep = 1.0;
        int m_decimals = 0;
        Qt::Alignment m_alignment = Qt::AlignRight | Qt::AlignVCenter;
    };
}

#endif // FAIRWINDSK_TOUCHSPINBOX_HPP
