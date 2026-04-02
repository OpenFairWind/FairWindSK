//
// Created by Codex on 01/04/26.
//

#ifndef FAIRWINDSK_TOUCHCHECKBOX_HPP
#define FAIRWINDSK_TOUCHCHECKBOX_HPP

#include <QString>
#include <QSize>
#include <Qt>
#include <QWidget>

class QEvent;
class QPushButton;

namespace fairwindsk::ui::widgets {
    QT_BEGIN_NAMESPACE
    namespace Ui { class TouchCheckBox; }
    QT_END_NAMESPACE

    class TouchCheckBox : public QWidget {
        Q_OBJECT
        Q_PROPERTY(QString text READ text WRITE setText)
        Q_PROPERTY(bool checked READ isChecked WRITE setChecked NOTIFY toggled)
        Q_PROPERTY(Qt::CheckState checkState READ checkState WRITE setCheckState NOTIFY stateChanged)
        Q_PROPERTY(bool tristate READ isTristate WRITE setTristate)

    public:
        explicit TouchCheckBox(QWidget *parent = nullptr);
        ~TouchCheckBox() override;

        QString text() const;
        bool isChecked() const;
        Qt::CheckState checkState() const;
        bool isTristate() const;
        QSize iconSize() const;
        QPushButton *button() const;

    public slots:
        void setText(const QString &text);
        void setChecked(bool checked);
        void setCheckState(Qt::CheckState state);
        void setTristate(bool tristate);
        void setIconSize(const QSize &size);
        void toggle();
        void click();
        void animateClick();
        void setEnabled(bool enabled);

    signals:
        void clicked(bool checked);
        void pressed();
        void released();
        void toggled(bool checked);
        void stateChanged(int state);

    protected:
        bool eventFilter(QObject *watched, QEvent *event) override;

    private slots:
        void onButtonClicked();
        void onButtonPressed();
        void onButtonReleased();

    private:
        void advanceState();
        void applyState(Qt::CheckState state, bool emitSignals);
        void refreshAppearance();

        Ui::TouchCheckBox *ui = nullptr;
        Qt::CheckState m_checkState = Qt::Unchecked;
        bool m_tristate = false;
    };
}

#endif // FAIRWINDSK_TOUCHCHECKBOX_HPP
