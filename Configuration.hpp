//
// Created by Raffaele Montella on 09/05/24.
//

#ifndef FAIRWINDSK_CONFIGURATION_HPP
#define FAIRWINDSK_CONFIGURATION_HPP

#include <QObject>
#include <QColor>
#include <QString>
#include <QSettings>
#include <nlohmann/json.hpp>


namespace fairwindsk {

    class Configuration { //: QObject {
        //Q_OBJECT

    public:
        Configuration();
        Configuration(const Configuration &configuration);
        explicit Configuration(const QString& filename);

        ~Configuration();

        void setDefault();

        nlohmann::json &getRoot();
        void setRoot(const nlohmann::json &jsonData);

        QString getVesselSpeedUnits();
        QString getWindSpeedUnits();
        QString getDistanceUnits();
        QString getRangeUnits();
        QString getDepthUnits();

        QString getSignalKServerUrl();
        void setSignalKServerUrl(const QString& signalKServerUrl);
        QString getSignalKPath(const QString &key) const;

        static QString getToken();
        static void setToken(const QString& token);
        static QString settingsFilename();

        QString getAutopilotApp();
        QString getAnchorApp();

        void setVirtualKeyboard(bool value);
        bool getVirtualKeyboard();

        void setUiScaleMode(const QString &value);
        QString getUiScaleMode() const;
        void setUiScalePreset(const QString &value);
        QString getUiScalePreset() const;
        void setComfortViewMode(const QString &value);
        QString getComfortViewMode() const;
        void setComfortViewPreset(const QString &value);
        QString getComfortViewPreset() const;
        void setComfortThemeStyleSheet(const QString &preset, const QString &styleSheet);
        QString getComfortThemeStyleSheet(const QString &preset) const;
        void clearComfortThemeStyleSheet(const QString &preset);
        void setComfortThemeColor(const QString &preset, const QString &key, const QColor &color);
        QColor getComfortThemeColor(const QString &preset, const QString &key, const QColor &fallback = QColor()) const;
        void clearComfortThemeColors(const QString &preset);
        void setComfortBackgroundImagePath(const QString &preset, const QString &area, const QString &path);
        QString getComfortBackgroundImagePath(const QString &preset, const QString &area) const;
        void clearComfortBackgroundImagePath(const QString &preset, const QString &area);
        void setLauncherRows(int value);
        int getLauncherRows() const;
        void setLauncherColumns(int value);
        int getLauncherColumns() const;
        void setCoordinateFormat(const QString &value);
        QString getCoordinateFormat() const;

        void setWindowMode(QString value);
        QString getWindowMode();
        void setWindowWidth(int value);
        int getWindowWidth();
        void setWindowHeight(int value);
        int getWindowHeight();
        void setWindowLeft(int value);
        int getWindowLeft();
        void setWindowTop(int value);
        int getWindowTop();

        QString getAutopilot();
        void setAutopilot(const QString& autopilot);


        void setFilename(QString filename);
        QString getFilename();
        void save();
        bool load();
        void save(const QString& filename);
        bool load(const QString& filename);

        int findApp(const QString& name);


    private:
        nlohmann::json &ensureObject(const char *key);
        QString getString(const char *section, const char *key, const QString &defaultValue = QString()) const;
        QString getString(const char *section, const QString &key, const QString &defaultValue = QString()) const;
        int getInt(const char *section, const char *key, int defaultValue = 0) const;
        bool getBool(const char *section, const char *key, bool defaultValue = false) const;

        nlohmann::json m_jsonData;
        QString m_filename;

        QString getUnits(const QString &units) const;

    };

} // fairwindsk

#endif //FAIRWINDSK_CONFIGURATION_HPP
