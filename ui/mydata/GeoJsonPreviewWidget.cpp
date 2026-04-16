//
// Created by Codex on 21/03/26.
//

#include "GeoJsonPreviewWidget.hpp"

#include <algorithm>
#include <QJsonArray>
#include <QJsonDocument>
#include <QByteArray>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

#include "AppItem.hpp"
#include "FairWindSK.hpp"
#include "ui/IconUtils.hpp"
#include "ui/web/WebView.hpp"
#include "ui_GeoJsonPreviewWidget.h"

namespace fairwindsk::ui::mydata {
    namespace {
        void collectCoordinatesFromValue(const QJsonValue &value, QVector<QPair<double, double>> *coordinates) {
            if (!coordinates || value.isUndefined() || value.isNull()) {
                return;
            }

            if (value.isArray()) {
                const auto array = value.toArray();
                if (array.size() >= 2 && array.at(0).isDouble() && array.at(1).isDouble()) {
                    coordinates->append({array.at(0).toDouble(), array.at(1).toDouble()});
                    return;
                }

                for (const auto &item : array) {
                    collectCoordinatesFromValue(item, coordinates);
                }
                return;
            }

            if (value.isObject()) {
                const auto object = value.toObject();
                if (object["type"].toString() == "FeatureCollection" && object["features"].isArray()) {
                    collectCoordinatesFromValue(object["features"], coordinates);
                    return;
                }
                if (object["type"].toString() == "Feature") {
                    collectCoordinatesFromValue(object["geometry"], coordinates);
                    return;
                }
                if (object.contains("coordinates")) {
                    collectCoordinatesFromValue(object["coordinates"], coordinates);
                }
            }
        }
    }

    QString GeoJsonPreviewWidget::detectFreeboardUrl() {
        const auto fairWindSk = fairwindsk::FairWindSK::getInstance();
        if (!fairWindSk) {
            return {};
        }

        const auto hashes = fairWindSk->getAppsHashes();
        for (const auto &hash : hashes) {
            auto *appItem = fairWindSk->getAppItemByHash(hash);
            if (!appItem) {
                continue;
            }

            const QStringList candidates = {
                appItem->getName(),
                appItem->getDisplayName(),
                appItem->getUrl()
            };

            for (const auto &candidate : candidates) {
                if (candidate.contains("freeboard", Qt::CaseInsensitive)) {
                    return appItem->getUrl();
                }
            }
        }

        return {};
    }

    GeoJsonPreviewWidget::GeoJsonPreviewWidget(QWidget *parent)
        : QWidget(parent),
          ui(new Ui::GeoJsonPreviewWidget),
          m_freeboardView(new fairwindsk::ui::web::WebView(FairWindSK::getInstance() ? FairWindSK::getInstance()->getWebEngineProfile() : nullptr, this)),
          m_textView(nullptr) {
        ui->setupUi(this);
        m_tabWidget = ui->tabWidget;
        auto *previewLayout = new QVBoxLayout(ui->widgetPreviewHost);
        previewLayout->setContentsMargins(0, 0, 0, 0);
        previewLayout->setSpacing(0);
        m_view = new fairwindsk::ui::web::WebView(FairWindSK::getInstance() ? FairWindSK::getInstance()->getWebEngineProfile() : nullptr, ui->widgetPreviewHost);
        previewLayout->addWidget(m_view);
        m_textView = ui->plainTextEditGeoJson;
        m_textView->setReadOnly(true);
        m_textView->setLineWrapMode(QPlainTextEdit::NoWrap);
        setMinimumSize(320, 240);
        connect(m_freeboardView, &fairwindsk::ui::web::WebView::loadFinished, this, [this](const bool ok) {
            if (ok) {
                scheduleFreeboardFocus();
            }
        });
        setMessage(tr("GeoJSON preview will appear here."));
    }

    GeoJsonPreviewWidget::~GeoJsonPreviewWidget() {
        delete ui;
    }

