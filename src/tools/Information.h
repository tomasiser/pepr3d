#pragma once
#include "tools/Tool.h"
#include "ui/IconsMaterialDesign.h"
#include "ui/SidePane.h"

namespace pepr3d {

class Information : public ITool {
   public:
    virtual std::string getName() const override {
        return "Information";
    }

    virtual std::string getIcon() const override {
        return ICON_MD_INFO_OUTLINE;
    }

    virtual void drawToSidePane(SidePane& sidePane) override {
        sidePane.drawText("Pepr3D 0.1 (early preview)");
        sidePane.drawText("github.com/tomasiser/pepr3d");
    }
};
}  // namespace pepr3d
