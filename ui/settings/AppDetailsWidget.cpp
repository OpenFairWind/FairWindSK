//
// Created by Codex on 28/03/26.
//

#include "AppDetailsWidget.hpp"

#include <QApplication>
#include <QMouseEvent>
#include <QTouchEvent>
#include <QVBoxLayout>

#include "ui/widgets/TouchIconBrowser.hpp"
#include "ui_AppDetailsWidget.h"

namespace fairwindsk::ui::settings {
    AppDetailsWidget::AppDetailsWidget(QWidget *parent) : QWidget(parent), ui(new Ui::AppDetailsWidget) {
        ui->setupUi(this);

        auto *iconBrowserHostLayout = new QVBoxLayout(ui->widget_AppsIconBrowserHost);
        iconBrowserHostLayout->setContentsMargins(0, 0, 0, 0);
        iconBrowserHostLayout->setSpacing(0);
        m_iconBrowser = new fairwindsk::ui::widgets::TouchIconBrowser(ui->widget_AppsIconBrowserHost);
        iconBrowserHostLayout->addWidget(m_iconBrowser);

        hideIconPicker();

        connect(ui->pushButton_Apps_AppIcon_Browse, &QPushButton::clicked, this, &AppDetailsWidget::showIconPicker);
        if (m_iconBrowser) {
            connect(m_iconBrowser, &fairwindsk::ui::widgets::TouchIconBrowser::pathSelected, this, [this](const QString &path) {
                updateIconPreview(path);
            });
            connect(m_iconBrowser, &fairwindsk::ui::widgets::TouchIconBrowser::pathActivated, this, [this](const QString &path) {
                const QString iconPath = fairwindsk::ui::widgets::TouchIconBrowser::normalizedIconStoragePath(path);
                setAppIconPath(iconPath);
                hideIconPicker();
                emit iconPathSelected(iconPath);
            });
        }
    }

    AppDetailsWidget::~AppDetailsWidget() {
        delete ui;
        ui = nullptr;
    }

    void AppDetailsWidget::setAppIconPath(const QString &path) {
        m_currentIconPath = fairwindsk::ui::widgets::TouchIconBrowser::normalizedIconStoragePath(path.trimmed());
        if (m_iconBrowser) {
            m_iconBrowser->setCurrentPath(m_currentIconPath);
        }
        updateIconPreview(m_currentIconPath);
    }

    QString AppDetailsWidget::appIconPath() const {
        return m_currentIconPath;
    }

    void AppDetailsWidget::showIconPicker() {
        if (m_iconBrowser) {
            m_iconBrowser->setCurrentPath(m_currentIconPath);
        }
        ui->frame_AppsIconPicker->setVisible(true);
        if (m_iconBrowser) {
            m_iconBrowser->setFocus();
        }
        qApp->installEventFilter(this);
    }

    void AppDetailsWidget::hideIconPicker() {
        ui->frame_AppsIconPicker->setVisible(false);
        qApp->removeEventFilter(this);
    }

    bool AppDetailsWidget::eventFilter(QObject *watched, QEvent *event) {
        Q_UNUSED(watched);
        if (!ui->frame_AppsIconPicker->isVisible()) {
            return QWidget::eventFilter(watched, event);
        }

        QPoint globalPos;
        bool hasGlobalPos = false;
        if (event->type() == QEvent::MouseButtonPress) {
            if (const auto *mouseEvent = static_cast<QMouseEvent *>(event)) {
                globalPos = mouseEvent->globalPosition().toPoint();
                hasGlobalPos = true;
            }
        } else if (event->type() == QEvent::TouchBegin) {
            if (const auto *touchEvent = static_cast<QTouchEvent *>(event); !touchEvent->points().isEmpty()) {
                globalPos = touchEvent->points().constFirst().globalPosition().toPoint();
                hasGlobalPos = true;
            }
        }

        if (!hasGlobalPos) {
            return QWidget::eventFilter(watched, event);
        }

        const QRect pickerRect(ui->frame_AppsIconPicker->mapToGlobal(QPoint(0, 0)), ui->frame_AppsIconPicker->size());
        const QRect browseRect(ui->pushButton_Apps_AppIcon_Browse->mapToGlobal(QPoint(0, 0)), ui->pushButton_Apps_AppIcon_Browse->size());
        if (!pickerRect.contains(globalPos) && !browseRect.contains(globalPos)) {
            hideIconPicker();
        }

        return QWidget::eventFilter(watched, event);
    }

    void AppDetailsWidget::updateIconPreview(const QString &path) {
        ui->label_Apps_Icon->setPixmap(fairwindsk::ui::widgets::TouchIconBrowser::iconPixmapForPath(path, 128));
    }
}
