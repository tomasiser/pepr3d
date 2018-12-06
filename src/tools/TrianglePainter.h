#pragma once
#include <optional>
#include "commands/CommandManager.h"
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

    virtual std::string getDescription() const override {
        return "Color individual triangles by clicking or dragging the mouse.";
    }

    virtual std::string getIcon() const override {
        return ICON_MD_EDIT;
    }

    virtual void drawToSidePane(SidePane& sidePane) override;
    virtual void drawToModelView(ModelView& modelView) override;
    virtual void onModelViewMouseDown(ModelView& modelView, ci::app::MouseEvent event) override;
    virtual void onModelViewMouseUp(ModelView& modelView, ci::app::MouseEvent event) override;
    virtual void onModelViewMouseDrag(ModelView& modelView, ci::app::MouseEvent event) override;
    virtual void onModelViewMouseMove(ModelView& modelView, ci::app::MouseEvent event) override;
    virtual void onNewGeometryLoaded(ModelView& modelView) override;

   private:
    MainApplication& mApplication;
    glm::vec2 mLastClick;
    ci::Ray mLastRay;

    bool mGroupCommands = false;
    std::optional<std::size_t> mHoveredTriangleId = {};
};
}  // namespace pepr3d
