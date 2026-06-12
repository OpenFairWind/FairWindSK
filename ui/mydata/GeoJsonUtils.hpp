//
// Created by Codex on 21/03/26.
//

#ifndef FAIRWINDSK_UI_MYDATA_GEOJSONUTILS_HPP
#define FAIRWINDSK_UI_MYDATA_GEOJSONUTILS_HPP

#include <QJsonDocument>
#include <QList>

#include "HistoryTrackModel.hpp"
#include "ResourceModel.hpp"

namespace fairwindsk::ui::mydata {

    QJsonDocument exportResourcesAsGeoJson(ResourceKind kind, const QList<QPair<QString, QJsonObject>> &resources);
    bool importResourcesFromGeoJson(ResourceKind kind,
                                    const QJsonDocument &document,
                                    QList<QPair<QString, QJsonObject>> *resources,
                                    QString *message = nullptr);

    QJsonDocument exportTrackPointsAsGeoJson(const QList<HistoryTrackPoint> &points);
    bool importTrackPointsFromGeoJson(const QJsonDocument &document,
                                      QList<HistoryTrackPoint> *points,
                                      QString *message = nullptr);
}

#endif // FAIRWINDSK_UI_MYDATA_GEOJSONUTILS_HPP
