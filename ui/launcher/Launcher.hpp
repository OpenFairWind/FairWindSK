//
// Created by __author__ on 21/01/2022.
//

#ifndef LAUNCHER_HPP
#define LAUNCHER_HPP

#include <QWidget>
#include <QToolButton>
#include <QString>

#include <FairWindSK.hpp>
#include "ui_Launcher.h"

namespace Ui { class Launcher; }

namespace fairwindsk::ui::launcher {
    class Launcher : public QWidget {
    Q_OBJECT

    public:
        explicit Launcher(QWidget *parent = nullptr);

        ~Launcher() override ;
        void refreshFromConfiguration(bool forceRebuild = false);
        QString currentPageTitle() const;
        QIcon currentPageIcon() const;

        void resizeEvent(QResizeEvent *event) override;
        void showEvent(QShowEvent *event) override;
        bool eventFilter(QObject *watched, QEvent *event) override;

    public slots:
        void onScrollLeft();
        void onScrollRight();

    signals:

        void foregroundAppChanged(const QString hash);
        void pageContextChanged();

    private:
        QString buildLayoutSignature() const;
        void rebuildTiles();
        void resize();
        void updateScrollButtons() const;
        int pageWidth() const;
        int currentPage() const;

    private:
        Ui::Launcher *ui = nullptr;

        int m_cols = 0;
        int m_rows = 0;
        int m_pageCount = 0;
        int m_targetPage = 0;
        QGridLayout *m_layout = nullptr;
        QList<QWidget *> m_tiles;
        QString m_layoutSignature;
        QString m_currentRootPageId;
        QStringList m_navigationStack;
    };
} // fairwindsk::ui

#endif //LAUNCHER_HPP
