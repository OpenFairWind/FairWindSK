//
// Created by OpenAI Codex on 29/04/26.
//

#include "Localization.hpp"

#include <QStringList>

#include "Configuration.hpp"

namespace fairwindsk::localization {
    namespace {
        QLocale englishFallbackLocale() {
            return QLocale(QLocale::English, QLocale::UnitedStates);
        }

        QLocale defaultLocaleForLanguage(const QString &languageCode) {
            if (languageCode == QStringLiteral("it")) {
                return QLocale(QLocale::Italian, QLocale::Italy);
            }

            return englishFallbackLocale();
        }

        bool isSupportedSystemLanguage(const QLocale &locale) {
            return locale.language() == QLocale::English || locale.language() == QLocale::Italian;
        }
    }

    QString normalizeLanguageSelection(const QString &value) {
        QString normalized = value.trimmed().toLower();
        normalized.replace(QLatin1Char('-'), QLatin1Char('_'));
        if (normalized.isEmpty() || normalized == QStringLiteral("system")) {
            return QStringLiteral("system");
        }

        const QString languageCode = normalized.section(QLatin1Char('_'), 0, 0);
        if (languageCode == QStringLiteral("en") || languageCode == QStringLiteral("it")) {
            return languageCode;
        }

        return QStringLiteral("en");
    }

    QString effectiveLanguageCodeForSelection(const QString &value) {
        const QString normalized = normalizeLanguageSelection(value);
        if (normalized == QStringLiteral("en") || normalized == QStringLiteral("it")) {
            return normalized;
        }

        return QLocale::system().language() == QLocale::Italian ? QStringLiteral("it") : QStringLiteral("en");
    }

    QString effectiveLanguageCode(const Configuration &configuration) {
        return effectiveLanguageCodeForSelection(configuration.getLanguage());
    }

    QLocale effectiveLocaleForSelection(const QString &value) {
        const QString normalized = normalizeLanguageSelection(value);
        if (normalized == QStringLiteral("system")) {
            const QLocale systemLocale = QLocale::system();
            return isSupportedSystemLanguage(systemLocale) ? systemLocale : englishFallbackLocale();
        }

        return defaultLocaleForLanguage(normalized);
    }

    QLocale effectiveLocale(const Configuration &configuration) {
        return effectiveLocaleForSelection(configuration.getLanguage());
    }

    QString languageTag(const QLocale &locale) {
        const QString tag = locale.bcp47Name().trimmed();
        if (tag.isEmpty() || tag == QStringLiteral("C")) {
            return QStringLiteral("en-US");
        }

        return tag;
    }

    QString cultureName(const QLocale &locale) {
        const QString name = locale.name().trimmed();
        if (name.isEmpty() || name == QStringLiteral("C")) {
            return QStringLiteral("en_US");
        }

        return name;
    }

    QString webAcceptLanguageHeader(const QLocale &locale) {
        const QString tag = languageTag(locale);
        const QString primaryLanguage = tag.section(QLatin1Char('-'), 0, 0).toLower();
        QStringList languages;

        languages.append(tag);

        if (tag.compare(primaryLanguage, Qt::CaseInsensitive) != 0) {
            languages.append(primaryLanguage == QStringLiteral("en")
                             ? QStringLiteral("en;q=0.9")
                             : QStringLiteral("%1;q=0.9").arg(primaryLanguage));
        }

        if (primaryLanguage != QStringLiteral("en")) {
            languages.append(QStringLiteral("en;q=0.8"));
        }

        return languages.join(QLatin1Char(','));
    }

    QString translationResourcePath(const QString &languageCode) {
        const QString normalized = normalizeLanguageSelection(languageCode);
        if (normalized == QStringLiteral("en") || normalized == QStringLiteral("system")) {
            return QString();
        }

        return QStringLiteral(":/resources/i18n/fairwindsk_%1.qm").arg(normalized);
    }
}