    void GeoJsonPreviewWidget::setFreeboardEnabled(const bool enabled) {
        m_freeboardEnabled = enabled;
        if (!enabled) {
            ensureFreeboardTab(QString());
        }
    }

    void GeoJsonPreviewWidget::setCurrentView(const int index) {
        m_tabWidget->setCurrentIndex(index);
    }

    void GeoJsonPreviewWidget::setTabBarAutoHide(const bool hide) {
        m_tabWidget->setTabBarAutoHide(hide);
    }

    void GeoJsonPreviewWidget::setGeoJsonTabVisible(const bool visible) {
        const int geoJsonIndex = m_tabWidget->indexOf(m_textView);
        if (visible) {
            if (geoJsonIndex < 0) {
                m_tabWidget->addTab(m_textView, tr("GeoJSON"));
            }
        } else if (geoJsonIndex >= 0) {
            m_tabWidget->removeTab(geoJsonIndex);
        }
    }

    QString GeoJsonPreviewWidget::geoJsonText() const {
        return m_textView->toPlainText();
    }

    void GeoJsonPreviewWidget::ensureFreeboardTab(const QString &url) {
        const int freeboardTabIndex = m_tabWidget->indexOf(m_freeboardView);
        if (!m_freeboardEnabled || url.isEmpty()) {
            if (freeboardTabIndex >= 0) {
                m_tabWidget->removeTab(freeboardTabIndex);
            }
            m_freeboardView->setHtml(QString());
            return;
        }

        if (freeboardTabIndex < 0) {
            m_tabWidget->addTab(m_freeboardView, tr("Freeboard"));
        }
        m_freeboardView->load(QUrl(url));
    }

