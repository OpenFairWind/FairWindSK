#include "AndroidApps.hpp"

#include <algorithm>
#include <QAbstractItemView>
#include <QLabel>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

#include "AppItem.hpp"
#include "FairWindSK.hpp"
#include "Settings.hpp"
#include "platform/android/AndroidApplicationBridge.hpp"
#include "ui/widgets/TouchCheckBox.hpp"

namespace fairwindsk::ui::settings {
    namespace {
        constexpr auto kLauncherLayoutKey = "launcherLayout";
        constexpr auto kLauncherNodesKey = "nodes";
        constexpr auto kNodeItemsKey = "items";
        constexpr auto kNodeChildrenKey = "children";
    }

    AndroidApps::AndroidApps(Settings *settings, QWidget *parent)
        : QWidget(parent), m_settings(settings) {
        // Build a spacious, scrollable selection surface suitable for helm touch displays.
        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(16, 16, 16, 16);
        layout->setSpacing(12);

        auto *titleLabel = new QLabel(tr("Android applications"), this);
        QFont titleFont = titleLabel->font();
        titleFont.setPointSizeF(std::max(18.0, titleFont.pointSizeF()));
        titleFont.setBold(true);
        titleLabel->setFont(titleFont);
        layout->addWidget(titleLabel);

        auto *descriptionLabel = new QLabel(
            tr("Select the installed Android applications that will be available in Settings > Applications and assignable to launcher pages."),
            this);
        descriptionLabel->setWordWrap(true);
        layout->addWidget(descriptionLabel);

        m_refreshButton = new QPushButton(tr("Refresh installed applications"), this);
        m_refreshButton->setMinimumHeight(48);
        layout->addWidget(m_refreshButton);

        m_statusLabel = new QLabel(this);
        m_statusLabel->setWordWrap(true);
        layout->addWidget(m_statusLabel);

        m_applicationsList = new QListWidget(this);
        m_applicationsList->setSpacing(6);
        m_applicationsList->setUniformItemSizes(false);
        m_applicationsList->setSelectionMode(QAbstractItemView::SingleSelection);
        m_applicationsList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        layout->addWidget(m_applicationsList, 1);

        connect(m_refreshButton, &QPushButton::clicked, this, &AndroidApps::refreshApplications);
        refreshApplications();
    }

    void AndroidApps::refreshApplications() {
        // Refresh package metadata explicitly so installs and removals are reflected on demand.
        m_discoveredApplications = platform::android::AndroidApplicationBridge::launchableApplications();
        rebuildList();
    }

