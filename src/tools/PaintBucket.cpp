#include "tools/PaintBucket.h"
#include "commands/CmdPaintSingleColor.h"
#include "ui/MainApplication.h"

namespace pepr3d {

void PaintBucket::drawToSidePane(SidePane &sidePane) {
    sidePane.drawColorPalette(mApplication.getCurrentGeometry()->getColorManager());
    sidePane.drawSeparator();

    sidePane.drawCheckbox("Paint while dragging", mShouldPaintWhileDrag);
    sidePane.drawCheckbox("Color whole model", mDoNotStop);
    if(!mDoNotStop) {
        sidePane.drawCheckbox("Stop on different color", mStopOnColor);
        sidePane.drawCheckbox("Stop on sharp edges", mStopOnNormal);
        if(mStopOnNormal) {
            sidePane.drawIntDragger("Maximum absolute angle", mStopOnNormalDegrees, 0.25f, 0, 180, "%.0f°", 40.0f);
        }
    }
    sidePane.drawSeparator();

    const size_t triSize = mApplication.getCurrentGeometry()->getTriangleCount();
    sidePane.drawText("Number of triangles: " + std::to_string(triSize) + "\n");
    sidePane.drawText("Polyhedron closed 0/1: " + std::to_string(mApplication.getCurrentGeometry()->polyClosedCheck()) +
                      "\n");
    sidePane.drawText("Vertex count: " + std::to_string(mApplication.getCurrentGeometry()->polyVertCount()) + "\n");
}

void PaintBucket::drawToModelView(ModelView &modelView) {
    if(mHoveredTriangleId) {
        modelView.drawTriangleHighlight(*mHoveredTriangleId);
    }
}

void PaintBucket::onModelViewMouseDown(ModelView &modelView, ci::app::MouseEvent event) {
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

    const double angleRads = mStopOnNormalDegrees * glm::pi<double>() / 180.0;
    const NormalStopping normalFtor(geometry, glm::cos(angleRads),
                                    geometry->getTriangle(*mHoveredTriangleId).getNormal());

    const ColorStopping colorFtor(geometry);

    auto combinedCriterion = [&normalFtor, &colorFtor, this](const size_t a, const size_t b) -> bool {
        if(mDoNotStop) {
            return true;
        }
        bool result = true;
        if(mStopOnNormal) {
            result &= normalFtor(a, b);
        }
        if(mStopOnColor) {
            result &= colorFtor(a, b);
        }
        return result;
    };

    std::vector<size_t> trianglesToPaint = geometry->bucket(*mHoveredTriangleId, combinedCriterion);

    if(trianglesToPaint.empty()) {
        return;
    }

    const size_t currentColorIndex = geometry->getColorManager().getActiveColorIndex();
    const bool hoverOverSameTriangle = geometry->getTriangleColor(*mHoveredTriangleId) == currentColorIndex;

    // We only want to re-draw if we are not dragging, or if you are dragging and reached a new region
    if(!mDragging || (mDragging && !hoverOverSameTriangle)) {
        mCommandManager.execute(std::make_unique<CmdPaintSingleColor>(std::move(trianglesToPaint), currentColorIndex),
                                mDragging);
    }
}

void PaintBucket::onModelViewMouseDrag(class ModelView &modelView, ci::app::MouseEvent event) {
    if(mShouldPaintWhileDrag) {
        mDragging = true;
        onModelViewMouseMove(modelView, event);
        onModelViewMouseDown(modelView, event);
        mDragging = false;
    }
}

void PaintBucket::onModelViewMouseMove(ModelView &modelView, ci::app::MouseEvent event) {
    const ci::Ray cameraRay = modelView.getRayFromWindowCoordinates(event.getPos());
    auto geometry = mApplication.getCurrentGeometry();
    if(geometry == nullptr) {
        mHoveredTriangleId = {};
        return;
    }
    mHoveredTriangleId = geometry->intersectMesh(cameraRay);
}

}  // namespace pepr3d