#pragma once
#include "tools/Tool.h"
#include "ui/SidePane.h"
#include "ui/ModelView.h"
#include "ui/IconsMaterialDesign.h"

namespace pepr3d {

class TrianglePainter : public ITool {
public:
    virtual std::string getName() const override {
        return "Triangle Painter";
    }

    virtual std::string getIcon() const override {
        return ICON_MD_NETWORK_CELL;
    }

    virtual void drawToSidePane(SidePane& sidePane) override;
    virtual void onModelViewMouseDown(ModelView& modelView, ci::app::MouseEvent event) override;

private:
    glm::vec2 mLastClick;
    ci::Ray mLastRay;
};

}
