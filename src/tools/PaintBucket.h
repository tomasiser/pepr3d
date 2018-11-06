#pragma once
#include "tools/Tool.h"
#include "ui/IconsMaterialDesign.h"
#include "ui/SidePane.h"

namespace pepr3d {

class PaintBucket : public ITool {
   public:
    PaintBucket(MainApplication& app) : mApplication(app) {}

    virtual std::string getName() const override {
        return "Paint Bucket";
    }

    virtual std::string getIcon() const override {
        return ICON_MD_FORMAT_COLOR_FILL;
    }

    virtual void drawToSidePane(SidePane& sidePane) override;
    virtual void onModelViewMouseDown(ModelView& modelView, ci::app::MouseEvent event) override;
    virtual void onModelViewMouseDrag(ModelView& modelView, ci::app::MouseEvent event) override;

   private:
    MainApplication& mApplication;
};
}  // namespace pepr3d
