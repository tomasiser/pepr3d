#pragma once
#include "tools/Tool.h"
#include "ui/IconsMaterialDesign.h"
#include "ui/SidePane.h"

namespace pepr3d {

class MainApplication;

class DisplayOptions : public ITool {
   public:
    DisplayOptions(MainApplication& app) : mApplication(app) {}

    virtual std::string getName() const override {
        return "Display Options";
    }

    virtual std::string getIcon() const override {
        return ICON_MD_VISIBILITY;
    }

    virtual void drawToSidePane(SidePane& sidePane) override;

   private:
    MainApplication& mApplication;
};
}  // namespace pepr3d
