//
// Created by Raffaele Montella on 03/04/21.
//

#ifndef FAIRWINDSK_FAIRWINDSK_HPP
#define FAIRWINDSK_FAIRWINDSK_HPP

#include <QString>
#include <QMainWindow>
#include <map>
#include <QJsonDocument>




namespace fairwindsk {
    /*
     * FairWind
     * Singleton used to handle the entire FairWind ecosystem in a centralized way
     */
    class FairWindSK: public QObject {
        Q_OBJECT
    public:
        static FairWindSK *getInstance();

        QJsonObject getConfig();

        void setMainWindow(QMainWindow *mainWindow);
        QMainWindow *getMainWindow();

    private:
        QMainWindow *m_mainWindow;


        FairWindSK();
        inline static FairWindSK *m_instance = nullptr;


    };
}

#endif //FAIRWINDSK_FAIRWINDSK_HPP