//
// Created by Codex on 16/04/26.
//

#ifndef FAIRWINDSK_UI_MYDATA_REGIONS_HPP
#define FAIRWINDSK_UI_MYDATA_REGIONS_HPP

#include "ResourceCollectionPageBase.hpp"

namespace Ui { class Regions; }

namespace fairwindsk::ui::mydata {

    class Regions final : public ResourceCollectionPageBase {
        Q_OBJECT

    public:
        explicit Regions(QWidget *parent = nullptr);
        ~Regions() override;

    protected:
        QString pageTitle() const override;
        QString searchPlaceholderText() const override;

    private:
        ::Ui::Regions *ui = nullptr;
    };
}

#endif // FAIRWINDSK_UI_MYDATA_REGIONS_HPP
