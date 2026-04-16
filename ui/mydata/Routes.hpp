//
// Created by Codex on 16/04/26.
//

#ifndef FAIRWINDSK_UI_MYDATA_ROUTES_HPP
#define FAIRWINDSK_UI_MYDATA_ROUTES_HPP

#include "ResourceTab.hpp"

namespace fairwindsk::ui::mydata {

    class Routes final : public ResourceTab {
        Q_OBJECT

    public:
        explicit Routes(QWidget *parent = nullptr);
        ~Routes() override = default;

    protected:
        QString searchPlaceholderText() const override;
        QString namePlaceholderText() const override;
        QString descriptionPlaceholderText() const override;
        QString typePlaceholderText() const override;
        QString coordinatesPlaceholderText() const override;
        QString importButtonText() const override;
        QString exportButtonText() const override;
        QString primaryRowActionToolTip() const override;
        QIcon primaryRowActionIcon() const override;
    };
}

#endif // FAIRWINDSK_UI_MYDATA_ROUTES_HPP
