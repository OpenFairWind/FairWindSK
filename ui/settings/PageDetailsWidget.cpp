//
// Created by Codex on 28/03/26.
//

#include "PageDetailsWidget.hpp"

#include "ui/DrawerDialogHost.hpp"
#include "ui/widgets/TouchIconBrowser.hpp"
#include "ui_PageDetailsWidget.h"

namespace fairwindsk::ui::settings {
    PageDetailsWidget::PageDetailsWidget(QWidget *parent) : QWidget(parent), ui(new Ui::PageDetailsWidget) {
        ui->setupUi(this);
        connect(ui->pushButton_Page_Icon_Browse, &QPushButton::clicked, this, &PageDetailsWidget::showIconPicker);
    }

    PageDetailsWidget::~PageDetailsWidget() {
        delete ui;
        ui = nullptr;
    }

    void PageDetailsWidget::setPageName(const QString &name) const {
        ui->lineEdit_Page_Name->setText(name);
    }

    QString PageDetailsWidget::pageName() const {
        return ui->lineEdit_Page_Name->text().trimmed();
    }

    void PageDetailsWidget::setPageIconPath(const QString &path) {
        m_currentIconPath = fairwindsk::ui::widgets::TouchIconBrowser::normalizedIconStoragePath(path.trimmed());
        updateIconPreview(m_currentIconPath);
    }

    QString PageDetailsWidget::pageIconPath() const {
        return m_currentIconPath;
    }

    void PageDetailsWidget::showIconPicker() {
        const QString iconPath = fairwindsk::ui::drawer::getIconPath(this, tr("Choose Icon"), m_currentIconPath);
        if (!iconPath.isEmpty()) {
            setPageIconPath(iconPath);
            emit iconPathSelected(iconPath);
        }
    }

    void PageDetailsWidget::hideIconPicker() {
        // Icon picker is now a drawer — nothing inline to hide.
    }

    void PageDetailsWidget::updateIconPreview(const QString &path) {
        ui->label_Page_Icon->setPixmap(fairwindsk::ui::widgets::TouchIconBrowser::iconPixmapForPath(path, 128));
    }

    QString PageDetailsWidget::normalizedIconStoragePath(const QString &path) {
        return fairwindsk::ui::widgets::TouchIconBrowser::normalizedIconStoragePath(path);
    }

    QString PageDetailsWidget::resolvedLocalIconPath(const QString &path) {
        return fairwindsk::ui::widgets::TouchIconBrowser::resolvedLocalIconPath(path);
    }

    QPixmap PageDetailsWidget::iconPixmapForPath(const QString &path, const int iconSize) {
        return fairwindsk::ui::widgets::TouchIconBrowser::iconPixmapForPath(path, iconSize);
    }
}
