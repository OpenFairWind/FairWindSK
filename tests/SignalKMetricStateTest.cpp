#include "ui/widgets/SignalKMetricState.hpp"

#include <QColor>
#include <QtTest>

using fairwindsk::ui::widgets::SignalKMetricState;

class SignalKMetricStateTest final : public QObject {
    Q_OBJECT

private slots:
    void preservesOperationalStateColors();
};

void SignalKMetricStateTest::preservesOperationalStateColors() {
    // Deliberately use distinct colors so an accidental fallback is immediately visible.
    const QColor liveColor(QStringLiteral("#11aa22"));
    const QColor staleColor(QStringLiteral("#ffbb00"));
    const QColor missingColor(QStringLiteral("#667788"));

    QCOMPARE(fairwindsk::ui::widgets::signalKMetricTextColor(
                 SignalKMetricState::Live,
                 liveColor,
                 staleColor,
                 missingColor),
             liveColor);
    QCOMPARE(fairwindsk::ui::widgets::signalKMetricTextColor(
                 SignalKMetricState::Stale,
                 liveColor,
                 staleColor,
                 missingColor),
             staleColor);
    QCOMPARE(fairwindsk::ui::widgets::signalKMetricTextColor(
                 SignalKMetricState::Missing,
                 liveColor,
                 staleColor,
                 missingColor),
             missingColor);
}

QTEST_APPLESS_MAIN(SignalKMetricStateTest)

#include "SignalKMetricStateTest.moc"
