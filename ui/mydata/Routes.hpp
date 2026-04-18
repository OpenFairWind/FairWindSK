//
// Created by Codex on 16/04/26.
//

#ifndef FAIRWINDSK_UI_MYDATA_ROUTES_HPP
#define FAIRWINDSK_UI_MYDATA_ROUTES_HPP

#include "ResourceCollectionPageBase.hpp"

namespace Ui { class Routes; }

namespace fairwindsk::ui::mydata {

    class Routes final : public ResourceCollectionPageBase {
        Q_OBJECT

    public:
        explicit Routes(QWidget *parent = nullptr);
        ~Routes() override;

    protected:
        QString pageTitle() const override;
        QString searchPlaceholderText() const override;

    private:
        ::Ui::Routes *ui = nullptr;
    };
}

#endif // FAIRWINDSK_UI_MYDATA_ROUTES_HPP