    void GeoJsonPreviewWidget::setGeoJson(const QJsonDocument &document, const QString &) {
        const QString base64Json = QString::fromLatin1(document.toJson(QJsonDocument::Compact).toBase64());
        const QString freeboardUrl = m_freeboardEnabled ? detectFreeboardUrl() : QString{};
        const QColor accentColor = palette().color(QPalette::Highlight);
        const QColor accentStroke = fairwindsk::ui::bestContrastingColor(
            accentColor,
            {palette().color(QPalette::WindowText),
             palette().color(QPalette::Text),
             QColor(Qt::black),
             QColor(Qt::white)});
        const QColor fillColor = fairwindsk::ui::comfortAlpha(accentColor, 72);
        updateFocusCoordinate(document);
        ensureFreeboardTab(freeboardUrl);
        m_textView->setPlainText(QString::fromUtf8(document.toJson(QJsonDocument::Indented)));
        const QString script = QStringLiteral(R"(
const data = JSON.parse(atob('%1'));
function renderPreview() {
  if (!window.ol) {
    window.setTimeout(renderPreview, 150);
    return;
  }

  const mapNode = document.getElementById('map');
  const info = document.getElementById('info');
  const fallbackSource = document.getElementById('fallback-source');
  if (!mapNode || !info || !fallbackSource) {
    return;
  }
  const format = new ol.format.GeoJSON();
  const featureCollection = data.type === 'FeatureCollection'
    ? data
    : (data.type === 'Feature' ? { type: 'FeatureCollection', features: [data] } : { type: 'FeatureCollection', features: [] });
  const features = format.readFeatures(featureCollection, {
    dataProjection: 'EPSG:4326',
    featureProjection: 'EPSG:3857'
  });

  if (!features.length) {
    fallbackSource.textContent = '';
    info.textContent = 'No previewable geometry is available for this resource.';
    mapNode.innerHTML = '';
    return;
  }

  mapNode.innerHTML = '';
  const vectorSource = new ol.source.Vector({ features });
  const vectorLayer = new ol.layer.Vector({
    source: vectorSource,
    style: new ol.style.Style({
      image: new ol.style.Circle({
        radius: 7,
        fill: new ol.style.Fill({ color: '%4' }),
        stroke: new ol.style.Stroke({ color: '%5', width: 2 })
      }),
      stroke: new ol.style.Stroke({ color: '%4', width: 3 }),
      fill: new ol.style.Fill({ color: '%6' })
    })
  });

  const map = new ol.Map({
    target: mapNode,
    controls: [],
    interactions: ol.interaction.defaults({ altShiftDragRotate: false, pinchRotate: false }),
    layers: [
      new ol.layer.Tile({ source: new ol.source.OSM() }),
      new ol.layer.Tile({ source: new ol.source.XYZ({ url: 'https://tiles.openseamap.org/seamark/{z}/{x}/{y}.png' }), opacity: 0.7 }),
      vectorLayer
    ],
    view: new ol.View({
      center: ol.proj.fromLonLat([%2, %3]),
      zoom: 11
    })
  });

  const extent = vectorSource.getExtent();
  if (extent && ol.extent.getWidth(extent) > 0 && ol.extent.getHeight(extent) > 0) {
    map.getView().fit(extent, { padding: [40, 40, 40, 40], maxZoom: 11, duration: 0 });
  } else {
    map.getView().setCenter(ol.proj.fromLonLat([%2, %3]));
    map.getView().setZoom(11);
  }

  fallbackSource.textContent = 'OpenLayers, OpenStreetMap + OpenSeaMap';
  info.textContent = `${features.length} feature(s) rendered over the map preview.`;
}

let resizeTimer = null;
window.addEventListener('load', renderPreview);
window.addEventListener('resize', () => {
  window.clearTimeout(resizeTimer);
  resizeTimer = window.setTimeout(renderPreview, 100);
});
window.setTimeout(renderPreview, 0);
window.setTimeout(renderPreview, 250);
window.setTimeout(renderPreview, 1000);
)")
            .arg(base64Json)
            .arg(m_focusLongitude, 0, 'f', 8)
            .arg(m_focusLatitude, 0, 'f', 8)
            .arg(accentColor.name())
            .arg(accentStroke.name())
            .arg(fillColor.name(QColor::HexArgb));

        m_view->setHtml(htmlForContent(script), QUrl(QStringLiteral("https://preview.local/")));
    }

    void GeoJsonPreviewWidget::setMessage(const QString &message, const QString &) {
        ensureFreeboardTab(QString());
        const QString safeMessage = QString::fromUtf8(QJsonDocument(QJsonArray{message}).toJson(QJsonDocument::Compact));
        m_textView->setPlainText(message);
        const QString script = QStringLiteral(R"(
const map = document.getElementById('map');
const fallbackSource = document.getElementById('fallback-source');
const info = document.getElementById('info');
map.innerHTML = '';
fallbackSource.textContent = '';
info.textContent = %1[0];
)").arg(safeMessage.trimmed());
        m_view->setHtml(htmlForContent(script), QUrl(QStringLiteral("https://preview.local/")));
    }

    void GeoJsonPreviewWidget::updateFocusCoordinate(const QJsonDocument &document) {
        QVector<QPair<double, double>> coordinates;
        collectCoordinatesFromValue(document.isObject() ? QJsonValue(document.object()) : QJsonValue(document.array()),
                                    &coordinates);

        if (coordinates.isEmpty()) {
            m_hasFocusCoordinate = false;
            m_focusLongitude = 0.0;
            m_focusLatitude = 0.0;
            m_minLongitude = 0.0;
            m_maxLongitude = 0.0;
            m_minLatitude = 0.0;
            m_maxLatitude = 0.0;
            return;
        }

        double longitudeSum = 0.0;
        double latitudeSum = 0.0;
        m_minLongitude = coordinates.first().first;
        m_maxLongitude = coordinates.first().first;
        m_minLatitude = coordinates.first().second;
        m_maxLatitude = coordinates.first().second;
        for (const auto &coordinate : coordinates) {
            longitudeSum += coordinate.first;
            latitudeSum += coordinate.second;
            m_minLongitude = std::min(m_minLongitude, coordinate.first);
            m_maxLongitude = std::max(m_maxLongitude, coordinate.first);
            m_minLatitude = std::min(m_minLatitude, coordinate.second);
            m_maxLatitude = std::max(m_maxLatitude, coordinate.second);
        }

        m_hasFocusCoordinate = true;
        m_focusLongitude = longitudeSum / coordinates.size();
        m_focusLatitude = latitudeSum / coordinates.size();
    }

