#pragma once
#include <map>
#include <optional>
#include <unordered_map>
#include "commands/CommandManager.h"
#include "geometry/Geometry.h"
#include "tools/Tool.h"
#include "ui/IconsMaterialDesign.h"
#include "ui/ModelView.h"
#include "ui/SidePane.h"

namespace pepr3d {

class Segmentation : public Tool {
   public:
    explicit Segmentation(MainApplication& app) : mApplication(app) {}

    virtual std::string getName() const override {
        return "Automatic Segmentation";
    }

    virtual std::string getDescription() const override {
        return "Automatically separate the model into regions based on the thickness of the model and color each "
               "region.";
    }

    virtual std::optional<Hotkey> getHotkey(const Hotkeys& hotkeys) const override {
        return hotkeys.findHotkey(HotkeyAction::SelectSegmentation);
    }

    virtual std::string getIcon() const override {
        return ICON_MD_DASHBOARD;
    }

    virtual bool isEnabled() const override {
        if(mSdfEnabled == nullptr) {
            return mGeometryCorrect;
        }
        return mGeometryCorrect && *mSdfEnabled;
    }

    virtual void drawToSidePane(SidePane& sidePane) override;
    virtual void drawToModelView(ModelView& modelView) override;
    virtual void onModelViewMouseDown(ModelView& modelView, ci::app::MouseEvent event) override;
    // virtual void onModelViewMouseUp(ModelView& modelView, ci::app::MouseEvent event) override;
    // virtual void onModelViewMouseDrag(ModelView& modelView, ci::app::MouseEvent event) override;
    virtual void onModelViewMouseMove(ModelView& modelView, ci::app::MouseEvent event) override;
    virtual void onToolDeselect(ModelView& modelView) override;
    virtual void onNewGeometryLoaded(ModelView& modelView) override;

   private:
    MainApplication& mApplication;

    float mNumberOfClusters = 20.0f;  // [0, 100]%, real range {2, .., 15}
    float mSmoothingLambda = 30.0f;   // [0, 100]%, real range [0.01, 1]
    size_t mNumberOfSegments = 0;
    bool mPickState = false;
    bool mGeometryCorrect = true;
    const bool* mSdfEnabled = nullptr;

    std::vector<size_t> mNewColors;
    std::vector<glm::vec4> mSegmentationColors;
    std::optional<std::size_t> mHoveredTriangleId = {};

    std::map<size_t, std::vector<size_t>> mSegmentToTriangleIds;
    std::unordered_map<size_t, size_t> mTriangleToSegmentMap;

    void reset();
    void computeSegmentation();
    void cancel();
    void setSegmentColor(const size_t segmentId, const glm::vec4 newColor);
};
}  // namespace pepr3d
