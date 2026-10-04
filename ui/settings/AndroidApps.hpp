#ifndef FAIRWINDSK_ANDROIDAPPS_HPP
#define FAIRWINDSK_ANDROIDAPPS_HPP

#include <QWidget>
#include <nlohmann/json.hpp>

class QLabel;
class QListWidget;
class QListWidgetItem;
class QEvent;
class QPushButton;
class QShowEvent;

template<typename T>
class QFutureWatcher;

namespace fairwindsk::ui::widgets {
    class TouchCheckBox;
}

namespace fairwindsk::ui::settings {

    class Settings;

    class AndroidApps final : public QWidget {
        Q_OBJECT

    public:
        explicit AndroidApps(Settings *settings, QWidget *parent = nullptr);
        void refreshApplications();

    protected:
        bool eventFilter(QObject *watched, QEvent *event) override;
        void showEvent(QShowEvent *event) override;

    private:
        void rebuildList();
        void refreshLauncherMode();
        void setApplicationSelected(const nlohmann::json &application, bool selected);
        void removeApplicationFromLauncherPages(const QString &appName);
        static void removeApplicationFromNodes(nlohmann::json &nodes, const QString &appName);

        Settings *m_settings = nullptr;
        QLabel *m_statusLabel = nullptr;
        QListWidget *m_applicationsList = nullptr;
        QPushButton *m_refreshButton = nullptr;
        widgets::TouchCheckBox *m_launcherModeCheckBox = nullptr;
        QFutureWatcher<nlohmann::json> *m_discoveryWatcher = nullptr;
        nlohmann::json m_discoveredApplications = nlohmann::json::array();
        bool m_rebuilding = false;
    };

} // fairwindsk::ui::settings

#endif // FAIRWINDSK_ANDROIDAPPS_HPP
