//
// Created by Codex on 28/03/26.
//

#include "PageDetailsWidget.hpp"

#include <QVBoxLayout>

#include "ui/widgets/TouchIconBrowser.hpp"
#include "ui_PageDetailsWidget.h"

namespace fairwindsk::ui::settings {
    PageDetailsWidget::PageDetailsWidget(QWidget *parent) : QWidget(parent), ui(new Ui::PageDetailsWidget) {
        ui->setupUi(this);

        auto *iconBrowserHostLayout = new QVBoxLayout(ui->widget_PageIconBrowserHost);
        iconBrowserHostLayout->setContentsMargins(0, 0, 0, 0);
        iconBrowserHostLayout->setSpacing(0);
        m_iconBrowser = new fairwindsk::ui::widgets::TouchIconBrowser(ui->widget_PageIconBrowserHost);
        iconBrowserHostLayout->addWidget(m_iconBrowser);

        hideIconPicker();

        connect(ui->pushButton_Page_Icon_Browse, &QPushButton::clicked, this, &PageDetailsWidget::showIconPicker);
        if (m_iconBrowser) {
            connect(m_iconBrowser, &fairwindsk::ui::widgets::TouchIconBrowser::pathSelected, this, [this](const QString &path) {
                updateIconPreview(path);
            });
            connect(m_iconBrowser, &fairwindsk::ui::widgets::TouchIconBrowser::canceled, this, &PageDetailsWidget::hideIconPicker);
            connect(m_iconBrowser, &fairwindsk::ui::widgets::TouchIconBrowser::pathActivated, this, [this](const QString &path) {
                const QString iconPath = fairwindsk::ui::widgets::TouchIconBrowser::normalizedIconStoragePath(path);
                setPageIconPath(iconPath);
                hideIconPicker();
                emit iconPathSelected(iconPath);
            });
        }
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
        if (m_iconBrowser) {
            m_iconBrowser->setCurrentPath(m_currentIconPath);
        }
        updateIconPreview(m_currentIconPath);
    }

    QString PageDetailsWidget::pageIconPath() const {
        return m_currentIconPath;
    }

    void PageDetailsWidget::showIconPicker() {
        if (m_iconBrowser) {
            m_iconBrowser->setCurrentPath(m_currentIconPath);
        }
        ui->frame_PageIconPicker->setVisible(true);
        if (m_iconBrowser) {
            m_iconBrowser->setFocus();
        }
    }

    void PageDetailsWidget::hideIconPicker() {
        ui->frame_PageIconPicker->setVisible(false);
    }

    void PageDetailsWidget::applySelectedIcon() {
        if (!m_iconBrowser) {
            hideIconPicker();
            return;
        }

        const QString iconPath = fairwindsk::ui::widgets::TouchIconBrowser::normalizedIconStoragePath(m_iconBrowser->selectedPath());
        if (iconPath.isEmpty()) {
            hideIconPicker();
            return;
        }

        setPageIconPath(iconPath);
        hideIconPicker();
        emit iconPathSelected(iconPath);
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
