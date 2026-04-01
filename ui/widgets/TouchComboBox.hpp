//
// Created by Codex on 01/04/26.
//

#ifndef FAIRWINDSK_TOUCHCOMBOBOX_HPP
#define FAIRWINDSK_TOUCHCOMBOBOX_HPP

#include <QVariant>
#include <QWidget>

class QFrame;
class QHBoxLayout;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;

namespace fairwindsk::ui::widgets {
    class TouchComboBox : public QWidget {
    Q_OBJECT
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex)
    Q_PROPERTY(QString currentText READ currentText)

    public:
        explicit TouchComboBox(QWidget *parent = nullptr);
        ~TouchComboBox() override;

        void addItem(const QString &text, const QVariant &userData = QVariant());
        void clear();
        int count() const;
        int currentIndex() const;
        QString currentText() const;
        QVariant currentData(int role = Qt::UserRole) const;
        int findData(const QVariant &data, int role = Qt::UserRole) const;
        int findText(const QString &text, Qt::MatchFlags flags = Qt::MatchExactly | Qt::MatchCaseSensitive) const;

    public slots:
        void setCurrentIndex(int index);
        void setEnabled(bool enabled);

    signals:
        void currentIndexChanged(int index);
        void currentTextChanged(const QString &text);
        void activated(int index);

    protected:
        bool eventFilter(QObject *watched, QEvent *event) override;
        void resizeEvent(QResizeEvent *event) override;

    private slots:
        void togglePopup();
        void handleItemClicked(QListWidgetItem *item);

    private:
        void updateDisplay();
        void positionPopup();

        QHBoxLayout *m_layout = nullptr;
        QLineEdit *m_editor = nullptr;
        QPushButton *m_button = nullptr;
        QFrame *m_popup = nullptr;
        QListWidget *m_listWidget = nullptr;
        int m_currentIndex = -1;
    };
}

#endif // FAIRWINDSK_TOUCHCOMBOBOX_HPP
