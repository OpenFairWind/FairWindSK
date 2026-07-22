#include "ProtocolUtils.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace fairwindsk::signalk {

    DeltaParseResult parseDelta(const QByteArray &payload) {
        DeltaParseResult result;
        QJsonParseError error;
        const QJsonDocument document = QJsonDocument::fromJson(payload, &error);
        if (error.error != QJsonParseError::NoError || !document.isObject()) {
            return result;
        }

        const QJsonObject envelope = document.object();
        if (!envelope.value(QStringLiteral("updates")).isArray()) {
            return result;
        }

        result.validEnvelope = true;
        const QString context = envelope.value(QStringLiteral("context")).toString();
        for (const QJsonValue &updateValue : envelope.value(QStringLiteral("updates")).toArray()) {
            if (!updateValue.isObject()) {
                continue;
            }
            const QJsonValue valuesValue = updateValue.toObject().value(QStringLiteral("values"));
            if (!valuesValue.isArray()) {
                continue;
            }
            for (const QJsonValue &itemValue : valuesValue.toArray()) {
                if (!itemValue.isObject()) {
                    continue;
                }
                const QJsonObject item = itemValue.toObject();
                const QString path = item.value(QStringLiteral("path")).toString();
                if (path.isEmpty() || !item.contains(QStringLiteral("value"))) {
                    continue;
                }
                result.values.append({context, path, item.value(QStringLiteral("value"))});
            }
        }
        return result;
    }

    bool isDataStale(const QDateTime &lastActivity, const QDateTime &now, const qint64 timeoutMs) {
        if (!lastActivity.isValid() || !now.isValid() || timeoutMs < 0) {
            return false;
        }
        return lastActivity.msecsTo(now) > timeoutMs;
    }

    ReconnectState nextReconnectState(const ReconnectState state, const ReconnectEvent event) {
        if (event == ReconnectEvent::Stop) {
            return ReconnectState::Disconnected;
        }
        switch (state) {
            case ReconnectState::Disconnected:
                return event == ReconnectEvent::Start ? ReconnectState::Connecting : state;
            case ReconnectState::Connecting:
                if (event == ReconnectEvent::Connected) return ReconnectState::Live;
                if (event == ReconnectEvent::ConnectionLost || event == ReconnectEvent::DiscoveryFailed) return ReconnectState::Waiting;
                return state;
            case ReconnectState::Live:
                return event == ReconnectEvent::ConnectionLost ? ReconnectState::Waiting : state;
            case ReconnectState::Waiting:
                return event == ReconnectEvent::RetryTimer ? ReconnectState::Recovering : state;
            case ReconnectState::Recovering:
                if (event == ReconnectEvent::Connected) return ReconnectState::Live;
                if (event == ReconnectEvent::ConnectionLost || event == ReconnectEvent::DiscoveryFailed) return ReconnectState::Waiting;
                return state;
        }
        return state;
    }

}
