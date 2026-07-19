#ifndef FAIRWINDSK_ANDROIDAPPS_HPP
#define FAIRWINDSK_ANDROIDAPPS_HPP

#include <QWidget>
#include <nlohmann/json.hpp>

class QLabel;
class QListWidget;
class QListWidgetItem;
class QPushButton;

namespace fairwindsk::ui::settings {

    class Settings;

    class AndroidApps final : public QWidget {
        Q_OBJECT

    public:
        explicit AndroidApps(Settings *settings, QWidget *parent = nullptr);
        void refreshApplications();

    private:
        void rebuildList();
        void setApplicationSelected(const nlohmann::json &application, bool selected);
        void removeApplicationFromLauncherPages(const QString &appName);
        static void removeApplicationFromNodes(nlohmann::json &nodes, const QString &appName);

        Settings *m_settings = nullptr;
        QLabel *m_statusLabel = nullptr;
        QListWidget *m_applicationsList = nullptr;
        QPushButton *m_refreshButton = nullptr;
        nlohmann::json m_discoveredApplications = nlohmann::json::array();
        bool m_rebuilding = false;
    };

} // fairwindsk::ui::settings

#endif // FAIRWINDSK_ANDROIDAPPS_HPP
