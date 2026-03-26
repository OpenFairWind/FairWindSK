//
// Created by __author__ on 21/01/2022.
//

// You may need to build the project (run Qt uic code generator) to get "ui_MainPage.h" resolved

#include <algorithm>
#include <QFontMetrics>
#include <QFrame>
#include <QGridLayout>
#include <QList>
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

        m_layout = new QGridLayout(ui->scrollAreaWidgetContents);
        if (m_layout) {
            m_layout->setContentsMargins(0, 0, 0, 0);
            m_layout->setHorizontalSpacing(16);
            m_layout->setVerticalSpacing(16);
            ui->toolButton_Left->setStyleSheet(kLauncherButtonStyle);
            ui->toolButton_Right->setStyleSheet(kLauncherButtonStyle);
            ui->toolButton_Left->setAutoRaise(true);
            ui->toolButton_Right->setAutoRaise(true);
            ui->scrollArea->setFrameShape(QFrame::NoFrame);

            ui->scrollAreaWidgetContents->setLayout(m_layout);
            ui->scrollArea->setWidgetResizable(false);
            ui->scrollArea->horizontalScrollBar()->setVisible(false);

            connect(ui->toolButton_Right, &QToolButton::clicked, this, &Launcher::onScrollRight);
            connect(ui->toolButton_Left, &QToolButton::clicked, this, &Launcher::onScrollLeft);
            connect(ui->scrollArea->horizontalScrollBar(), &QScrollBar::valueChanged, this, [this]() { updateScrollButtons(); });
            connect(ui->scrollArea->horizontalScrollBar(), &QScrollBar::rangeChanged, this, [this]() { updateScrollButtons(); });

            QList<QPair<AppItem *, QString>> orderedApps;
            auto fairWindSK = fairwindsk::FairWindSK::getInstance();
            for (auto &hash : fairWindSK->getAppsHashes()) {
                auto app = fairWindSK->getAppItemByHash(hash);
                if (app->getActive()) {
                    orderedApps.append(QPair<AppItem *, QString>(app, hash));
                }
            }
            std::sort(orderedApps.begin(), orderedApps.end(), [](const auto &left, const auto &right) {
                if (left.first->getOrder() != right.first->getOrder()) {
                    return left.first->getOrder() < right.first->getOrder();
                }
                const int displayNameCompare = QString::compare(left.first->getDisplayName(), right.first->getDisplayName(), Qt::CaseInsensitive);
                if (displayNameCompare != 0) {
                    return displayNameCompare < 0;
                }
                return QString::compare(left.second, right.second, Qt::CaseInsensitive) < 0;
            });

            const int itemsPerPage = std::max(1, m_rows * m_cols);
            m_pageCount = std::max(1, int((orderedApps.size() + itemsPerPage - 1) / itemsPerPage));
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
        if (viewportSize.width() <= 0 || viewportSize.height() <= 0) {
            return;
        }
        m_stableViewportHeight = qMax(m_stableViewportHeight, viewportSize.height());
        const int stableViewportHeight = qMax(viewportSize.height(), m_stableViewportHeight);
        const int availableHeight = qMax(240, stableViewportHeight - layout->contentsMargins().top() - layout->contentsMargins().bottom());
        const int availableWidth = qMax(320, viewportSize.width() - layout->contentsMargins().left() - layout->contentsMargins().right());
        const int rowHeight = qMax(140, (availableHeight - ((m_rows - 1) * layout->verticalSpacing())) / qMax(1, m_rows));
        const int columnWidth = qMax(180, (availableWidth - ((m_cols - 1) * layout->horizontalSpacing())) / qMax(1, m_cols));
        const int totalColumns = std::max(1, layout->columnCount());
        const int contentWidth = std::max(viewportSize.width(),
                                          (m_pageCount * availableWidth) +
                                          ((std::max(0, totalColumns - 1)) * layout->horizontalSpacing()) +
                                          layout->contentsMargins().left() +
                                          layout->contentsMargins().right());
        const int contentHeight = std::max(viewportSize.height(),
                                           availableHeight +
                                           layout->contentsMargins().top() +
                                           layout->contentsMargins().bottom());

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
            const int iconWidth = qMax(72, columnWidth - 24);
            const int iconHeight = qMax(72, rowHeight - textHeight - 28);

            button->setIconSize(QSize(iconWidth, iconHeight));
            button->setFixedSize(columnWidth, rowHeight);
        }

        ui->scrollAreaWidgetContents->resize(contentWidth, contentHeight);
        updateScrollButtons();
    }

    void Launcher::onScrollLeft() {
        ui->scrollArea->horizontalScrollBar()->setValue((currentPage() - 1) * pageWidth());
    }

    void Launcher::onScrollRight() {
        const auto nextPage = std::min(m_pageCount - 1, currentPage() + 1);
        ui->scrollArea->horizontalScrollBar()->setValue(nextPage * pageWidth());
    }

    void Launcher::updateScrollButtons() const {
        const auto *scrollBar = ui->scrollArea->horizontalScrollBar();
        const bool canScroll = scrollBar->maximum() > scrollBar->minimum();
        ui->toolButton_Left->setEnabled(canScroll && scrollBar->value() > scrollBar->minimum());
        ui->toolButton_Right->setEnabled(canScroll && scrollBar->value() < scrollBar->maximum());
    }

    int Launcher::pageWidth() const {
        return qMax(1, ui->scrollArea->viewport()->width());
    }

    int Launcher::currentPage() const {
        const auto *scrollBar = ui->scrollArea->horizontalScrollBar();
        return qBound(0, (scrollBar->value() + (pageWidth() / 2)) / pageWidth(), std::max(0, m_pageCount - 1));
    }

} // fairwindsk::ui::launcher
