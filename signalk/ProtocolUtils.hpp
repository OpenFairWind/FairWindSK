#ifndef FAIRWINDSK_SIGNALK_PROTOCOLUTILS_HPP
#define FAIRWINDSK_SIGNALK_PROTOCOLUTILS_HPP

#include <QDateTime>
#include <QJsonValue>
#include <QList>
#include <QString>

namespace fairwindsk::signalk {

    struct DeltaValue {
        QString context;
        QString path;
        QJsonValue value;
    };

    struct DeltaParseResult {
        bool validEnvelope = false;
        QList<DeltaValue> values;
    };

    enum class ReconnectState {
        Disconnected,
        Connecting,
        Live,
        Waiting,
        Recovering
    };

    enum class ReconnectEvent {
        Start,
        Connected,
        ConnectionLost,
        RetryTimer,
        DiscoveryFailed,
        Stop
    };

    DeltaParseResult parseDelta(const QByteArray &payload);
    bool isDataStale(const QDateTime &lastActivity, const QDateTime &now, qint64 timeoutMs);
    ReconnectState nextReconnectState(ReconnectState state, ReconnectEvent event);

}

#endif
