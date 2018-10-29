#pragma once
#include "tools/Tool.h"
#include "ui/IconsMaterialDesign.h"
#include "ui/SidePane.h"

namespace pepr3d {

class Segmentation : public ITool {
   public:
    virtual std::string getName() const override {
        return "Segmentation";
    }

    virtual std::string getIcon() const override {
        return ICON_MD_TEXTURE;
    }
};
}
