#pragma once
#include <cinder/Ray.h>
#include "tools/Tool.h"
#include "ui/IconsMaterialDesign.h"
#include "ui/SidePane.h"

namespace pepr3d {
struct BrushSettings {
    /// Index of the selected color
    size_t color=0;

    /// Size of a brush in model space units
    double size = 0.1f;

    /// Paint only to triangles connected to the origin
    bool continuous = true;

    /// Paint onto backward facing triangles
    bool paintBackfaces = false;
};

class Brush : public ITool {
   public:
    Brush(MainApplication& app, CommandManager<class Geometry>& commandManager)
        : mApplication(app), mCommandManager(commandManager) {}

    virtual std::string getName() const override {
        return "Brush";
    }

    virtual std::string getIcon() const override {
        return ICON_MD_BRUSH;
    }

    virtual void onModelViewMouseDown(ModelView& modelView, ci::app::MouseEvent event) override;
    virtual void onModelViewMouseUp(ModelView& modelView, ci::app::MouseEvent event) override;
    virtual void onModelViewMouseDrag(ModelView& modelView, ci::app::MouseEvent event) override;
    virtual void onModelViewMouseMove(ModelView& modelView, ci::app::MouseEvent event) override;

   private:
    /// Do painting tick on the model
    void paint();

    /// Stop paining
    void stopPaint();

    void updateHighlight() const;

    ci::Ray mLastRay;
    MainApplication& mApplication;
    CommandManager<class Geometry>& mCommandManager;

    BrushSettings mBrushSettings;
};
}  // namespace pepr3d
