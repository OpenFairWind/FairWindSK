#ifndef FAIRWINDSK_MANUALSCREENSHOTCAPTURE_HPP
#define FAIRWINDSK_MANUALSCREENSHOTCAPTURE_HPP

#include <QString>

namespace fairwindsk::ui {
class MainWindow;
}

namespace fairwindsk::runtime {
void scheduleManualScreenshotCapture(ui::MainWindow &window,
                                     const QString &outputDirectory);
} // namespace fairwindsk::runtime

#endif // FAIRWINDSK_MANUALSCREENSHOTCAPTURE_HPP
