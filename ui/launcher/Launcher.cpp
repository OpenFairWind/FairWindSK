//
// Created by __author__ on 21/01/2022.
//

// You may need to build the project (run Qt uic code generator) to get "ui_MainPage.h" resolved

#include <algorithm>
#include <functional>
#include <QEnterEvent>
#include <QFrame>
#include <QGridLayout>
#include <QList>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QScrollBar>
#include <QStringList>
#include <QTimer>

#include "Launcher.hpp"
#include "AppItem.hpp"

namespace fairwindsk::ui::launcher {
    namespace {
        using OrderedApp = QPair<AppItem *, QString>;

        QList<OrderedApp> collectOrderedApps() {
            QList<OrderedApp> orderedApps;
            const auto fairWindSK = fairwindsk::FairWindSK::getInstance();
            for (const auto &hash : fairWindSK->getAppsHashes()) {
                auto *app = fairWindSK->getAppItemByHash(hash);
                if (app && app->getActive()) {
                    orderedApps.append(OrderedApp(app, hash));
                }
            }

            std::sort(orderedApps.begin(), orderedApps.end(), [](const auto &left, const auto &right) {
                if (left.first->getOrder() != right.first->getOrder()) {
                    return left.first->getOrder() < right.first->getOrder();
                }
                const int displayNameCompare = QString::compare(left.first->getDisplayName(),
                                                                right.first->getDisplayName(),
                                                                Qt::CaseInsensitive);
                if (displayNameCompare != 0) {
                    return displayNameCompare < 0;
                }
                return QString::compare(left.second, right.second, Qt::CaseInsensitive) < 0;
            });

            return orderedApps;
        }

        class AppTile final : public QFrame {
        public:
            explicit AppTile(QWidget *parent = nullptr) : QFrame(parent) {
                setCursor(Qt::PointingHandCursor);
                setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
            }

            void setAppData(const QString &hash,
                            const QString &title,
                            const QString &description,
                            const QPixmap &pixmap) {
                m_hash = hash;
                m_title = title;
                m_pixmap = pixmap;
                setToolTip(description);
                update();
            }

            void setBasePointSize(const qreal pointSize) {
                m_basePointSize = pointSize;
            }

            void setActivateHandler(std::function<void(const QString &)> handler) {
                m_onActivated = std::move(handler);
            }

            qreal basePointSize() const {
                return m_basePointSize;
            }

            QString appHash() const {
                return m_hash;
            }

        protected:
            void enterEvent(QEnterEvent *event) override {
                QFrame::enterEvent(event);
                m_hovered = true;
                update();
            }

            void leaveEvent(QEvent *event) override {
                QFrame::leaveEvent(event);
                m_hovered = false;
                update();
            }

            void mouseReleaseEvent(QMouseEvent *event) override {
                QFrame::mouseReleaseEvent(event);
                if (event->button() == Qt::LeftButton && rect().contains(event->position().toPoint()) && m_onActivated) {
                    m_onActivated(m_hash);
                }
            }

            void paintEvent(QPaintEvent *event) override {
                Q_UNUSED(event);

                QPainter painter(this);
                painter.setRenderHint(QPainter::Antialiasing, true);
                painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

                const QRectF tileRect = rect().adjusted(1, 1, -1, -1);
                const qreal radius = 2.0;

                QPainterPath clipPath;
                clipPath.addRoundedRect(tileRect, radius, radius);
                painter.setClipPath(clipPath);

                painter.fillRect(tileRect, QColor(16, 22, 32));
                if (!m_pixmap.isNull()) {
                    const QPixmap scaled = m_pixmap.scaled(tileRect.size().toSize(),
                                                           Qt::KeepAspectRatioByExpanding,
                                                           Qt::SmoothTransformation);
                    const QRect sourceRect((scaled.width() - int(tileRect.width())) / 2,
                                           (scaled.height() - int(tileRect.height())) / 2,
                                           int(tileRect.width()),
                                           int(tileRect.height()));
                    painter.drawPixmap(tileRect.toRect(), scaled, sourceRect);
                }

                QLinearGradient overlay(tileRect.topLeft(), QPointF(tileRect.left(), tileRect.bottom()));
                overlay.setColorAt(0.0, QColor(0, 0, 0, 0));
                overlay.setColorAt(0.55, QColor(0, 0, 0, 10));
                overlay.setColorAt(1.0, QColor(0, 0, 0, 170));
                painter.fillRect(tileRect, overlay);

                painter.setClipping(false);
                painter.setPen(QPen(m_hovered ? QColor(255, 255, 255) : QColor(230, 231, 235), m_hovered ? 2.0 : 1.0));
                painter.drawRoundedRect(tileRect, radius, radius);

                QFont titleFont = font();
                if (m_basePointSize > 0.0) {
                    titleFont.setPointSizeF(m_basePointSize);
                }
                painter.setFont(titleFont);
                painter.setPen(QColor(248, 250, 252));
                painter.drawText(tileRect.adjusted(10, 10, -10, -10).toRect(),
                                 Qt::AlignLeft | Qt::AlignBottom | Qt::TextWordWrap,
                                 m_title);
            }

