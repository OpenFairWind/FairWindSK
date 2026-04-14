//
// Created by Codex on 14/04/26.
//

#ifndef FAIRWINDSK_UI_WIDGETS_TOUCHFILEBROWSER_HPP
#define FAIRWINDSK_UI_WIDGETS_TOUCHFILEBROWSER_HPP

#include <QFileSystemModel>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeView>
#include <QWidget>

namespace fairwindsk::ui::widgets {
    class TouchFileBrowser final : public QWidget {
        Q_OBJECT

    public:
        enum class Mode {
            OpenFile,
            SaveFile
        };

        explicit TouchFileBrowser(Mode mode,
                                  const QString &directory,
                                  const QString &fileName,
                                  const QStringList &nameFilters,
                                  QWidget *parent = nullptr);

        QString currentDirectory() const;
        QString fileName() const;
        QString selectedPath() const;
        bool canAccept(QString *message) const;

    signals:
        void finished(int result);

    protected:
        void showEvent(QShowEvent *event) override;
        void changeEvent(QEvent *event) override;
        bool eventFilter(QObject *watched, QEvent *event) override;

    private:
        void updateGeometryToHost();
        void applyComfortChrome();
        void navigateTo(const QString &path);
        void handleSelection(const QModelIndex &index, bool activate);
        void updateSelectionSummary();
        void finishDrawer(int result);
        static QString defaultBrowserDirectory();
        static QString preferredSuffix(const QStringList &nameFilters);

        Mode m_mode;
        QStringList m_nameFilters;
        QString m_currentDirectory;
        QString m_selectedPath;
        QFileSystemModel *m_model = nullptr;
        QTreeView *m_view = nullptr;
        QLineEdit *m_pathEdit = nullptr;
        QLineEdit *m_nameEdit = nullptr;
        QLabel *m_titleLabel = nullptr;
        QLabel *m_selectionLabel = nullptr;
        QFrame *m_browserFrame = nullptr;
        QPushButton *m_backButton = nullptr;
        QPushButton *m_homeButton = nullptr;
        QPushButton *m_upButton = nullptr;
        QPushButton *m_acceptButton = nullptr;
        QWidget *m_geometryHost = nullptr;
        bool m_isApplyingComfortChrome = false;
    };
}

#endif // FAIRWINDSK_UI_WIDGETS_TOUCHFILEBROWSER_HPP
