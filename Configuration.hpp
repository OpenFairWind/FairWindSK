//
// Created by Raffaele Montella on 09/05/24.
//

#ifndef FAIRWINDSK_CONFIGURATION_HPP
#define FAIRWINDSK_CONFIGURATION_HPP

#include <QObject>
#include <QString>
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

        static QString getToken();
        static void setToken(const QString& token);

        QString getAutopilotApp();
        QString getAnchorApp();

        void setFullScreen(bool value);
        bool getFullScreen();
        void setWindowWidth(int value);
        int getWindowWidth();
        void setWindowHeight(int value);
        int getWindowHeight();

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
        nlohmann::json m_jsonData;
        QString m_filename;

        QString getUnits(const QString &units);

    };

} // fairwindsk

#endif //FAIRWINDSK_CONFIGURATION_HPP
