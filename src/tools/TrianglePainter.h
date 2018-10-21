#pragma once
#include "Tool.h"
#include "SidePane.h"
#include "IconsMaterialDesign.h"

class TrianglePainter : public ITool {
public:
    virtual std::string_view getName() const override {
        return "Triangle Painter";
    }

    virtual std::string_view getIcon() const override {
        return ICON_MD_NETWORK_CELL;
    }
};