    void GeoJsonPreviewWidget::scheduleFreeboardFocus() {
        applyFreeboardFocus();
        QTimer::singleShot(600, this, &GeoJsonPreviewWidget::applyFreeboardFocus);
        QTimer::singleShot(1500, this, &GeoJsonPreviewWidget::applyFreeboardFocus);
        QTimer::singleShot(3000, this, &GeoJsonPreviewWidget::applyFreeboardFocus);
    }

    void GeoJsonPreviewWidget::applyFreeboardFocus() {
        if (!m_hasFocusCoordinate || !m_freeboardView) {
            return;
        }

        const QString script = QStringLiteral(R"(
(function() {
  const lon = %1;
  const lat = %2;
  const minLon = %3;
  const minLat = %4;
  const maxLon = %5;
  const maxLat = %6;
  function projectToMercator(longitude, latitude) {
    const x = longitude * 20037508.34 / 180.0;
    let y = Math.log(Math.tan((90.0 + latitude) * Math.PI / 360.0)) / (Math.PI / 180.0);
    y = y * 20037508.34 / 180.0;
    return [x, y];
  }
  const center = projectToMercator(lon, lat);
  const paddedExtentLon = Math.max(0.01, Math.abs(maxLon - minLon) * 1.8);
  const paddedExtentLat = Math.max(0.01, Math.abs(maxLat - minLat) * 1.8);
  const mercatorExtent = [
    projectToMercator(lon - paddedExtentLon / 2, lat - paddedExtentLat / 2)[0],
    projectToMercator(lon - paddedExtentLon / 2, lat - paddedExtentLat / 2)[1],
    projectToMercator(lon + paddedExtentLon / 2, lat + paddedExtentLat / 2)[0],
    projectToMercator(lon + paddedExtentLon / 2, lat + paddedExtentLat / 2)[1]
  ];
  const geoExtent = [lon - paddedExtentLon / 2, lat - paddedExtentLat / 2, lon + paddedExtentLon / 2, lat + paddedExtentLat / 2];
  const visited = new Set();
  const queue = [window];
  let centered = 0;
  while (queue.length) {
    const current = queue.shift();
    if (!current || (typeof current !== 'object' && typeof current !== 'function') || visited.has(current)) {
      continue;
    }
    visited.add(current);
    try {
      const mapObject = typeof current.getView === 'function' ? current : (current.map && typeof current.map.getView === 'function' ? current.map : null);
      if (mapObject) {
        const view = mapObject.getView();
        if (view && typeof view.setCenter === 'function') {
          if (typeof view.fit === 'function') {
            try {
              if (window.ol && window.ol.proj && typeof window.ol.proj.transformExtent === 'function') {
                view.fit(window.ol.proj.transformExtent(geoExtent, 'EPSG:4326', 'EPSG:3857'), {
                  padding: [48, 48, 48, 48],
                  maxZoom: 15,
                  duration: 0
                });
              } else {
                view.fit(mercatorExtent, {
                  padding: [48, 48, 48, 48],
                  maxZoom: 15,
                  duration: 0
                });
              }
            } catch (error) {
              view.setCenter(center);
            }
          } else {
            view.setCenter(center);
          }
          if (typeof view.getZoom === 'function' && typeof view.setZoom === 'function' && view.getZoom() < 13) {
            view.setZoom(15);
          }
          centered += 1;
        }
      }
      if (current.setView && current.flyTo) {
        current.setView([lat, lon], Math.max(14, current.getZoom ? current.getZoom() : 14), { animate: false });
        centered += 1;
      }
    } catch (error) {}
    try {
      for (const key of Object.keys(current).slice(0, 100)) {
        const next = current[key];
        if (next && !visited.has(next)) {
          queue.push(next);
        }
      }
    } catch (error) {}
  }
  return centered;
})();
)")
            .arg(m_focusLongitude, 0, 'f', 8)
            .arg(m_focusLatitude, 0, 'f', 8)
            .arg(m_minLongitude, 0, 'f', 8)
            .arg(m_minLatitude, 0, 'f', 8)
            .arg(m_maxLongitude, 0, 'f', 8)
            .arg(m_maxLatitude, 0, 'f', 8);

        m_freeboardView->runJavaScript(script);
    }

