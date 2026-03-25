//
// Created by __author__ on 21/01/2022.
//

// You may need to build the project (run Qt uic code generator) to get "ui_MainPage.h" resolved

#include <algorithm>
#include <QFontMetrics>
#include <QGridLayout>
#include <QNetworkReply>
#include <QRect>
#include <QScreen>
#include <QScrollBar>
#include <QtCore/qjsonarray.h>

#include "Launcher.hpp"
#include "AppItem.hpp"

namespace fairwindsk::ui::launcher {
    namespace {
        const QString kLauncherButtonStyle = QStringLiteral(
            "QToolButton {"
            " background: transparent;"
            " color: #f9fafb;"
            " border: none;"
            " padding: 8px;"
            " }"
            "QToolButton:hover { background: rgba(255, 255, 255, 0.08); border-radius: 8px; }"
            "QToolButton:pressed { background: rgba(255, 255, 255, 0.14); border-radius: 8px; }");
    }

    Launcher::Launcher(QWidget *parent) : QWidget(parent), ui(new Ui::Launcher) {
        ui->setupUi(this);

        const auto configuration = fairwindsk::FairWindSK::getInstance()->getConfiguration();
        m_cols = std::max(1, configuration->getLauncherColumns());
        m_rows = std::max(1, configuration->getLauncherRows());
        m_iconSize = 256;

        m_layout = new QGridLayout(ui->scrollAreaWidgetContents);
        if (m_layout) {
            m_layout->setContentsMargins(0, 0, 0, 0);
            m_layout->setHorizontalSpacing(16);
            m_layout->setVerticalSpacing(16);
            ui->toolButton_Left->setStyleSheet(kLauncherButtonStyle);
            ui->toolButton_Right->setStyleSheet(kLauncherButtonStyle);

            ui->scrollAreaWidgetContents->setLayout(m_layout);
            ui->scrollArea->horizontalScrollBar()->setVisible(false);

            connect(ui->toolButton_Right, &QToolButton::clicked, this, &Launcher::onScrollRight);
            connect(ui->toolButton_Left, &QToolButton::clicked, this, &Launcher::onScrollLeft);
            connect(ui->scrollArea->horizontalScrollBar(), &QScrollBar::valueChanged, this, [this]() { updateScrollButtons(); });
            connect(ui->scrollArea->horizontalScrollBar(), &QScrollBar::rangeChanged, this, [this]() { updateScrollButtons(); });

            QMap<int, QPair<AppItem *, QString>> orderedApps;
            auto fairWindSK = fairwindsk::FairWindSK::getInstance();
            for (auto &hash : fairWindSK->getAppsHashes()) {
                auto app = fairWindSK->getAppItemByHash(hash);
                if (app->getActive()) {
                    orderedApps[app->getOrder()] = QPair<AppItem *, QString>(app, hash);
                }
            }

            const int itemsPerPage = std::max(1, m_rows * m_cols);
            int index = 0;
            for (const auto &item : orderedApps) {
                auto appItem = item.first;
                auto name = item.second;
                const int page = index / itemsPerPage;
                const int indexInPage = index % itemsPerPage;
                const int row = indexInPage / m_cols;
                const int col = (page * m_cols) + (indexInPage % m_cols);

                auto *button = new QToolButton(ui->scrollAreaWidgetContents);
                button->setProperty("app_hash", name);
                button->setProperty("base_point_size", button->font().pointSizeF());
                button->setText(appItem->getDisplayName());
                button->setToolTip(appItem->getDescription());

                const QPixmap pixmap = appItem->getIcon();
                if (!pixmap.isNull()) {
                    button->setIcon(pixmap);
                    button->setIconSize(QSize(m_iconSize, m_iconSize));
                }

                button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
                button->setAutoRaise(true);
                button->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
                button->setStyleSheet(kLauncherButtonStyle);
                connect(button, &QToolButton::released, this, &Launcher::toolButton_App_released);

                m_layout->addWidget(button, row, col, Qt::AlignCenter);
                m_buttons[name] = button;
                ++index;
            }

            resize();
            updateScrollButtons();
        }
    }

    Launcher::~Launcher() {
        delete ui;
        ui = nullptr;
    }

    void Launcher::toolButton_App_released() {
        auto *buttonWidget = qobject_cast<QToolButton *>(sender());
        if (!buttonWidget) {
            return;
        }

        const QString hash = buttonWidget->property("app_hash").toString();
        if (hash.isEmpty()) {
            return;
        }

        if (FairWindSK::getInstance()->isDebug()) {
            qDebug() << "Apps - hash:" << hash;
        }

        emit foregroundAppChanged(hash);
    }

    void Launcher::resizeEvent(QResizeEvent *event) {
        QWidget::resizeEvent(event);
        resize();
    }

    void Launcher::resize() {
        auto *layout = qobject_cast<QGridLayout *>(ui->scrollAreaWidgetContents->layout());
        if (!layout) {
            return;
        }

        const auto viewportSize = ui->scrollArea->viewport()->size();
        const int availableHeight = qMax(240, viewportSize.height() - layout->contentsMargins().top() - layout->contentsMargins().bottom());
        const int availableWidth = qMax(320, viewportSize.width() - layout->contentsMargins().left() - layout->contentsMargins().right());
        const int rowHeight = qMax(140, (availableHeight - ((m_rows - 1) * layout->verticalSpacing())) / qMax(1, m_rows));
        const int columnWidth = qMax(180, (availableWidth - ((m_cols - 1) * layout->horizontalSpacing())) / qMax(1, m_cols));
        const int totalColumns = std::max(1, layout->columnCount());

        for (int col = 0; col < totalColumns; ++col) {
            layout->setColumnMinimumWidth(col, columnWidth);
        }

        for (int row = 0; row < m_rows; ++row) {
            layout->setRowMinimumHeight(row, rowHeight);
        }

        for (auto button : m_buttons) {
            QFont font = button->font();
            const qreal basePointSize = button->property("base_point_size").toReal();
            if (basePointSize > 0.0) {
                font.setPointSizeF(basePointSize);
                button->setFont(font);
            }

            const QFontMetrics metrics(button->font());
            const QRect textRect(0, 0, qMax(80, columnWidth - 16), rowHeight);
            const int textHeight = qMax(
                metrics.lineSpacing(),
                metrics.boundingRect(textRect, Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap, button->text()).height());
            const int iconSize = qMax(72, qMin(columnWidth - 24, rowHeight - textHeight - 28));

            button->setIconSize(QSize(iconSize, iconSize));
            button->setFixedSize(columnWidth, rowHeight);
        }

        updateScrollButtons();
    }

    void Launcher::onScrollLeft() {
        ui->scrollArea->horizontalScrollBar()->setValue(
                ui->scrollArea->horizontalScrollBar()->value() - ui->scrollArea->viewport()->width());
    }

    void Launcher::onScrollRight() {
        ui->scrollArea->horizontalScrollBar()->setValue(
                ui->scrollArea->horizontalScrollBar()->value() + ui->scrollArea->viewport()->width());
    }

    void Launcher::updateScrollButtons() const {
        const auto *scrollBar = ui->scrollArea->horizontalScrollBar();
        const bool canScroll = scrollBar->maximum() > scrollBar->minimum();
        ui->toolButton_Left->setEnabled(canScroll && scrollBar->value() > scrollBar->minimum());
        ui->toolButton_Right->setEnabled(canScroll && scrollBar->value() < scrollBar->maximum());
    }

} // fairwindsk::ui::launcher
