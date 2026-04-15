//
// Created by Codex on 28/03/26.
//

#ifndef FAIRWINDSK_APPDETAILSWIDGET_HPP
#define FAIRWINDSK_APPDETAILSWIDGET_HPP

#include <QEvent>
#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class AppDetailsWidget; }
QT_END_NAMESPACE

namespace fairwindsk::ui::widgets {
    class TouchIconBrowser;
}

namespace fairwindsk::ui::settings {
    class AppDetailsWidget final : public QWidget {
        Q_OBJECT

    public:
        explicit AppDetailsWidget(QWidget *parent = nullptr);
        ~AppDetailsWidget() override;

        void setAppIconPath(const QString &path);
        QString appIconPath() const;
        void showIconPicker();
        void hideIconPicker();

    signals:
        void iconPathSelected(const QString &path);

    private:
        bool eventFilter(QObject *watched, QEvent *event) override;
        void updateIconPreview(const QString &path);

    private:
        QString m_currentIconPath;
        fairwindsk::ui::widgets::TouchIconBrowser *m_iconBrowser = nullptr;

    public:
        ::Ui::AppDetailsWidget *ui = nullptr;
    };
}

#endif // FAIRWINDSK_APPDETAILSWIDGET_HPP
