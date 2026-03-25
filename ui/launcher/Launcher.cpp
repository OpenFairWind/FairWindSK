//
// Created by __author__ on 21/01/2022.
//

// You may need to build the project (run Qt uic code generator) to get "ui_MainPage.h" resolved

#include <algorithm>
#include <cmath>
#include <QScreen>
#include <QRect>
#include <QScrollBar>
#include <QGridLayout>
#include <QNetworkReply>
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

        m_cols = 4;
        m_rows = 2;
        m_iconSize = 256;

        // Create a new grid layout
        m_layout = new QGridLayout(ui->scrollAreaWidgetContents);
        if (m_layout) {
            m_layout->setContentsMargins(0,0,0,0);
            m_layout->setHorizontalSpacing(16);
            m_layout->setVerticalSpacing(16);
            ui->toolButton_Left->setStyleSheet(kLauncherButtonStyle);
            ui->toolButton_Right->setStyleSheet(kLauncherButtonStyle);


            // Set the UI scroll area with the newly created layout
            ui->scrollAreaWidgetContents->setLayout(m_layout);
            ui->scrollArea->horizontalScrollBar()->setVisible(false);


            // Right scroll
            connect(ui->toolButton_Right, &QToolButton::clicked, this, &Launcher::onScrollRight);

            // Left scroll
            connect(ui->toolButton_Left, &QToolButton::clicked, this, &Launcher::onScrollLeft);
            connect(ui->scrollArea->horizontalScrollBar(), &QScrollBar::valueChanged, this, [this]() {
                updateButtonScales();
                updateScrollButtons();
            });
            connect(ui->scrollArea->horizontalScrollBar(), &QScrollBar::rangeChanged, this, [this]() {
                updateButtonScales();
                updateScrollButtons();
            });

            // Order by order value
            QMap<int, QPair<AppItem *, QString>> map;
            int row = 0, col = 0;

            // Get the FairWind singleton
            auto fairWindSK = fairwindsk::FairWindSK::getInstance();

            //qDebug() << "appsHashes: " << fairWindSK->getAppsHashes();

            // Populate the inverted list
            for (auto &hash : fairWindSK->getAppsHashes()) {
                // Get the hash value
                auto app = fairWindSK->getAppItemByHash(hash);
                auto position = app->getOrder();

                //qDebug() << "---------AppItem: "  << app->getName() << " IsActive: " << app->getActive() << " order: " << app->getOrder();

                // Check if the app is active
                if (app->getActive()) {
                    map[position] = QPair<AppItem *, QString>(app, hash);
                }

            }

            m_cols = qMax(1, (map.size() + m_rows - 1) / m_rows);

            // Iterate on the available apps' hash values
            for (const auto& item: map) {
                // Get the hash value
                auto appItem = item.first;
                auto name = item.second;

                // Create a new button
                auto *button = new QToolButton(ui->scrollAreaWidgetContents);

                button->setProperty("app_hash", name);
                button->setProperty("launcher_column", col);
                button->setProperty("base_point_size", button->font().pointSizeF());

                // Set the app's name as the button's text
                button->setText(appItem->getDisplayName());

                // Set the tool tip
                button->setToolTip(appItem->getDescription());

                // Get the application icon
                QPixmap pixmap = appItem->getIcon();

                // Check if the icon is available
                if (!pixmap.isNull()) {
                    // Set the app's icon as the button's icon
                    button->setIcon(pixmap);

                    // Give the button's icon a fixed square
                    button->setIconSize(QSize(m_iconSize, m_iconSize));
                }

                // Set the button's style to have an icon and some text beneath it
                button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
                button->setAutoRaise(true);
                button->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
                button->setStyleSheet(kLauncherButtonStyle);

                // Launch the app when the button is clicked
                connect(button, &QToolButton::released, this, &Launcher::toolButton_App_released);

                // Add the newly created button to the grid layout as a widget
                m_layout->addWidget(button, row, col, Qt::AlignCenter);

                // Store in buttons in a map
                m_buttons[name] = button;

                row++;
                if (row == m_rows) {
                    row = 0;
                    col++;
                }

                // qDebug()<< name;
            }

            // Resize the icons
            resize();
            updateScrollButtons();
        }
    }

    Launcher::~Launcher() {
        delete ui;
        ui = nullptr;
    }

    /*
 * toolButton_App_released
 * Method called when the user wants to launch an app
 */
    void Launcher::toolButton_App_released() {

        // Get the sender button
        auto *buttonWidget = qobject_cast<QToolButton *>(sender());

        // Check if the sender is valid
        if (!buttonWidget) {

            // The sender is not a widget
            return;
        }

        // Get the app's hash value from the button's object name
        const QString hash = buttonWidget->property("app_hash").toString();

        if (hash.isEmpty()) {
            return;
        }

        // Check if the debug is active
        if (FairWindSK::getInstance()->isDebug()) {

            // Write a message
            qDebug() << "Apps - hash:" << hash;
        }

        // Emit the signal to tell the MainWindow to update the UI and show the app with that particular hash value
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
        const int maxColumnWidth = qMax(columnWidth, static_cast<int>(std::round(columnWidth * 1.10)));
        const int maxRowHeight = qMax(rowHeight, static_cast<int>(std::round(rowHeight * 1.08)));
        const int baseIconSize = qMax(72, qMin(columnWidth - 32, rowHeight - 64));

        m_baseColumnWidth = columnWidth;
        m_baseRowHeight = rowHeight;
        m_baseIconSize = baseIconSize;

        // Iterate on the columns
        for (int col = 0; col < m_cols; col++) {
            // Set the column width for each column
            layout->setColumnMinimumWidth(col, maxColumnWidth);
        }

        // Iterate on the rows
        for (int row = 0; row < m_rows; row++) {
            // Set the row height for each row
            layout->setRowMinimumHeight(row, maxRowHeight);
        }

        for (auto button : m_buttons) {
            button->setMinimumSize(columnWidth, rowHeight);
        }

        updateButtonScales();
        updateScrollButtons();
    }

    void Launcher::onScrollLeft() {
        if (!m_buttons.empty()) {
            ui->scrollArea->horizontalScrollBar()->setValue(
                    ui->scrollArea->horizontalScrollBar()->value() - m_buttons.first()->iconSize().width() * 2
            );
        }
    }

    void Launcher::onScrollRight() {
        if (!m_buttons.empty()) {
            ui->scrollArea->horizontalScrollBar()->setValue(
                    ui->scrollArea->horizontalScrollBar()->value() + m_buttons.first()->iconSize().width() * 2
            );
        }
    }

    void Launcher::updateScrollButtons() const {
        const auto *scrollBar = ui->scrollArea->horizontalScrollBar();
        const bool canScroll = scrollBar->maximum() > scrollBar->minimum();
        ui->toolButton_Left->setEnabled(canScroll && scrollBar->value() > scrollBar->minimum());
        ui->toolButton_Right->setEnabled(canScroll && scrollBar->value() < scrollBar->maximum());
    }

    void Launcher::updateButtonScales() {
        if (!m_layout || m_buttons.isEmpty() || m_baseColumnWidth <= 0 || m_baseRowHeight <= 0 || m_baseIconSize <= 0) {
            return;
        }

        const int viewportWidth = ui->scrollArea->viewport()->width();
        if (viewportWidth <= 0) {
            return;
        }

        const int scrollValue = ui->scrollArea->horizontalScrollBar()->value();
        const int viewportCenterX = scrollValue + (viewportWidth / 2);
        const int columnStride = m_layout->horizontalSpacing() + m_layout->columnMinimumWidth(0);
        const int leftMargin = m_layout->contentsMargins().left();
        const qreal influenceRadius = std::max<qreal>(160.0, viewportWidth * 0.7);

        for (auto button : m_buttons) {
            const int column = button->property("launcher_column").toInt();
            const int columnCenter = leftMargin + (column * columnStride) + (m_layout->columnMinimumWidth(column) / 2);
            const qreal distance = std::abs(columnCenter - viewportCenterX);
            const qreal normalized = std::min<qreal>(1.0, distance / influenceRadius);
            const qreal eased = 1.0 - (normalized * normalized);
            const qreal iconScale = 0.72 + (0.42 * eased);
            const qreal cardScale = 0.86 + (0.18 * eased);
            const qreal textScale = 0.94 + (0.10 * eased);

            button->setIconSize(QSize(
                qRound(m_baseIconSize * iconScale),
                qRound(m_baseIconSize * iconScale)));
            button->setFixedSize(
                qRound(m_baseColumnWidth * cardScale),
                qRound(m_baseRowHeight * cardScale));

            QFont font = button->font();
            const qreal basePointSize = button->property("base_point_size").toReal();
            if (basePointSize > 0.0) {
                font.setPointSizeF(basePointSize * textScale);
                button->setFont(font);
            }
        }
    }

} // fairwindsk::ui::launcher
