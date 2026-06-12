//
// Created by Codex on 28/03/26.
//

#include "AppDetailsWidget.hpp"

#include "ui/DrawerDialogHost.hpp"
#include "ui/widgets/TouchIconBrowser.hpp"
#include "ui_AppDetailsWidget.h"

namespace fairwindsk::ui::settings {
    AppDetailsWidget::AppDetailsWidget(QWidget *parent) : QWidget(parent), ui(new Ui::AppDetailsWidget) {
        ui->setupUi(this);
    }

    AppDetailsWidget::~AppDetailsWidget() {
        delete ui;
        ui = nullptr;
    }

    void AppDetailsWidget::setAppIconPath(const QString &path) {
        m_currentIconPath = fairwindsk::ui::widgets::TouchIconBrowser::normalizedIconStoragePath(path.trimmed());
        updateIconPreview(m_currentIconPath);
    }

    QString AppDetailsWidget::appIconPath() const {
        return m_currentIconPath;
    }

    void AppDetailsWidget::showIconPicker() {
        const QString iconPath = fairwindsk::ui::drawer::getIconPath(this, tr("Choose Icon"), m_currentIconPath);
        if (!iconPath.isEmpty()) {
            setAppIconPath(iconPath);
            emit iconPathSelected(iconPath);
        }
    }

    void AppDetailsWidget::hideIconPicker() {
        // Icon picker is now a drawer — nothing inline to hide.
    }

    void AppDetailsWidget::updateIconPreview(const QString &path) {
        ui->label_Apps_Icon->setPixmap(fairwindsk::ui::widgets::TouchIconBrowser::iconPixmapForPath(path, 128));
    }
}
