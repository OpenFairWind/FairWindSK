//
// Created by Codex on 01/04/26.
//

#ifndef FAIRWINDSK_TOUCHCOMBOBOX_HPP
#define FAIRWINDSK_TOUCHCOMBOBOX_HPP

#include <QVariant>
#include <QWidget>

class QFrame;
class QIcon;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QAction;

namespace fairwindsk::ui::widgets {
    QT_BEGIN_NAMESPACE
    namespace Ui { class TouchComboBox; }
    QT_END_NAMESPACE

    class TouchComboBox : public QWidget {
    Q_OBJECT
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex)
    Q_PROPERTY(QString currentText READ currentText WRITE setCurrentText)
    Q_PROPERTY(bool editable READ isEditable WRITE setEditable)

    public:
        explicit TouchComboBox(QWidget *parent = nullptr);
        ~TouchComboBox() override;

        void addItem(const QString &text, const QVariant &userData = QVariant());
        void addItem(const QIcon &icon, const QString &text, const QVariant &userData = QVariant());
        void clear();
        int count() const;
        int currentIndex() const;
        QString currentText() const;
        QVariant currentData(int role = Qt::UserRole) const;
        QVariant itemData(int index, int role = Qt::UserRole) const;
        int findData(const QVariant &data, int role = Qt::UserRole) const;
        int findText(const QString &text, Qt::MatchFlags flags = Qt::MatchExactly | Qt::MatchCaseSensitive) const;
        bool isEditable() const;

    public slots:
        void setCurrentIndex(int index);
        void setCurrentText(const QString &text);
        void setEditable(bool editable);
        void setEnabled(bool enabled);
        void removeItem(int index);

    signals:
        void currentIndexChanged(int index);
        void currentTextChanged(const QString &text);
        void activated(int index);
        void editTextChanged(const QString &text);

    protected:
        bool event(QEvent *event) override;
        bool eventFilter(QObject *watched, QEvent *event) override;
        void resizeEvent(QResizeEvent *event) override;

    private slots:
        void togglePopup();
        void handleItemClicked(QListWidgetItem *item);

    private:
        void applyTouchStyle();
        void updateDisplay();
        void positionPopup();
        void ensureCurrentItemVisible() const;
        int popupHeightForItems() const;

        Ui::TouchComboBox *ui = nullptr;
        QLineEdit *m_editor = nullptr;
        QFrame *m_popup = nullptr;
        QListWidget *m_listWidget = nullptr;
        QAction *m_iconAction = nullptr;
        int m_currentIndex = -1;
        bool m_editable = false;
        QString m_buttonStyleSheet;
        QString m_popupStyleSheet;
    };
}

#endif // FAIRWINDSK_TOUCHCOMBOBOX_HPP
