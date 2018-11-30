#include "tools/SemiautomaticSegmentation.h"

namespace pepr3d {

void SemiautomaticSegmentation::drawToSidePane(SidePane& sidePane) {
    sidePane.drawColorPalette(mApplication.getCurrentGeometry()->getColorManager());
    sidePane.drawSeparator();

    if(!mStartingTriangles.empty()) {
        for(const auto& colorTris : mStartingTriangles) {
            sidePane.drawText("Tri: " + std::to_string(colorTris.first) +
                              " color: " + std::to_string(colorTris.second));
        }

        if(sidePane.drawButton("Cancel")) {
            reset();
        }
    }
}

void SemiautomaticSegmentation::drawToModelView(ModelView& modelView) {
    if(mHoveredTriangleId) {
        modelView.drawTriangleHighlight(*mHoveredTriangleId);
    }
}

void SemiautomaticSegmentation::setupOverride() {
    std::vector<glm::vec4> newOverrideBuffer;
    const auto currentGeometry = mApplication.getCurrentGeometry();
    newOverrideBuffer.resize(currentGeometry->getTriangleCount() * 3);
    for(size_t i = 0; i < currentGeometry->getTriangleCount(); i++) {
        const size_t triangleColor = currentGeometry->getTriangleColor(i);
        const auto rgbTriangleColor = currentGeometry->getColorManager().getColor(triangleColor);
        newOverrideBuffer[3 * i] = rgbTriangleColor;
        newOverrideBuffer[3 * i + 1] = rgbTriangleColor;
        newOverrideBuffer[3 * i + 2] = rgbTriangleColor;
    }

    for(const auto& repaint : mStartingTriangles) {
        const size_t triIndex = repaint.first;
        const size_t colorIndex = repaint.second;
        const auto rgbTriangleColor = currentGeometry->getColorManager().getColor(colorIndex);
        newOverrideBuffer[3 * triIndex] = rgbTriangleColor;
        newOverrideBuffer[3 * triIndex + 1] = rgbTriangleColor;
        newOverrideBuffer[3 * triIndex + 2] = rgbTriangleColor;
    }

    mApplication.getModelView().getOverrideColorBuffer() = newOverrideBuffer;
    mApplication.getModelView().setColorOverride(true);
}

void SemiautomaticSegmentation::setTriangleColor() {
    const auto activeColor = mApplication.getCurrentGeometry()->getColorManager().getActiveColorIndex();
    auto findSameColorList = mStartingTriangles.find(*mHoveredTriangleId);

    // If we don't have this triangle assigned, or it is assigned to a different color, reassign.
    if(findSameColorList == mStartingTriangles.end() ||
       (findSameColorList != mStartingTriangles.end() && findSameColorList->second != activeColor)) {
        size_t triIndex = *mHoveredTriangleId;

        if(findSameColorList == mStartingTriangles.end()) {
            mStartingTriangles.insert({*mHoveredTriangleId, activeColor});
        } else {
            findSameColorList->second = activeColor;
        }

        if(mApplication.getModelView().isColorOverride()) {
            const auto rgbTriangleColor = mApplication.getCurrentGeometry()->getColorManager().getColor(activeColor);
            auto& overrideBuffer = mApplication.getModelView().getOverrideColorBuffer();
            overrideBuffer[3 * triIndex] = rgbTriangleColor;
            overrideBuffer[3 * triIndex + 1] = rgbTriangleColor;
            overrideBuffer[3 * triIndex + 2] = rgbTriangleColor;
        }
    }
}

void SemiautomaticSegmentation::onModelViewMouseDown(ModelView& modelView, ci::app::MouseEvent event) {
    if(!event.isLeftDown()) {
        return;
    }
    if(!mHoveredTriangleId) {
        return;
    }
    auto geometry = mApplication.getCurrentGeometry();
    if(geometry == nullptr) {
        return;
    }

    const bool emptyBefore = mStartingTriangles.empty();

    setTriangleColor();

    // Added the first triangle, override the buffer.
    if(emptyBefore && !mStartingTriangles.empty()) {
        setupOverride();
    }
}

void SemiautomaticSegmentation::onModelViewMouseDrag(ModelView& modelView, ci::app::MouseEvent event) {
    onModelViewMouseMove(modelView, event);
    onModelViewMouseDown(modelView, event);
}

void SemiautomaticSegmentation::onModelViewMouseMove(ModelView& modelView, ci::app::MouseEvent event) {
    const auto lastRay = modelView.getRayFromWindowCoordinates(event.getPos());
    const auto geometry = mApplication.getCurrentGeometry();
    if(geometry == nullptr) {
        mHoveredTriangleId = {};
        return;
    }
    mHoveredTriangleId = geometry->intersectMesh(lastRay);
}

void SemiautomaticSegmentation::reset() {
    mHoveredTriangleId = {};
    mStartingTriangles.clear();
}

}  // namespace pepr3d