    QString GeoJsonPreviewWidget::htmlForContent(const QString &bodyScript) const {
        const QColor windowColor = palette().color(QPalette::Window);
        const QColor textColor = fairwindsk::ui::bestContrastingColor(
            windowColor,
            {palette().color(QPalette::Text),
             palette().color(QPalette::WindowText),
             palette().color(QPalette::ButtonText),
             QColor(Qt::white),
             QColor(Qt::black)});
        const QColor panelColor = palette().color(QPalette::Base);
        const QColor mutedColor = fairwindsk::ui::comfortAlpha(textColor, 188);
        const QColor borderColor = fairwindsk::ui::comfortAlpha(palette().color(QPalette::Mid), 180);
        const QColor shadowColor = fairwindsk::ui::comfortAlpha(windowColor.darker(170), 180);
        const QColor overlayColor = fairwindsk::ui::comfortAlpha(windowColor, 192);

        return QStringLiteral(R"(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/ol@v10.6.1/ol.css" />
  <style>
    body {
      margin: 0;
      background: %1;
      color: %2;
      font-family: "Avenir Next", "Helvetica Neue", sans-serif;
    }
    .shell {
      display: flex;
      flex-direction: column;
      height: 100vh;
      background: linear-gradient(180deg, %3 0%%, %4 100%%);
    }
    .map {
      flex: 1;
      padding: 14px;
    }
    .map-stage {
      position: relative;
      width: 100%%;
      height: 100%%;
      border-radius: 14px;
      border: 1px solid %5;
      overflow: hidden;
      background: %6;
      box-shadow: inset 0 0 40px %7;
    }
    .overlay-source {
      position: absolute;
      left: 14px;
      top: 14px;
      z-index: 2;
      padding: 6px 10px;
      border-radius: 999px;
      background: %8;
      color: %2;
      font-size: 12px;
      backdrop-filter: blur(8px);
    }
    iframe {
      display: block;
    }
    .ol-viewport, .ol-layers, .ol-layer, .ol-layer canvas {
      border-radius: 14px;
    }
    .footer {
      padding: 12px 18px 18px;
      color: %9;
      font-size: 13px;
    }
  </style>
</head>
<body>
  <div class="shell">
    <div class="map">
      <div class="map-stage" id="map">
        <div class="overlay-source" id="fallback-source"></div>
      </div>
    </div>
    <div class="footer" id="info"></div>
  </div>
  <script src="https://cdn.jsdelivr.net/npm/ol@v10.6.1/dist/ol.js"></script>
  <script>
    %10
  </script>
</body>
</html>)")
            .arg(windowColor.name(),
                 textColor.name(),
                 panelColor.lighter(104).name(),
                 panelColor.darker(106).name(),
                 borderColor.name(QColor::HexArgb),
                 panelColor.name(),
                 shadowColor.name(QColor::HexArgb),
                 overlayColor.name(QColor::HexArgb),
                 mutedColor.name(QColor::HexArgb),
                 bodyScript);
    }
}
