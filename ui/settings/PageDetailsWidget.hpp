//
// Created by Codex on 28/03/26.
//

#ifndef FAIRWINDSK_PAGEDETAILSWIDGET_HPP
#define FAIRWINDSK_PAGEDETAILSWIDGET_HPP

#include <QEvent>
#include <QPixmap>
#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class PageDetailsWidget; }
QT_END_NAMESPACE

namespace fairwindsk::ui::settings {
    class PageDetailsWidget final : public QWidget {
        Q_OBJECT

    public:
        explicit PageDetailsWidget(QWidget *parent = nullptr);
        ~PageDetailsWidget() override;

        void setPageName(const QString &name) const;
        QString pageName() const;
        void setPageIconPath(const QString &path);
        QString pageIconPath() const;
        void showIconPicker();
        void hideIconPicker();
        static QString normalizedIconStoragePath(const QString &path);
        static QString resolvedLocalIconPath(const QString &path);
        static QPixmap iconPixmapForPath(const QString &path, int iconSize);

    signals:
        void iconPathSelected(const QString &path);

    private:
        bool eventFilter(QObject *watched, QEvent *event) override;
        void applySelectedIcon();
        void populateIconPicker();
        void ensureIconEntry(const QString &path);
        void updateIconPreview(const QString &path);

    private:
        QString m_currentIconPath;

    public:
        ::Ui::PageDetailsWidget *ui = nullptr;
    };
}

#endif // FAIRWINDSK_PAGEDETAILSWIDGET_HPP
