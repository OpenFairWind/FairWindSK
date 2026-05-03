#include "WidgetPalette.hpp"

#include <QApplication>
#include <QDrag>
#include <QGridLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMimeData>
#include <QMouseEvent>
#include <QPushButton>

#include "FairWindSK.hpp"
#include "ui/IconUtils.hpp"

namespace fairwindsk::ui::settings {
    namespace {
        using fairwindsk::ui::layout::EntryKind;
        using fairwindsk::ui::layout::LayoutEntry;

        constexpr auto kWidgetPaletteMimeType = "application/x-fairwindsk-widget-palette-entry";
        constexpr int kPaletteRowCount = 3;
        constexpr int kButtonWidth = 66;
        constexpr int kButtonHeight = 60;
        constexpr int kIconSize = 36;

        class PalettePushButton final : public QPushButton {
        public:
            PalettePushButton(const LayoutEntry &entry, QWidget *parent = nullptr)
                : QPushButton(parent),
                  m_entry(entry) {
            }

        protected:
            void mousePressEvent(QMouseEvent *event) override {
                if (event && event->button() == Qt::LeftButton) {
                    m_dragStartPosition = event->pos();
                }
                QPushButton::mousePressEvent(event);
            }

            void mouseMoveEvent(QMouseEvent *event) override {
                if (!event || !(event->buttons() & Qt::LeftButton)) {
                    QPushButton::mouseMoveEvent(event);
                    return;
                }

                if ((event->pos() - m_dragStartPosition).manhattanLength() < QApplication::startDragDistance()) {
                    QPushButton::mouseMoveEvent(event);
                    return;
                }

                auto *mimeData = new QMimeData();
                mimeData->setData(WidgetPalette::mimeType().toUtf8(), WidgetPalette::encodeEntry(m_entry));

                auto *drag = new QDrag(this);
                drag->setMimeData(mimeData);
                if (!icon().isNull()) {
                    drag->setPixmap(icon().pixmap(iconSize()));
                }
                drag->exec(Qt::CopyAction, Qt::CopyAction);
            }

        private:
            LayoutEntry m_entry;
            QPoint m_dragStartPosition;
        };

        QString lcdMetricIconPath(const QString &widgetId) {
            if (widgetId == QStringLiteral("cog")) {
                return QStringLiteral(":/resources/svg/OpenBridge/lcd-cog.svg");
            }
            if (widgetId == QStringLiteral("position")) {
                return QStringLiteral(":/resources/svg/OpenBridge/lcd-position.svg");
            }
            if (widgetId == QStringLiteral("sog")) {
                return QStringLiteral(":/resources/svg/OpenBridge/lcd-sog.svg");
            }
            if (widgetId == QStringLiteral("hdg")) {
                return QStringLiteral(":/resources/svg/OpenBridge/lcd-hdg.svg");
            }
            if (widgetId == QStringLiteral("stw")) {
                return QStringLiteral(":/resources/svg/OpenBridge/lcd-stw.svg");
            }
            if (widgetId == QStringLiteral("dpt")) {
                return QStringLiteral(":/resources/svg/OpenBridge/lcd-dpt.svg");
            }
            if (widgetId == QStringLiteral("wpt")) {
                return QStringLiteral(":/resources/svg/OpenBridge/lcd-wpt.svg");
            }
            if (widgetId == QStringLiteral("btw")) {
                return QStringLiteral(":/resources/svg/OpenBridge/lcd-btw.svg");
            }
            if (widgetId == QStringLiteral("dtg")) {
                return QStringLiteral(":/resources/svg/OpenBridge/lcd-dtg.svg");
            }
            if (widgetId == QStringLiteral("ttg")) {
                return QStringLiteral(":/resources/svg/OpenBridge/lcd-ttg.svg");
            }
            if (widgetId == QStringLiteral("eta")) {
                return QStringLiteral(":/resources/svg/OpenBridge/lcd-eta.svg");
            }
            if (widgetId == QStringLiteral("xte")) {
                return QStringLiteral(":/resources/svg/OpenBridge/lcd-xte.svg");
            }
            if (widgetId == QStringLiteral("vmg")) {
                return QStringLiteral(":/resources/svg/OpenBridge/lcd-vmg.svg");
            }
            if (widgetId == QStringLiteral("current_context")) {
                return QStringLiteral(":/resources/svg/OpenBridge/lcd-app.svg");
            }
            if (widgetId == QStringLiteral("clock_icons")) {
                return QStringLiteral(":/resources/svg/OpenBridge/lcd-clock.svg");
            }
            if (widgetId == QStringLiteral("signalk_status")) {
                return QStringLiteral(":/resources/svg/OpenBridge/layout-signalk.svg");
            }
            if (widgetId == QStringLiteral("open_apps")) {
                return QStringLiteral(":/resources/svg/OpenBridge/layout-open-apps.svg");
            }
            return {};
        }

