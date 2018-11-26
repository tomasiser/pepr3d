#include "tools/Segmentation.h"
#include <random>
#include <vector>
#include "commands/CmdPaintSingleColor.h"
#include "ui/MainApplication.h"

namespace pepr3d {

void Segmentation::drawToSidePane(SidePane& sidePane) {
    bool sdfComputed = mApplication.getCurrentGeometry()->sdfComputed();
    if(!sdfComputed) {
        sidePane.drawText("Warning: This computation may\ntake a long time to perform.");
        if(sidePane.drawButton("Compute SDF")) {
            mApplication.getCurrentGeometry()->preSegmentation();
        }
    } else {
        if(sidePane.drawButton("Segment!")) {
            computeSegmentaton();
        }
        sidePane.drawIntDragger("Robustness [2,15]", mNumberOfClusters, 0.25f, 2, 15, "%.0f", 40.0f);
        sidePane.drawFloatDragger("Edge tolerance [0,1]", mSmoothingLambda, .01f, 0.01f, 1.f, "%.02f", 70.f);
    }

    sidePane.drawSeparator();

    ColorManager& colorManager = mApplication.getCurrentGeometry()->getColorManager();
    if(mPickState) {
        sidePane.drawText("Segmented into " + std::to_string(mNumberOfSegments) +
                          " segments.\nAssign a color from the palette\nto each segment.");

        sidePane.drawColorPalette(mOldColorManager);

        for(const auto& toPaint : mSegmentToTriangleIds) {
            std::string displayText = "Segment " + std::to_string(toPaint.first);
            glm::vec3 hsvButtonColor = ci::rgbToHsv(static_cast<ci::ColorA>(colorManager.getColor(toPaint.first)));
            hsvButtonColor.y = 0.75;  // reduce the saturation
            ci::ColorA borderColor = ci::hsvToRgb(hsvButtonColor);
            float thickness = 3.0f;
            if(mHoveredTriangleId) {
                auto find = mTriangleToSegmentMap.find(*mHoveredTriangleId);
                assert(find != mTriangleToSegmentMap.end());
                assert((*find).first == *mHoveredTriangleId);
                if((*find).second == toPaint.first) {
                    borderColor = ci::ColorA(1, 0, 0, 1);
                    displayText = "Currently hovered";
                    thickness = 5.0f;
                }
            }
            if(sidePane.drawColoredButton(displayText.c_str(), borderColor, thickness)) {
                mNewColors[toPaint.first] = mOldColorManager.getActiveColorIndex();
                colorManager.setColor(toPaint.first, mOldColorManager.getColor(mOldColorManager.getActiveColorIndex()));
            }
        }

        if(sidePane.drawButton("Accept")) {
            // Find the maximum index of the color assignment
            size_t maxColorIndex = std::numeric_limits<size_t>::min();
            for(const auto& segment : mSegmentToTriangleIds) {
                const size_t activeColorAssigned = mNewColors[segment.first];
                if(activeColorAssigned > maxColorIndex) {
                    maxColorIndex = activeColorAssigned;
                }
            }

            // If the user assigned colors are valid (i.e. there aren't colors out of the palette size), apply.
            if(maxColorIndex < mOldColorManager.size()) {
                revertColorPalette();
                for(const auto& segment : mSegmentToTriangleIds) {
                    const size_t activeColorAssigned = mNewColors[segment.first];
                    auto proxy = segment.second;
                    mCommandManager.execute(
                        std::make_unique<CmdPaintSingleColor>(std::move(proxy), activeColorAssigned), false);
                }
                reset();
                CI_LOG_I("Segmentation applied.");
            } else {  // Else report the error to the user and continue.
                /// \todo Popup for the user
                CI_LOG_E("Please assign all segments to a color from the palette first.");
            }
        }
        if(sidePane.drawButton("Cancel")) {
            cancel();
        }
    }
}

void Segmentation::cancel() {
    if(mPickState) {
        // Restore the color buffer
        mApplication.getCurrentGeometry()->getColorBuffer() = mOldColorBuffer;
        // Revert and reset the rest
        revertColorPalette();
        reset();
        CI_LOG_I("Segmentation canceled.");
    }
}

void Segmentation::onToolDeselect(ModelView& modelView) {
    cancel();
}

void Segmentation::onModelViewMouseMove(ModelView& modelView, ci::app::MouseEvent event) {
    ci::Ray mLastRay = modelView.getRayFromWindowCoordinates(event.getPos());
    auto geometry = mApplication.getCurrentGeometry();
    if(geometry == nullptr) {
        mHoveredTriangleId = {};
        return;
    }
    mHoveredTriangleId = geometry->intersectMesh(mLastRay);
}

void Segmentation::drawToModelView(ModelView& modelView) {
    if(mHoveredTriangleId && mPickState) {
        modelView.drawTriangleHighlight(*mHoveredTriangleId);
    }
}

void Segmentation::onModelViewMouseDown(ModelView& modelView, ci::app::MouseEvent event) {
    if(!event.isLeftDown()) {
        return;
    }
    if(!mHoveredTriangleId) {
        return;
    }

    auto find = mTriangleToSegmentMap.find(*mHoveredTriangleId);
    assert(find != mTriangleToSegmentMap.end());
    assert((*find).first == *mHoveredTriangleId);

    const size_t segId = (*find).second;
    assert(segId < mNumberOfSegments);
    assert(segId < mNewColors.size());
    mNewColors[segId] = mOldColorManager.getActiveColorIndex();

    ColorManager& colorManager = mApplication.getCurrentGeometry()->getColorManager();
    colorManager.setColor(segId, mOldColorManager.getColor(mOldColorManager.getActiveColorIndex()));
}

void Segmentation::revertColorPalette() {
    ColorManager& colorManager = mApplication.getCurrentGeometry()->getColorManager();
    colorManager = mOldColorManager;
}

void Segmentation::reset() {
    mNumberOfSegments = 0;
    mPickState = false;

    mOldColorBuffer.clear();
    mOldColorManager = ColorManager();
    mNewColors.clear();
    mHoveredTriangleId = {};

    mSegmentToTriangleIds.clear();
    mTriangleToSegmentMap.clear();
}

void Segmentation::computeSegmentaton() {
    cancel();

    ColorManager& colorManager = mApplication.getCurrentGeometry()->getColorManager();
    mNumberOfSegments = mApplication.getCurrentGeometry()->segmentation(mNumberOfClusters, mSmoothingLambda,
                                                                        mSegmentToTriangleIds, mTriangleToSegmentMap);

    if(mNumberOfSegments > 0) {
        // We only want to save the state if we segmented for the first time
        if(mPickState == false) {
            mPickState = true;
            // Keep the old map, we will restore it later
            mOldColorManager = colorManager;
            // Create a new map to display our new segments
            colorManager = ColorManager(mNumberOfSegments);
            mOldColorBuffer = mApplication.getCurrentGeometry()->getColorBuffer();
        }

        // If we this time segment into more segments than previously
        if(mNewColors.size() < mNumberOfSegments) {
            mNewColors.resize(mNumberOfSegments);
            for(size_t i = 0; i < mNumberOfSegments; ++i) {
                mNewColors[i] = i;
            }
        }
        if(colorManager.size() < mNumberOfSegments) {
            colorManager = ColorManager(mNumberOfSegments);
        }

        assert(mNumberOfSegments != std::numeric_limits<size_t>::max());
        assert(mNumberOfSegments <= PEPR3D_MAX_PALETTE_COLORS);
        assert(colorManager.size() == mNumberOfSegments);

        // Create a new color buffer based on the segmentation
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

void Segmentation::onNewGeometryLoaded(ModelView& modelView) {
    CI_LOG_I("Model changed, segmentation reset");
    reset();
}
}  // namespace pepr3d