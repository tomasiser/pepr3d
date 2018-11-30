#pragma once
#include "tools/Tool.h"
#include "ui/IconsMaterialDesign.h"
#include "ui/MainApplication.h"

namespace pepr3d {

class SemiautomaticSegmentation : public ITool {
   public:
    SemiautomaticSegmentation(MainApplication& app) : mApplication(app) {}

    virtual std::string getName() const override {
        return "Manual Segmentation";
    }

    virtual std::string getIcon() const override {
        return ICON_MD_WIDGETS;
    }

    virtual void drawToSidePane(SidePane& sidePane) override;
    virtual void drawToModelView(ModelView& modelView) override;
    virtual void onModelViewMouseDown(ModelView& modelView, ci::app::MouseEvent event) override;
    virtual void onModelViewMouseDrag(ModelView& modelView, ci::app::MouseEvent event) override;
    virtual void onModelViewMouseMove(ModelView& modelView, ci::app::MouseEvent event) override;

   private:
    MainApplication& mApplication;

    std::optional<std::size_t> mHoveredTriangleId = {};
    std::unordered_map<std::size_t, std::size_t> mStartingTriangles;

    void reset();
    void setupOverride();
    void setTriangleColor();
};
}  // namespace pepr3d