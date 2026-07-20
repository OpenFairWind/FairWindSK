#include "AndroidApplicationBridge.hpp"

#if defined(Q_OS_ANDROID)
#include <QJniObject>
#endif

namespace fairwindsk::platform::android {
#if defined(Q_OS_ANDROID)
    namespace {
        nlohmann::json parseJsonArray(const QJniObject &payload) {
            if (!payload.isValid()) {
                return nlohmann::json::array();
            }

            const QByteArray utf8 = payload.toString().toUtf8();
            try {
                const auto parsed = nlohmann::json::parse(utf8.constData(), utf8.constData() + utf8.size());
                return parsed.is_array() ? parsed : nlohmann::json::array();
            } catch (const std::exception &) {
                return nlohmann::json::array();
            }
        }
    }
#endif

    nlohmann::json AndroidApplicationBridge::launchableApplications() {
#if defined(Q_OS_ANDROID)
        // Ask the Android activity for a JSON snapshot so the Qt UI stays platform-neutral.
        const QJniObject payload = QJniObject::callStaticObjectMethod(
            "org/openfairwind/fairwindsk/FairWindSKActivity",
            "launchableApplicationsJson",
            "()Ljava/lang/String;");
        // Parse defensively because package metadata is supplied by third-party applications.
        return parseJsonArray(payload);
#else
        // Keep the shared settings source linkable while the Android tab remains absent.
        return nlohmann::json::array();
#endif
    }

    nlohmann::json AndroidApplicationBridge::recentApplications() {
#if defined(Q_OS_ANDROID)
        return parseJsonArray(QJniObject::callStaticObjectMethod(
            "org/openfairwind/fairwindsk/FairWindSKActivity",
            "recentApplicationsJson",
            "()Ljava/lang/String;"));
#else
        return nlohmann::json::array();
#endif
    }

    bool AndroidApplicationBridge::launchApplication(const QString &packageName, const QString &activityName) {
#if defined(Q_OS_ANDROID)
        // Use an explicit component so the exact activity selected in Settings is launched.
        return QJniObject::callStaticMethod<jboolean>(
            "org/openfairwind/fairwindsk/FairWindSKActivity",
            "launchAndroidApplication",
            "(Ljava/lang/String;Ljava/lang/String;)Z",
            QJniObject::fromString(packageName).object<jstring>(),
            QJniObject::fromString(activityName).object<jstring>());
#else
        Q_UNUSED(packageName);
        Q_UNUSED(activityName);
        return false;
#endif
    }

    bool AndroidApplicationBridge::isDefaultLauncher() {
#if defined(Q_OS_ANDROID)
        return QJniObject::callStaticMethod<jboolean>(
            "org/openfairwind/fairwindsk/FairWindSKActivity",
            "isDefaultHomeApplication",
            "()Z");
#else
        return false;
#endif
    }

    void AndroidApplicationBridge::requestLauncherMode(const bool launcherMode) {
#if defined(Q_OS_ANDROID)
        QJniObject::callStaticMethod<void>(
            "org/openfairwind/fairwindsk/FairWindSKActivity",
            "requestHomeApplicationMode",
            "(Z)V",
            static_cast<jboolean>(launcherMode));
#else
        Q_UNUSED(launcherMode);
#endif
    }

    bool AndroidApplicationBridge::hasHardwareNavigationKey(const int keyCode) {
#if defined(Q_OS_ANDROID)
        return QJniObject::callStaticMethod<jboolean>(
            "org/openfairwind/fairwindsk/FairWindSKActivity",
            "hasHardwareNavigationKey",
            "(I)Z",
            keyCode);
#else
        Q_UNUSED(keyCode);
        return true;
#endif
    }

    void AndroidApplicationBridge::requestSystemBack() {
#if defined(Q_OS_ANDROID)
        QJniObject::callStaticMethod<void>(
            "org/openfairwind/fairwindsk/FairWindSKActivity",
            "requestSystemBack",
            "()V");
#endif
    }

} // fairwindsk::platform::android