    void AndroidApps::rebuildList() {
        m_rebuilding = true;
        m_applicationsList->clear();

        for (const auto &application : m_discoveredApplications) {
            if (!application.is_object() || !application.contains("name") || !application["name"].is_string()) {
                continue;
            }

            AppItem appItem(application);
            auto *item = new QListWidgetItem(m_applicationsList);
            item->setData(Qt::UserRole, QString::fromStdString(application.dump()));
            item->setToolTip(appItem.getDescription());
            item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

            // Keep selection and launch controls physically separate so neither obscures the app icon.
            auto *row = new QWidget(m_applicationsList);
            row->setMinimumHeight(68);
            auto *rowLayout = new QHBoxLayout(row);
            rowLayout->setContentsMargins(10, 4, 12, 4);
            rowLayout->setSpacing(14);

            auto *launchButton = new QToolButton(row);
            launchButton->setIcon(QIcon(appItem.getIcon(false)));
            launchButton->setIconSize(QSize(52, 52));
            launchButton->setFixedSize(60, 60);
            launchButton->setAutoRaise(true);
            launchButton->setToolTip(tr("Launch %1").arg(appItem.getDisplayName(false)));
            launchButton->setAccessibleName(tr("Launch %1").arg(appItem.getDisplayName(false)));
            rowLayout->addWidget(launchButton);

            auto *nameLabel = new QLabel(appItem.getDisplayName(false), row);
            nameLabel->setToolTip(appItem.getDescription());
            nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            rowLayout->addWidget(nameLabel, 1);

            auto *selectionCheckBox = new widgets::TouchCheckBox(row);
            selectionCheckBox->setText(tr("Available"));
            selectionCheckBox->setIconSize(QSize(40, 40));
            selectionCheckBox->setChecked(m_settings->getConfiguration()->findApp(appItem.getName()) != -1);
            selectionCheckBox->setToolTip(tr("Make %1 available in launcher pages").arg(appItem.getDisplayName(false)));
            rowLayout->addWidget(selectionCheckBox);
            m_applicationsList->setItemWidget(item, row);
            item->setSizeHint(QSize(row->sizeHint().width(), std::max(72, row->sizeHint().height())));

            connect(selectionCheckBox, &widgets::TouchCheckBox::toggled, this,
                    [this, application](const bool selected) {
                        if (!m_rebuilding) {
                            setApplicationSelected(application, selected);
                        }
                    });
            connect(launchButton, &QToolButton::clicked, this,
                    [this, application]() {
                        AppItem selectedApplication(application);
                        if (!platform::android::AndroidApplicationBridge::launchApplication(
                                selectedApplication.getAndroidPackage(),
                                selectedApplication.getAndroidActivity())) {
                            m_statusLabel->setText(tr("%1 could not be launched. It may have been removed or disabled.")
                                                       .arg(selectedApplication.getDisplayName(false)));
                        }
                    });
        }

        m_statusLabel->setText(m_applicationsList->count() == 0
                                   ? tr("No launchable Android applications were found.")
                                   : tr("%n launchable Android application(s) found.", nullptr, m_applicationsList->count()));
        m_rebuilding = false;
    }

    void AndroidApps::setApplicationSelected(const nlohmann::json &application, const bool selected) {
        AppItem appItem(application);
        const QString appName = appItem.getName();
        auto &root = m_settings->getConfiguration()->getRoot();
        if (!root.contains("apps") || !root["apps"].is_array()) {
            root["apps"] = nlohmann::json::array();
        }

        const int existingIndex = m_settings->getConfiguration()->findApp(appName);
        if (selected && existingIndex == -1) {
            root["apps"].push_back(application);
        } else if (!selected && existingIndex != -1) {
            root["apps"].erase(root["apps"].begin() + existingIndex);
            removeApplicationFromLauncherPages(appName);
        } else {
            return;
        }

        // Rebuild the shared runtime registry so the launcher changes without restarting FairWindSK.
        m_settings->markDirty(FairWindSK::RuntimeApps, 0);
    }

    void AndroidApps::removeApplicationFromLauncherPages(const QString &appName) {
        auto &root = m_settings->getConfiguration()->getRoot();
        if (!root.contains(kLauncherLayoutKey) || !root[kLauncherLayoutKey].is_object()) {
            return;
        }
        auto &layout = root[kLauncherLayoutKey];
        if (layout.contains(kLauncherNodesKey)) {
            removeApplicationFromNodes(layout[kLauncherNodesKey], appName);
        }
    }

    void AndroidApps::removeApplicationFromNodes(nlohmann::json &nodes, const QString &appName) {
        if (!nodes.is_array()) {
            return;
        }

        for (auto &node : nodes) {
            if (!node.is_object()) {
                continue;
            }
            if (node.contains(kNodeItemsKey) && node[kNodeItemsKey].is_array()) {
                for (auto &entry : node[kNodeItemsKey]) {
                    if (entry.is_string() && QString::fromStdString(entry.get<std::string>()) == appName) {
                        entry = "";
                    }
                }
            }
            if (node.contains(kNodeChildrenKey)) {
                removeApplicationFromNodes(node[kNodeChildrenKey], appName);
            }
        }
    }

} // fairwindsk::ui::settings