        private:
            QString m_hash;
            QString m_title;
            QPixmap m_pixmap;
            qreal m_basePointSize = 0.0;
            bool m_hovered = false;
            std::function<void(const QString &)> m_onActivated;
        };

        const QString kNavigationButtonStyle = QStringLiteral(
            "QToolButton {"
            " background: transparent;"
            " color: #f9fafb;"
            " border: none;"
            " padding: 8px;"
            " }"
            "QToolButton:hover { background: rgba(255, 255, 255, 0.08); border-radius: 8px; }"
            "QToolButton:pressed { background: rgba(255, 255, 255, 0.14); border-radius: 8px; }");
        const QString kLauncherFrameStyle = QStringLiteral(
            "QScrollArea {"
            " background: transparent;"
            " border: 4px solid rgba(245, 245, 245, 0.95);"
            " border-radius: 3px;"
            " }");
    }

    Launcher::Launcher(QWidget *parent) : QWidget(parent), ui(new Ui::Launcher) {
        ui->setupUi(this);

        m_layout = new QGridLayout(ui->scrollAreaWidgetContents);
        if (m_layout) {
            m_layout->setContentsMargins(6, 6, 6, 6);
            m_layout->setHorizontalSpacing(4);
            m_layout->setVerticalSpacing(4);
            ui->toolButton_Left->setStyleSheet(kNavigationButtonStyle);
            ui->toolButton_Right->setStyleSheet(kNavigationButtonStyle);
            ui->toolButton_Left->setAutoRaise(true);
            ui->toolButton_Right->setAutoRaise(true);
            ui->scrollArea->setFrameShape(QFrame::NoFrame);
            ui->scrollArea->setStyleSheet(kLauncherFrameStyle);

            ui->scrollAreaWidgetContents->setLayout(m_layout);
            ui->scrollArea->setWidgetResizable(false);
            ui->scrollArea->horizontalScrollBar()->setVisible(false);
            ui->scrollArea->viewport()->installEventFilter(this);

            connect(ui->toolButton_Right, &QToolButton::clicked, this, &Launcher::onScrollRight);
            connect(ui->toolButton_Left, &QToolButton::clicked, this, &Launcher::onScrollLeft);
            connect(ui->scrollArea->horizontalScrollBar(), &QScrollBar::valueChanged, this, [this]() {
                m_targetPage = currentPage();
                updateScrollButtons();
            });
            connect(ui->scrollArea->horizontalScrollBar(), &QScrollBar::rangeChanged, this, [this]() { updateScrollButtons(); });
            refreshFromConfiguration();
        }
    }

    Launcher::~Launcher() {
        delete ui;
        ui = nullptr;
    }

    void Launcher::resizeEvent(QResizeEvent *event) {
        QWidget::resizeEvent(event);
        resize();
    }

    void Launcher::showEvent(QShowEvent *event) {
        QWidget::showEvent(event);
        resize();
    }

    void Launcher::refreshFromConfiguration(const bool forceRebuild) {
        const auto configuration = fairwindsk::FairWindSK::getInstance()->getConfiguration();
        const int newColumns = std::max(1, configuration->getLauncherColumns());
        const int newRows = std::max(1, configuration->getLauncherRows());
        const QString newLayoutSignature = buildLayoutSignature();
        const bool geometryChanged = m_cols != newColumns || m_rows != newRows;
        const bool contentChanged = m_layoutSignature != newLayoutSignature;

        m_targetPage = qBound(0, currentPage(), std::max(0, m_pageCount - 1));
        m_cols = newColumns;
        m_rows = newRows;

        if (forceRebuild || geometryChanged || contentChanged || m_tiles.isEmpty()) {
            if (geometryChanged) {
                m_stableViewportHeight = 0;
            }
            rebuildTiles();
            m_layoutSignature = newLayoutSignature;
        }

        QTimer::singleShot(0, this, [this]() { resize(); });
        updateScrollButtons();
    }

