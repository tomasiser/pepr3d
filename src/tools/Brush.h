#pragma once
#include <cinder/Ray.h>
#include "tools/Tool.h"
#include "ui/IconsMaterialDesign.h"
#include "ui/SidePane.h"

namespace pepr3d {

class Brush : public ITool {
   public:
    Brush(MainApplication& app, CommandManager<class Geometry>& commandManager)
        : mApplication(app), mCommandManager(commandManager) {}

    void setBrushSize(float brushSize) {
        mBrushSize = brushSize;
    }

    float getBrushSize() const {return mBrushSize;}

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

    float mBrushSize = 0.1f;
};
}  // namespace pepr3d
