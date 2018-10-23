#pragma once
#include "tools/Tool.h"
#include "ui/SidePane.h"
#include "ui/IconsMaterialDesign.h"

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

}
