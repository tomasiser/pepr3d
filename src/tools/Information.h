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
};
}  // namespace pepr3d
