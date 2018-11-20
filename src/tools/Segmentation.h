#pragma once
#include <optional>
#include <unordered_map>
#include "commands/CommandManager.h"
#include "tools/Tool.h"
#include "ui/IconsMaterialDesign.h"
#include "ui/ModelView.h"
#include "ui/SidePane.h"

namespace pepr3d {

class Segmentation : public ITool {
   public:
    Segmentation(MainApplication& app, CommandManager<class Geometry>& commandManager)
        : mApplication(app), mCommandManager(commandManager) {}

    virtual std::string getName() const override {
        return "Segmentation";
    }

    virtual std::string getIcon() const override {
        return ICON_MD_DASHBOARD;
    }

    virtual void drawToSidePane(SidePane& sidePane) override;
    // virtual void drawToModelView(ModelView& modelView) override;
    // virtual void onModelViewMouseDown(ModelView& modelView, ci::app::MouseEvent event) override;
    // virtual void onModelViewMouseUp(ModelView& modelView, ci::app::MouseEvent event) override;
    // virtual void onModelViewMouseDrag(ModelView& modelView, ci::app::MouseEvent event) override;
    // virtual void onModelViewMouseMove(ModelView& modelView, ci::app::MouseEvent event) override;

   private:
    MainApplication& mApplication;
    CommandManager<class Geometry>& mCommandManager;

    int mNumberOfClusters = 5;
    float mSmoothingLambda = 0.3f;
    size_t mNumberOfSegments = 0;

    std::unordered_map<size_t, std::vector<size_t>> mSegmentToTriangleIds;
};
}  // namespace pepr3d
