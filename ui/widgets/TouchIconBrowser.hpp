//
// Created by Codex on 15/04/26.
//

#ifndef FAIRWINDSK_UI_WIDGETS_TOUCHICONBROWSER_HPP
#define FAIRWINDSK_UI_WIDGETS_TOUCHICONBROWSER_HPP

#include <QPixmap>
#include <QFrame>
#include <QWidget>

class QLabel;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QEvent;

namespace fairwindsk::ui::widgets {
    class TouchIconBrowser final : public QWidget {
        Q_OBJECT

    public:
        explicit TouchIconBrowser(QWidget *parent = nullptr);

        void setCurrentPath(const QString &path);
        QString currentPath() const;
        QString selectedPath() const;

        static QString normalizedIconStoragePath(const QString &path);
        static QString resolvedLocalIconPath(const QString &path);
        static QPixmap iconPixmapForPath(const QString &path, int iconSize);

    signals:
        void canceled();
        void pathSelected(const QString &path);
        void pathActivated(const QString &path);

    protected:
        void changeEvent(QEvent *event) override;

    private:
        void applyComfortChrome();
        void populate();
        void ensureIconEntry(const QString &path);
        void updatePreview(const QString &path);

    private:
        QString m_currentPath;
        QLabel *m_previewLabel = nullptr;
        QLabel *m_selectionLabel = nullptr;
        QListWidget *m_listWidget = nullptr;
        QPushButton *m_cancelButton = nullptr;
        QPushButton *m_applyButton = nullptr;
        QFrame *m_previewFrame = nullptr;
        bool m_isApplyingComfortChrome = false;
    };
}

#endif // FAIRWINDSK_UI_WIDGETS_TOUCHICONBROWSER_HPP
