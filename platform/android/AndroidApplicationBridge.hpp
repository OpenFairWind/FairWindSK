#ifndef FAIRWINDSK_ANDROIDAPPLICATIONBRIDGE_HPP
#define FAIRWINDSK_ANDROIDAPPLICATIONBRIDGE_HPP

#include <QString>
#include <nlohmann/json.hpp>

namespace fairwindsk::platform::android {

    class AndroidApplicationBridge final {
    public:
        static nlohmann::json launchableApplications();
        static nlohmann::json recentApplications();
        static bool launchApplication(const QString &packageName, const QString &activityName);
        static bool isDefaultLauncher();
        static void requestLauncherMode(bool launcherMode);
        static bool hasHardwareNavigationKey(int keyCode);
        static void requestSystemBack();
    };

} // fairwindsk::platform::android

#endif // FAIRWINDSK_ANDROIDAPPLICATIONBRIDGE_HPP
