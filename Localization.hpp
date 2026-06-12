//
// Created by OpenAI Codex on 29/04/26.
//

#ifndef FAIRWINDSK_LOCALIZATION_HPP
#define FAIRWINDSK_LOCALIZATION_HPP

#include <QLocale>
#include <QString>

namespace fairwindsk {
    class Configuration;

    namespace localization {
        QString normalizeLanguageSelection(const QString &value);
        QString effectiveLanguageCodeForSelection(const QString &value);
        QString effectiveLanguageCode(const Configuration &configuration);
        QLocale effectiveLocaleForSelection(const QString &value);
        QLocale effectiveLocale(const Configuration &configuration);
        QString languageTag(const QLocale &locale);
        QString cultureName(const QLocale &locale);
        QString webAcceptLanguageHeader(const QLocale &locale);
        QString translationResourcePath(const QString &languageCode);
    }
}

#endif //FAIRWINDSK_LOCALIZATION_HPP
