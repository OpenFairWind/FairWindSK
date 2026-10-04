#include "ManualScreenshotCapture.hpp"

#include "DiagnosticsSupport.hpp"
#include "ui/MainWindow.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QTabWidget>
#include <QTimer>
#include <QToolButton>

#include <cstdlib>
#include <functional>
#include <memory>

namespace fairwindsk::runtime {
void scheduleManualScreenshotCapture(ui::MainWindow &window,
                                     const QString &outputDirectory) {
  struct CaptureStep {
    QString filename;
    std::function<void()> prepare;
  };

  QDir().mkpath(outputDirectory);
  auto *windowPointer = &window;
  auto visibleTabWidget = [windowPointer]() -> QTabWidget * {
    const auto tabWidgets = windowPointer->findChildren<QTabWidget *>();
    for (QTabWidget *tabWidget : tabWidgets) {
      if (tabWidget->isVisible()) {
        return tabWidget;
      }
    }
    return nullptr;
  };
  auto selectTab = [visibleTabWidget](const int index) {
    if (auto *tabs = visibleTabWidget()) {
      tabs->setCurrentIndex(index);
    }
  };
  auto clickButton = [windowPointer](const QString &name) {
    if (auto *button = windowPointer->findChild<QToolButton *>(name)) {
      button->click();
    }
  };

  const QList<CaptureStep> steps = {
      {QStringLiteral("screen-layout-overview.png"),
       [windowPointer]() { windowPointer->onApps(); }},
      {QStringLiteral("launcher-overview.png"),
       [windowPointer]() { windowPointer->onApps(); }},
      {QStringLiteral("topbar-overview.png"), {}},
      {QStringLiteral("bottombar-overview.png"), {}},
      {QStringLiteral("settings-overview.png"),
       [windowPointer]() { windowPointer->onSettings(); }},
      {QStringLiteral("settings-main.png"), [selectTab]() { selectTab(0); }},
      {QStringLiteral("settings-topbar-editor.png"),
       [selectTab]() { selectTab(1); }},
      {QStringLiteral("settings-widgets.png"), [selectTab]() { selectTab(3); }},
      {QStringLiteral("settings-comfort.png"), [selectTab]() { selectTab(4); }},
      {QStringLiteral("settings-connection.png"),
       [selectTab]() { selectTab(5); }},
      {QStringLiteral("settings-signalk.png"), [selectTab]() { selectTab(6); }},
      {QStringLiteral("settings-units.png"), [selectTab]() { selectTab(7); }},
      {QStringLiteral("settings-apps.png"), [selectTab]() { selectTab(8); }},
      {QStringLiteral("settings-system-log.png"),
       [selectTab]() { selectTab(9); }},
      {QStringLiteral("settings-system-perf.png"), {}},
      {QStringLiteral("mydata-main-tabs.png"),
       [windowPointer]() { windowPointer->onMyData(); }},
      {QStringLiteral("mydata-waypoints-list.png"),
       [selectTab]() { selectTab(0); }},
      {QStringLiteral("mydata-routes-list.png"),
       [selectTab]() { selectTab(1); }},
      {QStringLiteral("mydata-regions-list.png"),
       [selectTab]() { selectTab(2); }},
      {QStringLiteral("mydata-notes-list.png"),
       [selectTab]() { selectTab(3); }},
      {QStringLiteral("mydata-charts-list.png"),
       [selectTab]() { selectTab(4); }},
      {QStringLiteral("mydata-tracks-list.png"),
       [selectTab]() { selectTab(5); }},
      {QStringLiteral("mydata-files-browser.png"),
       [selectTab]() { selectTab(6); }},
      {QStringLiteral("pob-bar-active.png"),
       [clickButton]() { clickButton(QStringLiteral("toolButton_POB")); }},
      {QStringLiteral("autopilot-bar.png"),
       [clickButton]() {
         clickButton(QStringLiteral("toolButton_Autopilot"));
       }},
      {QStringLiteral("anchor-bar.png"),
       [clickButton]() { clickButton(QStringLiteral("toolButton_Anchor")); }},
      {QStringLiteral("alarms-bar.png"),
       [clickButton]() { clickButton(QStringLiteral("toolButton_Alarms")); }}};

  auto index = std::make_shared<int>(0);
  auto runStep = std::make_shared<std::function<void()>>();
  *runStep = [&window, outputDirectory, steps, index, runStep]() {
    if (*index >= steps.size()) {
      markGracefulShutdown();
      std::_Exit(EXIT_SUCCESS);
    }
    const CaptureStep &step = steps.at(*index);
    if (step.prepare) {
      step.prepare();
    }
    QTimer::singleShot(
        500, &window, [&window, outputDirectory, steps, index, runStep]() {
          const QString path =
              QDir(outputDirectory).filePath(steps.at(*index).filename);
          if (!window.grab().save(path, "PNG")) {
            qCritical() << "Unable to save manual screenshot" << path;
            QCoreApplication::exit(1);
            return;
          }
          ++(*index);
          (*runStep)();
        });
  };
  QTimer::singleShot(1500, &window, [runStep]() { (*runStep)(); });
}
} // namespace fairwindsk::runtime

namespace {
void startManualScreenshotCapture() {
  const QString outputDirectory =
      qEnvironmentVariable("FAIRWINDSK_MANUAL_SCREENSHOT_DIR").trimmed();
  if (outputDirectory.isEmpty()) {
    return;
  }

  auto waitForWindow = std::make_shared<std::function<void()>>();
  *waitForWindow = [outputDirectory, waitForWindow]() {
    if (auto *window = fairwindsk::ui::MainWindow::instance()) {
      fairwindsk::runtime::scheduleManualScreenshotCapture(*window,
                                                           outputDirectory);
      return;
    }
    QTimer::singleShot(250, [waitForWindow]() { (*waitForWindow)(); });
  };
  QTimer::singleShot(0, [waitForWindow]() { (*waitForWindow)(); });
}
} // namespace

Q_COREAPP_STARTUP_FUNCTION(startManualScreenshotCapture)
