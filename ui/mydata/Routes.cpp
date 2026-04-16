//
// Created by Codex on 16/04/26.
//

#include "Routes.hpp"

namespace fairwindsk::ui::mydata {

    Routes::Routes(QWidget *parent)
        : ResourceTab(ResourceKind::Route, parent) {
        refreshWorkflowTexts();
    }

    QString Routes::searchPlaceholderText() const {
        return tr("Search routes by name, description, or waypoint leg");
    }

    QString Routes::namePlaceholderText() const {
        return tr("Route name");
    }

    QString Routes::descriptionPlaceholderText() const {
        return tr("Passage notes, ETA strategy, or watch handover details");
    }

    QString Routes::typePlaceholderText() const {
        return tr("route, sail plan, delivery");
    }

    QString Routes::coordinatesPlaceholderText() const {
        return tr("41.9028, 12.4964\n41.9031, 12.4970\n41.9038, 12.4976");
    }

    QString Routes::importButtonText() const {
        return tr("Import route");
    }

    QString Routes::exportButtonText() const {
        return tr("Export route");
    }

    QString Routes::primaryRowActionToolTip() const {
        return tr("Preview route geometry");
    }

    QIcon Routes::primaryRowActionIcon() const {
        return QIcon(QStringLiteral(":/resources/svg/OpenBridge/navigation-route.svg"));
    }
}
