#include "tools/Segmentation.h"
#include <random>
#include <vector>
#include "commands/CmdPaintSingleColor.h"
#include "geometry/Geometry.h"
#include "ui/MainApplication.h"

namespace pepr3d {

void Segmentation::drawToSidePane(SidePane& sidePane) {
    if(sidePane.drawButton("Compute SDF")) {
        mApplication.getCurrentGeometry()->preSegmentation();
    }

    bool pickColors = false;

    if(sidePane.drawButton("Segment!")) {
        mNumberOfSegments =
            mApplication.getCurrentGeometry()->segmentation(mNumberOfClusters, mSmoothingLambda, mSegmentToTriangleIds);
        if(mNumberOfSegments > 0) {
            pickColors = true;
        }
    }

    sidePane.drawIntDragger("Robustness [2,15]", mNumberOfClusters, 0.25f, 2, 15, "%.0f", 40.0f);
    sidePane.drawFloatDragger("Edge tolerance [0,1]", mSmoothingLambda, .01f, 0.01f, 1.f, "%.02f", 70.f);

    sidePane.drawSeparator();

    if(mNumberOfSegments == 0) {
        sidePane.drawText("Segment the model first.");
    } else {
        sidePane.drawText("Segmented into " + std::to_string(mNumberOfSegments) + " segments.");
    }
    ColorManager& colorManager = mApplication.getCurrentGeometry()->getColorManager();

    for(const auto& toPaint : mSegmentToTriangleIds) {
        std::string displayText =
            "Segment " + std::to_string(toPaint.first) + ", triangles: " + std::to_string(toPaint.second.size());
        ImGui::TextColored(ImVec4(colorManager.getColor(toPaint.first)), displayText.c_str());
    }

    if(pickColors && mNumberOfSegments != 0) {
        assert(mNumberOfSegments != std::numeric_limits<size_t>::max());
        assert(mNumberOfSegments <= PEPR3D_MAX_PALETTE_COLORS);

        // Keep the old map, we will restore it later
        ColorManager::ColorMap oldMap = colorManager.getColorMap();
        // Create a new map to display our new segments
        colorManager = ColorManager(mNumberOfSegments);

        for(const auto& toPaint : mSegmentToTriangleIds) {
            std::vector<size_t> proxy = toPaint.second;
            mCommandManager.execute(std::make_unique<CmdPaintSingleColor>(std::move(proxy), toPaint.first), false);
        }
    }
}

}  // namespace pepr3d