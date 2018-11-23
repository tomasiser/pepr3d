#include "tools/Segmentation.h"
#include <random>
#include <vector>
#include "commands/CmdPaintSingleColor.h"
#include "ui/MainApplication.h"

namespace pepr3d {

void Segmentation::drawToSidePane(SidePane& sidePane) {
    if(sidePane.drawButton("Compute SDF")) {
        mApplication.getCurrentGeometry()->preSegmentation();
    }

    ColorManager& colorManager = mApplication.getCurrentGeometry()->getColorManager();

    if(sidePane.drawButton("Segment!")) {
        mSegmentToTriangleIds.clear();
        mNewColors.clear();
        mNumberOfSegments =
            mApplication.getCurrentGeometry()->segmentation(mNumberOfClusters, mSmoothingLambda, mSegmentToTriangleIds);

        if(mNumberOfSegments > 0) {
            // We only want to save the state if we segmented for the first time
            if(mPickState == false) {
                // Keep the old map, we will restore it later
                mOldColorManager = colorManager;
                mOldColorManagerMap = colorManager.getColorMap();
                mOldColorManagerSize = colorManager.size();
                mNewColors.resize(mNumberOfSegments);
                for(size_t i = 0; i < mNumberOfSegments; ++i) {
                    mNewColors[i] = i;
                }
                // Create a new map to display our new segments
                colorManager = ColorManager(mNumberOfSegments);
                mOldColorBuffer = mApplication.getCurrentGeometry()->getColorBuffer();
            }
            mPickState = true;
            assert(mNumberOfSegments != std::numeric_limits<size_t>::max());
            assert(mNumberOfSegments <= PEPR3D_MAX_PALETTE_COLORS);
            assert(colorManager.size() == mNumberOfSegments);

            std::vector<Geometry::ColorIndex> newColorBuffer;
            newColorBuffer.resize(mOldColorBuffer.size());

            // Set the colors in the new color buffer
            for(const auto& toPaint : mSegmentToTriangleIds) {
                std::vector<size_t> proxy = toPaint.second;
                for(const auto& tri : proxy) {
                    newColorBuffer[3 * tri] = static_cast<Geometry::ColorIndex>(toPaint.first);
                    newColorBuffer[3 * tri + 1] = static_cast<Geometry::ColorIndex>(toPaint.first);
                    newColorBuffer[3 * tri + 2] = static_cast<Geometry::ColorIndex>(toPaint.first);
                }
            }
            mApplication.getCurrentGeometry()->getColorBuffer() = newColorBuffer;
        }
    }

    sidePane.drawIntDragger("Robustness [2,15]", mNumberOfClusters, 0.25f, 2, 15, "%.0f", 40.0f);
    sidePane.drawFloatDragger("Edge tolerance [0,1]", mSmoothingLambda, .01f, 0.01f, 1.f, "%.02f", 70.f);

    sidePane.drawSeparator();

    if(mPickState) {
        sidePane.drawText("Segmented into " + std::to_string(mNumberOfSegments) + " segments.");

        sidePane.drawColorPalette(mOldColorManager);
        sidePane.drawText("Current color selected: " + std::to_string(mOldColorManager.getActiveColorIndex()));

        for(const auto& toPaint : mSegmentToTriangleIds) {
            std::string displayText = "Segment " + std::to_string(toPaint.first);
            glm::vec3 hsvButtonColor = ci::rgbToHsv(static_cast<ci::ColorA>(colorManager.getColor(toPaint.first)));
            hsvButtonColor.y = 0.5;  // reduce the saturation
            if(sidePane.drawColoredButton(displayText.c_str(), ci::hsvToRgb(hsvButtonColor))) {
                mNewColors[toPaint.first] = mOldColorManager.getActiveColorIndex();
                colorManager.setColor(toPaint.first, mOldColorManager.getColor(mOldColorManager.getActiveColorIndex()));
            }
        }

        if(sidePane.drawButton("Accept")) {
            mPickState = false;
            ColorManager& colorManager = mApplication.getCurrentGeometry()->getColorManager();
            colorManager = ColorManager(mOldColorManagerMap.begin(), mOldColorManagerMap.end());
            for(const auto& toPaint : mSegmentToTriangleIds) {
                const size_t activeColorAssigned = mNewColors[toPaint.first];
                auto proxy = toPaint.second;
                std::cout << "Assigned seg " << toPaint.first << " to color " << activeColorAssigned << "\n";
                mCommandManager.execute(std::make_unique<CmdPaintSingleColor>(std::move(proxy), activeColorAssigned),
                                        false);
            }
            mOldColorManagerMap.clear();
            mOldColorManagerSize = 0;
            std::cout << "Segmentation applied.\n";
            mSegmentToTriangleIds.clear();
        }
    } else {
        sidePane.drawText("Segment the model first.");
    }
}

void Segmentation::onToolDeselect(ModelView& modelView) {
    if(mPickState) {
        mApplication.getCurrentGeometry()->getColorBuffer() = mOldColorBuffer;
        ColorManager& colorManager = mApplication.getCurrentGeometry()->getColorManager();
        colorManager = ColorManager(mOldColorManagerMap.begin(), mOldColorManagerMap.end());
        mOldColorManagerMap.clear();
        mOldColorManagerSize = 0;
        mPickState = false;
        std::cout << "Segmentation canceled.\n";
        mSegmentToTriangleIds.clear();
    }
}

}  // namespace pepr3d