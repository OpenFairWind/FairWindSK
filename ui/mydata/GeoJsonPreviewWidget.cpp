//
// Created by Codex on 21/03/26.
//

#include "GeoJsonPreviewWidget.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QPlainTextEdit>
#include <QTabWidget>
#include <QUrl>
#include <QVBoxLayout>
#include <QWebEngineView>

#include "AppItem.hpp"
#include "FairWindSK.hpp"

namespace fairwindsk::ui::mydata {
    QString GeoJsonPreviewWidget::detectFreeboardUrl() {
        const auto fairWindSk = fairwindsk::FairWindSK::getInstance();
        if (!fairWindSk) {
            return {};
        }

        const auto hashes = fairWindSk->getAppsHashes();
        for (const auto &hash : hashes) {
            auto *appItem = fairWindSk->getAppItemByHash(hash);
            if (!appItem || !appItem->getActive()) {
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
          m_tabWidget(new QTabWidget(this)),
          m_view(new QWebEngineView(this)),
          m_freeboardView(new QWebEngineView(this)),
          m_textView(new QPlainTextEdit(this)) {
        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(m_tabWidget);
        m_tabWidget->addTab(m_view, tr("Preview"));
        m_textView->setReadOnly(true);
        m_textView->setLineWrapMode(QPlainTextEdit::NoWrap);
        m_textView->setStyleSheet("QPlainTextEdit { background: #f7f7f4; color: #1f2937; selection-background-color: #c7d2fe; selection-color: #111827; }");
        m_tabWidget->addTab(m_textView, tr("GeoJSON"));
        setMinimumSize(320, 240);
        setMessage(tr("GeoJSON preview will appear here."));
    }

    void GeoJsonPreviewWidget::ensureFreeboardTab(const QString &url) {
        const int freeboardTabIndex = m_tabWidget->indexOf(m_freeboardView);
        if (url.isEmpty()) {
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
        const QString json = QString::fromUtf8(document.toJson(QJsonDocument::Compact));
        const QString freeboardUrl = detectFreeboardUrl();
        ensureFreeboardTab(freeboardUrl);
        m_textView->setPlainText(QString::fromUtf8(document.toJson(QJsonDocument::Indented)));
        const QString script = QStringLiteral(R"(
const data = %1;
const map = document.getElementById('map');
const tilePane = document.getElementById('tile-pane');
const svg = document.getElementById('preview');
const info = document.getElementById('info');
const fallbackSource = document.getElementById('fallback-source');
const TILE_SIZE = 256;
const PADDING = 48;

function flattenCoordinates(geometry, into) {
  if (!geometry || geometry === null) return;
  const type = geometry.type;
  const coords = geometry.coordinates;
  if (!coords) return;

  function visit(value) {
    if (Array.isArray(value) && value.length >= 2 && typeof value[0] === 'number' && typeof value[1] === 'number') {
      into.push([value[0], value[1]]);
      return;
    }
    if (Array.isArray(value)) {
      value.forEach(visit);
    }
  }

  if (type === 'GeometryCollection' && Array.isArray(geometry.geometries)) {
    geometry.geometries.forEach(g => flattenCoordinates(g, into));
    return;
  }

  visit(coords);
}

function toFeatures(input) {
  if (!input) return [];
  if (input.type === 'FeatureCollection' && Array.isArray(input.features)) return input.features;
  if (input.type === 'Feature') return [input];
  return [];
}

const features = toFeatures(data);
const allPoints = [];
features.forEach(feature => flattenCoordinates(feature.geometry, allPoints));

if (!allPoints.length) {
  tilePane.innerHTML = '';
  svg.innerHTML = '';
  fallbackSource.textContent = '';
  info.textContent = 'No previewable geometry is available for this resource.';
} else {
  function projectMercator(point, zoom) {
    const sinLat = Math.sin(point[1] * Math.PI / 180);
    const worldSize = TILE_SIZE * Math.pow(2, zoom);
    const x = (point[0] + 180) / 360 * worldSize;
    const y = (0.5 - Math.log((1 + sinLat) / (1 - sinLat)) / (4 * Math.PI)) * worldSize;
    return [x, y];
  }

  let minLon = allPoints[0][0], maxLon = allPoints[0][0], minLat = allPoints[0][1], maxLat = allPoints[0][1];
  allPoints.forEach(([lon, lat]) => {
    minLon = Math.min(minLon, lon); maxLon = Math.max(maxLon, lon);
    minLat = Math.min(minLat, lat); maxLat = Math.max(maxLat, lat);
  });

  const viewportWidth = Math.max(320, map.clientWidth || 960);
  const viewportHeight = Math.max(240, map.clientHeight || 540);
  let zoom = 16;
  for (; zoom >= 1; --zoom) {
    const topLeft = projectMercator([minLon, maxLat], zoom);
    const bottomRight = projectMercator([maxLon, minLat], zoom);
    const spanX = Math.max(1, Math.abs(bottomRight[0] - topLeft[0]));
    const spanY = Math.max(1, Math.abs(bottomRight[1] - topLeft[1]));
    if (spanX <= viewportWidth - PADDING * 2 && spanY <= viewportHeight - PADDING * 2) {
      break;
    }
  }

  const center = projectMercator([(minLon + maxLon) / 2, (minLat + maxLat) / 2], zoom);
  const viewportOrigin = [center[0] - viewportWidth / 2, center[1] - viewportHeight / 2];
  const viewportEnd = [center[0] + viewportWidth / 2, center[1] + viewportHeight / 2];
  const startTileX = Math.floor(viewportOrigin[0] / TILE_SIZE);
  const endTileX = Math.floor(viewportEnd[0] / TILE_SIZE);
  const startTileY = Math.floor(viewportOrigin[1] / TILE_SIZE);
  const endTileY = Math.floor(viewportEnd[1] / TILE_SIZE);
  const maxTile = Math.pow(2, zoom) - 1;

  tilePane.innerHTML = '';
  overlayPane.innerHTML = '';
  svg.innerHTML = '';
  svg.setAttribute('viewBox', `0 0 ${viewportWidth} ${viewportHeight}`);

  function normalizedTileX(tileX) {
    const tileCount = Math.pow(2, zoom);
    return ((tileX %% tileCount) + tileCount) %% tileCount;
  }

  function appendTile(src, tileX, tileY, opacity) {
    if (tileY < 0 || tileY > maxTile) {
      return;
    }
    const tile = document.createElement('img');
    tile.src = src;
    tile.loading = 'lazy';
    tile.style.left = `${tileX * TILE_SIZE - viewportOrigin[0]}px`;
    tile.style.top = `${tileY * TILE_SIZE - viewportOrigin[1]}px`;
    tile.style.opacity = opacity;
    tilePane.appendChild(tile);
  }

  for (let tileX = startTileX; tileX <= endTileX; ++tileX) {
    for (let tileY = startTileY; tileY <= endTileY; ++tileY) {
      const normalizedX = normalizedTileX(tileX);
      appendTile(`https://tile.openstreetmap.org/${zoom}/${normalizedX}/${tileY}.png`, tileX, tileY, '1');
      appendTile(`https://tiles.openseamap.org/seamark/${zoom}/${normalizedX}/${tileY}.png`, tileX, tileY, '0.7');
    }
  }

  function project(point) {
    const worldPoint = projectMercator(point, zoom);
    return [worldPoint[0] - viewportOrigin[0], worldPoint[1] - viewportOrigin[1]];
  }

  function pathForCoordinates(coords, closePath) {
    if (!Array.isArray(coords) || !coords.length) return '';
    const projected = coords.map(project);
    let d = `M ${projected[0][0].toFixed(2)} ${projected[0][1].toFixed(2)}`;
    for (let i = 1; i < projected.length; ++i) {
      d += ` L ${projected[i][0].toFixed(2)} ${projected[i][1].toFixed(2)}`;
    }
    if (closePath) d += ' Z';
    return d;
  }

  features.forEach(feature => {
    const geometry = feature.geometry;
    if (!geometry) return;
    const type = geometry.type;
    const coords = geometry.coordinates;
    if (type === 'Point') {
      const [x, y] = project(coords);
      const circle = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
      circle.setAttribute('cx', x.toFixed(2));
      circle.setAttribute('cy', y.toFixed(2));
      circle.setAttribute('r', '7');
      circle.setAttribute('fill', '#f59e0b');
      circle.setAttribute('stroke', '#111827');
      circle.setAttribute('stroke-width', '2');
      svg.appendChild(circle);
    } else if (type === 'LineString') {
      const path = document.createElementNS('http://www.w3.org/2000/svg', 'path');
      path.setAttribute('d', pathForCoordinates(coords, false));
      path.setAttribute('fill', 'none');
      path.setAttribute('stroke', '#0ea5e9');
      path.setAttribute('stroke-width', '4');
      path.setAttribute('stroke-linecap', 'round');
      path.setAttribute('stroke-linejoin', 'round');
      svg.appendChild(path);
    } else if (type === 'Polygon') {
      (coords || []).forEach(ring => {
        const path = document.createElementNS('http://www.w3.org/2000/svg', 'path');
        path.setAttribute('d', pathForCoordinates(ring, true));
        path.setAttribute('fill', 'rgba(16,185,129,0.25)');
        path.setAttribute('stroke', '#10b981');
        path.setAttribute('stroke-width', '3');
        svg.appendChild(path);
      });
    } else if (type === 'MultiPoint') {
      (coords || []).forEach(point => {
        const [x, y] = project(point);
        const circle = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
        circle.setAttribute('cx', x.toFixed(2));
        circle.setAttribute('cy', y.toFixed(2));
        circle.setAttribute('r', '6');
        circle.setAttribute('fill', '#f59e0b');
        circle.setAttribute('stroke', '#111827');
        circle.setAttribute('stroke-width', '2');
        svg.appendChild(circle);
      });
    } else if (type === 'MultiLineString') {
      (coords || []).forEach(line => {
        const path = document.createElementNS('http://www.w3.org/2000/svg', 'path');
        path.setAttribute('d', pathForCoordinates(line, false));
        path.setAttribute('fill', 'none');
        path.setAttribute('stroke', '#0ea5e9');
        path.setAttribute('stroke-width', '4');
        path.setAttribute('stroke-linecap', 'round');
        path.setAttribute('stroke-linejoin', 'round');
        svg.appendChild(path);
      });
    } else if (type === 'MultiPolygon') {
      (coords || []).forEach(polygon => {
        (polygon || []).forEach(ring => {
          const path = document.createElementNS('http://www.w3.org/2000/svg', 'path');
          path.setAttribute('d', pathForCoordinates(ring, true));
          path.setAttribute('fill', 'rgba(16,185,129,0.25)');
          path.setAttribute('stroke', '#10b981');
          path.setAttribute('stroke-width', '3');
          svg.appendChild(path);
        });
      });
    }
  });

  fallbackSource.textContent = 'OpenStreetMap + OpenSeaMap fallback';
  info.textContent = `${features.length} feature(s) rendered over the map preview.`;
}
)")
                .arg(json);

        m_view->setHtml(htmlForContent(script));
    }

    void GeoJsonPreviewWidget::setMessage(const QString &message, const QString &) {
        ensureFreeboardTab(QString());
        const QString safeMessage = QString::fromUtf8(QJsonDocument(QJsonArray{message}).toJson(QJsonDocument::Compact));
        m_textView->setPlainText(message);
        const QString script = QStringLiteral(R"(
const tilePane = document.getElementById('tile-pane');
const svg = document.getElementById('preview');
const fallbackSource = document.getElementById('fallback-source');
const info = document.getElementById('info');
tilePane.innerHTML = '';
svg.innerHTML = '';
fallbackSource.textContent = '';
info.textContent = %1[0];
)").arg(safeMessage.trimmed());
        m_view->setHtml(htmlForContent(script));
    }

    QString GeoJsonPreviewWidget::htmlForContent(const QString &bodyScript) {
        return QStringLiteral(R"(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <style>
    body {
      margin: 0;
      background: #07111b;
      color: #e5edf7;
      font-family: "Avenir Next", "Helvetica Neue", sans-serif;
    }
    .shell {
      display: flex;
      flex-direction: column;
      height: 100vh;
      background: linear-gradient(180deg, #0b1724 0%%, #132739 100%%);
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
      border: 1px solid rgba(229, 237, 247, 0.12);
      overflow: hidden;
      background: #0b1825;
      box-shadow: inset 0 0 40px rgba(7, 17, 27, 0.35);
    }
    .tiles, .overlays, .overlays svg {
      position: absolute;
      inset: 0;
    }
    .tiles img {
      position: absolute;
      width: 256px;
      height: 256px;
      user-select: none;
      -webkit-user-drag: none;
    }
    .overlay-source {
      position: absolute;
      left: 14px;
      top: 14px;
      z-index: 2;
      padding: 6px 10px;
      border-radius: 999px;
      background: rgba(7, 17, 27, 0.72);
      color: #e5edf7;
      font-size: 12px;
      backdrop-filter: blur(8px);
    }
    iframe {
      display: block;
    }
    svg {
      width: 100%%;
      height: 100%%;
      pointer-events: none;
    }
    .footer {
      padding: 12px 18px 18px;
      color: #aebfd3;
      font-size: 13px;
    }
  </style>
</head>
<body>
  <div class="shell">
    <div class="map">
      <div class="map-stage" id="map">
        <div class="tiles" id="tile-pane"></div>
        <div class="overlay-source" id="fallback-source"></div>
        <div class="overlays" id="overlay-pane">
          <svg id="preview" viewBox="0 0 960 540" preserveAspectRatio="xMidYMid meet"></svg>
        </div>
      </div>
    </div>
    <div class="footer" id="info"></div>
  </div>
  <script>
    %1
  </script>
</body>
</html>)")
            .arg(bodyScript);
    }
}
