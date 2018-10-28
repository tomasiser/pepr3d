#pragma once
#include "tools/Tool.h"
#include "ui/IconsMaterialDesign.h"
#include "ui/SidePane.h"

namespace pepr3d {

class DisplayOptions : public ITool {
   public:
    virtual std::string getName() const override {
        return "Display Options";
    }

    virtual std::string getIcon() const override {
        return ICON_MD_VISIBILITY;
    }
};
}
