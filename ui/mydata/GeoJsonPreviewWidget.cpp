//
// Created by Codex on 21/03/26.
//

#include "GeoJsonPreviewWidget.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QVBoxLayout>
#include <QWebEngineView>

namespace fairwindsk::ui::mydata {
    GeoJsonPreviewWidget::GeoJsonPreviewWidget(QWidget *parent)
        : QWidget(parent),
          m_view(new QWebEngineView(this)) {
        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(m_view);
        setMinimumSize(320, 240);
        setMessage(tr("GeoJSON preview will appear here."), tr("GeoJSON Preview"));
    }

    void GeoJsonPreviewWidget::setGeoJson(const QJsonDocument &document, const QString &title) {
        const QString json = QString::fromUtf8(document.toJson(QJsonDocument::Compact));
        const QString script = QStringLiteral(R"(
const data = %1;
const svg = document.getElementById('preview');
const info = document.getElementById('info');
const width = 960;
const height = 540;

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
  info.textContent = 'No previewable geometry is available for this resource.';
} else {
  let minLon = allPoints[0][0], maxLon = allPoints[0][0], minLat = allPoints[0][1], maxLat = allPoints[0][1];
  allPoints.forEach(([lon, lat]) => {
    minLon = Math.min(minLon, lon); maxLon = Math.max(maxLon, lon);
    minLat = Math.min(minLat, lat); maxLat = Math.max(maxLat, lat);
  });

  const lonSpan = Math.max(0.0001, maxLon - minLon);
  const latSpan = Math.max(0.0001, maxLat - minLat);
  const pad = 28;

  function project(point) {
    const x = pad + ((point[0] - minLon) / lonSpan) * (width - 2 * pad);
    const y = height - pad - ((point[1] - minLat) / latSpan) * (height - 2 * pad);
    return [x, y];
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

  info.textContent = `${features.length} feature(s) rendered in the embedded preview.`;
}
)")
                .arg(json);

        m_view->setHtml(htmlForContent(title.isEmpty() ? tr("GeoJSON Preview") : title, script));
    }

    void GeoJsonPreviewWidget::setMessage(const QString &message, const QString &title) {
        const QString safeMessage = QString::fromUtf8(QJsonDocument(QJsonArray{message}).toJson(QJsonDocument::Compact));
        const QString script = QStringLiteral(R"(
const info = document.getElementById('info');
info.textContent = %1[0];
)").arg(safeMessage.trimmed());
        m_view->setHtml(htmlForContent(title.isEmpty() ? tr("GeoJSON Preview") : title, script));
    }

    QString GeoJsonPreviewWidget::htmlForContent(const QString &title, const QString &bodyScript) {
        const QString safeTitle = QString::fromUtf8(QJsonDocument(QJsonArray{title}).toJson(QJsonDocument::Compact)).trimmed();
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
    .header {
      padding: 14px 18px;
      border-bottom: 1px solid rgba(229, 237, 247, 0.12);
      font-size: 18px;
      font-weight: 700;
      letter-spacing: 0.02em;
    }
    .map {
      flex: 1;
      padding: 18px;
    }
    svg {
      width: 100%%;
      height: 100%%;
      background:
        linear-gradient(rgba(148, 163, 184, 0.08) 1px, transparent 1px),
        linear-gradient(90deg, rgba(148, 163, 184, 0.08) 1px, transparent 1px),
        linear-gradient(180deg, #0b1825, #102236);
      background-size: 40px 40px, 40px 40px, 100%% 100%%;
      border-radius: 14px;
      border: 1px solid rgba(229, 237, 247, 0.12);
      box-shadow: inset 0 0 40px rgba(7, 17, 27, 0.35);
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
    <div class="header" id="title"></div>
    <div class="map">
      <svg id="preview" viewBox="0 0 960 540" preserveAspectRatio="xMidYMid meet"></svg>
    </div>
    <div class="footer" id="info"></div>
  </div>
  <script>
    document.getElementById('title').textContent = %1[0];
    %2
  </script>
</body>
</html>)")
            .arg(safeTitle, bodyScript);
    }
}
