//
// Created by Codex on 16/04/26.
//

#include "Regions.hpp"

namespace fairwindsk::ui::mydata {

    Regions::Regions(QWidget *parent)
        : ResourceTab(ResourceKind::Region, parent) {
        refreshWorkflowTexts();
    }

    QString Regions::searchPlaceholderText() const {
        return tr("Search regions by area name or operational note");
    }

    QString Regions::namePlaceholderText() const {
        return tr("Region name");
    }

    QString Regions::descriptionPlaceholderText() const {
        return tr("Anchorage, exclusion zone, race box, or harbor working note");
    }

    QString Regions::geometryPlaceholderText() const {
        return tr("{\n  \"type\": \"Polygon\",\n  \"coordinates\": [[[12.4000, 41.9000], [12.5200, 41.9000], [12.5200, 42.0200], [12.4000, 41.9000]]]\n}");
    }

    QString Regions::importButtonText() const {
        return tr("Import region");
    }

    QString Regions::exportButtonText() const {
        return tr("Export region");
    }

    QString Regions::primaryRowActionToolTip() const {
        return tr("Inspect region geometry");
    }

    QIcon Regions::primaryRowActionIcon() const {
        return QIcon(QStringLiteral(":/resources/svg/OpenBridge/search.svg"));
    }
}