    bool Launcher::eventFilter(QObject *watched, QEvent *event) {
        if (watched == ui->scrollArea->viewport() && event && event->type() == QEvent::Resize) {
            resize();
        }

        return QWidget::eventFilter(watched, event);
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
        const int availableHeight = qMax(220, stableViewportHeight - layout->contentsMargins().top() - layout->contentsMargins().bottom());
        const int availableWidth = qMax(320, viewportSize.width() - layout->contentsMargins().left() - layout->contentsMargins().right());
        const int rowHeight = qMax(110, (availableHeight - ((m_rows - 1) * layout->verticalSpacing())) / qMax(1, m_rows));
        const int columnWidth = qMax(140, (availableWidth - ((m_cols - 1) * layout->horizontalSpacing())) / qMax(1, m_cols));
        const int totalColumns = std::max(1, m_pageCount * m_cols);
        const int contentWidth = std::max(viewportSize.width(),
                                          layout->contentsMargins().left() +
                                          layout->contentsMargins().right() +
                                          (totalColumns * columnWidth) +
                                          (std::max(0, totalColumns - 1) * layout->horizontalSpacing()));
        const int contentHeight = std::max(viewportSize.height(),
                                           layout->contentsMargins().top() +
                                           layout->contentsMargins().bottom() +
                                           (m_rows * rowHeight) +
                                           (std::max(0, m_rows - 1) * layout->verticalSpacing()));

        for (int col = 0; col < totalColumns; ++col) {
            layout->setColumnMinimumWidth(col, columnWidth);
        }

        for (int row = 0; row < m_rows; ++row) {
            layout->setRowMinimumHeight(row, rowHeight);
        }

        for (auto *tileWidget : m_tiles) {
            auto *tile = dynamic_cast<AppTile *>(tileWidget);
            if (!tile) {
                continue;
            }

            QFont font = tile->font();
            const qreal basePointSize = tile->basePointSize();
            if (basePointSize > 0.0) {
                font.setPointSizeF(std::max<qreal>(10.0, basePointSize));
                tile->setFont(font);
            }
            tile->setFixedSize(columnWidth, rowHeight);
        }

        ui->scrollAreaWidgetContents->resize(contentWidth, contentHeight);
        const auto *scrollBar = ui->scrollArea->horizontalScrollBar();
        const int targetValue = qBound(scrollBar->minimum(),
                                       m_targetPage * pageWidth(),
                                       scrollBar->maximum());
        if (scrollBar->value() != targetValue) {
            ui->scrollArea->horizontalScrollBar()->setValue(targetValue);
        }
        updateScrollButtons();
    }

    void Launcher::onScrollLeft() {
        m_targetPage = std::max(0, currentPage() - 1);
        ui->scrollArea->horizontalScrollBar()->setValue(m_targetPage * pageWidth());
    }

    void Launcher::onScrollRight() {
        m_targetPage = std::min(m_pageCount - 1, currentPage() + 1);
        ui->scrollArea->horizontalScrollBar()->setValue(m_targetPage * pageWidth());
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

    QString Launcher::buildLayoutSignature() const {
        QStringList parts;
        const auto orderedApps = collectOrderedApps();
        parts.reserve(orderedApps.size());

        for (const auto &item : orderedApps) {
            auto *appItem = item.first;
            parts.append(QStringLiteral("%1|%2|%3|%4|%5")
                             .arg(item.second,
                                  QString::number(appItem->getOrder()),
                                  appItem->getDisplayName(),
                                  appItem->getDescription(),
                                  QString::number(appItem->getIcon().cacheKey())));
        }

        return parts.join(QStringLiteral("||"));
    }

    void Launcher::rebuildTiles() {
        if (!m_layout) {
            return;
        }

        const int preservedPage = qBound(0, m_targetPage, std::max(0, m_pageCount - 1));
        while (m_layout->count() > 0) {
            auto *item = m_layout->takeAt(0);
            if (auto *widget = item->widget()) {
                delete widget;
            }
            delete item;
        }
        m_tiles.clear();

        const auto orderedApps = collectOrderedApps();
        const int itemsPerPage = std::max(1, m_rows * m_cols);
        m_pageCount = std::max(1, int((orderedApps.size() + itemsPerPage - 1) / itemsPerPage));
        m_targetPage = qBound(0, preservedPage, std::max(0, m_pageCount - 1));
        int index = 0;
        for (const auto &item : orderedApps) {
            auto *appItem = item.first;
            const auto &name = item.second;
            const int page = index / itemsPerPage;
            const int indexInPage = index % itemsPerPage;
            const int row = indexInPage / m_cols;
            const int col = (page * m_cols) + (indexInPage % m_cols);

            auto *tile = new AppTile(ui->scrollAreaWidgetContents);
            tile->setBasePointSize(tile->font().pointSizeF() + 1.0);
            tile->setAppData(name, appItem->getDisplayName(), appItem->getDescription(), appItem->getIcon());
            tile->setActivateHandler([this](const QString &hash) {
                if (hash.isEmpty()) {
                    return;
                }

                if (FairWindSK::getInstance()->isDebug()) {
                    qDebug() << "Apps - hash:" << hash;
                }

                emit foregroundAppChanged(hash);
            });

            m_layout->addWidget(tile, row, col);
            m_tiles[name] = tile;
            ++index;
        }
    }

} // fairwindsk::ui::launcher