        QIcon iconForWidgetId(const QString &widgetId) {
            const QString lcdMetricIcon = lcdMetricIconPath(widgetId);
            if (!lcdMetricIcon.isEmpty()) {
                return QIcon(lcdMetricIcon);
            }

            if (widgetId == QStringLiteral("apps")) {
                return QIcon(QStringLiteral(":/resources/svg/OpenBridge/applications.svg"));
            }
            if (widgetId == QStringLiteral("mydata")) {
                return QIcon(QStringLiteral(":/resources/svg/OpenBridge/database.svg"));
            }
            if (widgetId == QStringLiteral("pob")) {
                return QIcon(QStringLiteral(":/resources/svg/OpenBridge/alarm-pob.svg"));
            }
            if (widgetId == QStringLiteral("autopilot")) {
                return QIcon(QStringLiteral(":/resources/svg/OpenBridge/command-autopilot.svg"));
            }
            if (widgetId == QStringLiteral("anchor")) {
                return QIcon(QStringLiteral(":/resources/svg/OpenBridge/anchor-iec.svg"));
            }
            if (widgetId == QStringLiteral("alarms")) {
                return QIcon(QStringLiteral(":/resources/svg/OpenBridge/alarm-general.svg"));
            }
            if (widgetId == QStringLiteral("settings")) {
                return QIcon(QStringLiteral(":/resources/svg/OpenBridge/settings-iec.svg"));
            }
            return {};
        }

        QIcon iconForEntry(const LayoutEntry &entry) {
            if (entry.kind == EntryKind::Stretch) {
                return QIcon(QStringLiteral(":/resources/svg/OpenBridge/layout-elastic-extender.svg"));
            }
            if (entry.kind == EntryKind::Separator) {
                return QIcon(QStringLiteral(":/resources/svg/OpenBridge/layout-separator.svg"));
            }
            return iconForWidgetId(entry.widgetId);
        }

        QString descriptionForEntry(const LayoutEntry &entry, const bool active) {
            if (entry.kind == EntryKind::Separator) {
                return QObject::tr("Visual divider");
            }
            if (entry.kind == EntryKind::Stretch) {
                return QObject::tr("Elastic extender");
            }
            return active ? QObject::tr("On bar") : QObject::tr("Tap or drag");
        }

        QString buttonStyleSheet(const fairwindsk::ui::ComfortChromeColors &colors) {
            return QStringLiteral(
                "QPushButton {"
                " text-align: center;"
                " padding: 6px;"
                " border-radius: 8px;"
                " border: 1px solid %1;"
                " background: %2;"
                " color: %3;"
                " }"
                "QPushButton:hover {"
                " background: %4;"
                " }"
                "QPushButton:pressed, QPushButton:checked {"
                " background: %5;"
                " color: %6;"
                " border-color: %5;"
                " }"
                "QPushButton:disabled {"
                " color: %7;"
                " border-color: %8;"
                " }")
                .arg(colors.border.name(),
                     fairwindsk::ui::comfortAlpha(colors.buttonBackground, 18).name(QColor::HexArgb),
                     colors.text.name(),
                     fairwindsk::ui::comfortAlpha(colors.hoverBackground, 84).name(QColor::HexArgb),
                     colors.accentBottom.name(),
                     colors.accentText.name(),
                     colors.disabledText.name(),
                     colors.border.darker(120).name());
        }
    }

    WidgetPalette::WidgetPalette(QWidget *parent)
        : QScrollArea(parent) {
        setWidgetResizable(true);
        setFrameShape(QFrame::NoFrame);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        m_container = new QWidget(this);
        m_layout = new QGridLayout(m_container);
        m_layout->setContentsMargins(0, 0, 0, 0);
        m_layout->setHorizontalSpacing(8);
        m_layout->setVerticalSpacing(8);
        m_container->setLayout(m_layout);
        setWidget(m_container);

        setMinimumHeight((kPaletteRowCount * kButtonHeight) + 36);
        setMaximumHeight((kPaletteRowCount * kButtonHeight) + 36);
    }

    QString WidgetPalette::mimeType() {
        return QString::fromLatin1(kWidgetPaletteMimeType);
    }

