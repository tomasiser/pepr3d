#pragma once
#include <optional>
#include "tools/Tool.h"
#include "ui/IconsMaterialDesign.h"
#include "ui/ModelView.h"
#include "ui/SidePane.h"

namespace pepr3d {

class MainApplication;

class TrianglePainter : public ITool {
   public:
    TrianglePainter(MainApplication& app) : mApplication(app) {}

    virtual std::string getName() const override {
        return "Triangle Painter";
    }

    virtual std::string getIcon() const override {
        return ICON_MD_NETWORK_CELL;
    }

    virtual void drawToSidePane(SidePane& sidePane) override;
    virtual void onModelViewMouseDown(ModelView& modelView, ci::app::MouseEvent event) override;
    virtual void onModelViewMouseDrag(ModelView& modelView, ci::app::MouseEvent event) override;

   private:
    MainApplication& mApplication;
    glm::vec2 mLastClick;
    ci::Ray mLastRay;
    size_t mSelectedTriangleOriginalColor = 0;
    std::optional<std::size_t> mSelectedTriangleId = {};
};
}  // namespace pepr3d
