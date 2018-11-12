#pragma once
#include "tools/Tool.h"
#include "ui/IconsMaterialDesign.h"
#include "ui/SidePane.h"

namespace pepr3d {

class PaintBucket : public ITool {
   public:
    virtual std::string getName() const override {
        return "Paint Bucket";
    }

    virtual std::string getIcon() const override {
        return ICON_MD_FORMAT_COLOR_FILL;
    }
};
}  // namespace pepr3d