    QByteArray WidgetPalette::encodeEntry(const LayoutEntry &entry) {
        QJsonObject payload;
        payload.insert(QStringLiteral("kind"), static_cast<int>(entry.kind));
        payload.insert(QStringLiteral("widgetId"), entry.widgetId);
        payload.insert(QStringLiteral("instanceId"), entry.instanceId);
        payload.insert(QStringLiteral("expandHorizontally"), entry.expandHorizontally);
        payload.insert(QStringLiteral("expandVertically"), entry.expandVertically);
        return QJsonDocument(payload).toJson(QJsonDocument::Compact);
    }

    LayoutEntry WidgetPalette::decodeEntry(const QByteArray &payload) {
        LayoutEntry entry;
        const auto document = QJsonDocument::fromJson(payload);
        if (!document.isObject()) {
            return entry;
        }

        const auto object = document.object();
        entry.kind = static_cast<EntryKind>(object.value(QStringLiteral("kind")).toInt());
        entry.widgetId = object.value(QStringLiteral("widgetId")).toString();
        entry.instanceId = object.value(QStringLiteral("instanceId")).toString();
        entry.expandHorizontally = object.value(QStringLiteral("expandHorizontally")).toBool();
        entry.expandVertically = object.value(QStringLiteral("expandVertically")).toBool();
        return entry;
    }

    QIcon WidgetPalette::entryIcon(const LayoutEntry &entry) {
        return iconForEntry(entry);
    }

    void WidgetPalette::setEntries(const QList<LayoutEntry> &entries) {
        while (m_layout && m_layout->count() > 0) {
            auto *item = m_layout->takeAt(0);
            if (item && item->widget()) {
                delete item->widget();
            }
            delete item;
        }
        m_buttons.clear();

        for (int index = 0; index < entries.size(); ++index) {
            const auto &entry = entries.at(index);
            auto *button = new PalettePushButton(entry, m_container);
            button->setMinimumSize(QSize(kButtonWidth, kButtonHeight));
            button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            button->setCheckable(entry.kind == EntryKind::Widget);
            button->setIcon(entryIcon(entry));
            button->setIconSize(QSize(kIconSize, kIconSize));
            button->setText(QString());
            button->setToolTip(layout::entryLabel(entry));
            button->setAccessibleName(layout::entryLabel(entry));
            connect(button, &QPushButton::clicked, this, [this, entry]() {
                emit entryActivated(entry);
            });

            const int row = index % kPaletteRowCount;
            const int column = index / kPaletteRowCount;
            m_layout->addWidget(button, row, column);
            m_buttons.append({entry, button});
        }

        m_layout->setColumnStretch(entries.size() / kPaletteRowCount + 1, 1);
        applyChrome();
    }

    void WidgetPalette::setActiveEntries(const QList<LayoutEntry> &entries) {
        QStringList activeWidgetIds;
        for (const auto &entry : entries) {
            if (entry.kind == EntryKind::Widget && entry.enabled) {
                activeWidgetIds.append(entry.widgetId);
            }
        }

        for (auto &paletteButton : m_buttons) {
            if (!paletteButton.button) {
                continue;
            }

            const bool active = paletteButton.entry.kind == EntryKind::Widget
                                    ? activeWidgetIds.contains(paletteButton.entry.widgetId)
                                    : false;
            paletteButton.button->setChecked(active);
            paletteButton.button->setToolTip(QStringLiteral("%1\n%2")
                                                 .arg(layout::entryLabel(paletteButton.entry),
                                                      descriptionForEntry(paletteButton.entry, active)));
        }

        applyChrome();
    }

    void WidgetPalette::applyChrome() {
        auto *fairWindSK = FairWindSK::getInstance();
        const auto *configuration = fairWindSK ? fairWindSK->getConfiguration() : nullptr;
        const QString preset = fairWindSK ? fairWindSK->getActiveComfortViewPreset(configuration) : QStringLiteral("default");
        const auto chrome = fairwindsk::ui::resolveComfortChromeColors(configuration, preset, palette(), false);
        const QString styleSheet = buttonStyleSheet(chrome);

        for (auto &paletteButton : m_buttons) {
            if (!paletteButton.button) {
                continue;
            }

            paletteButton.button->setStyleSheet(styleSheet);
            if (!paletteButton.button->icon().isNull()) {
                const QColor iconColor = paletteButton.button->isChecked() ? chrome.accentText : chrome.text;
                fairwindsk::ui::applyTintedButtonIcon(paletteButton.button, iconColor, QSize(kIconSize, kIconSize));
            }
        }
    }
}
