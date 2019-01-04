#include "tools/PaintBucket.h"
#include "commands/CmdPaintSingleColor.h"
#include "ui/MainApplication.h"

namespace pepr3d {

void PaintBucket::drawToSidePane(SidePane &sidePane) {
    if(!mGeometryCorrect) {
        sidePane.drawText("Polyhedron not built, since the geometry was damaged. Tool disabled.");
        return;
    }

    sidePane.drawColorPalette();
    sidePane.drawSeparator();

    sidePane.drawCheckbox("Paint while dragging", mShouldPaintWhileDrag);
    sidePane.drawTooltipOnHover("When enabled, you can drag your mouse over regions to continuously color them.");

    if(ImGui::RadioButton("Color whole model", mDoNotStop)) {
        mDoNotStop = true;
        mStopOnColor = false;
        mStopOnNormal = false;
    }
    sidePane.drawTooltipOnHover("When enabled, the selected color will always be applied to the whole model.");

    if(ImGui::RadioButton("Color based on criteria", !mDoNotStop)) {
        mDoNotStop = false;
        mStopOnColor = true;  // default
        mStopOnNormal = false;
    }
    sidePane.drawTooltipOnHover("When enabled, coloring will stop on boundaries based on the selected criteria.");

    if(!mDoNotStop) {
        if(ImGui::Checkbox("Stop on different color", &mStopOnColor)) {
            if(!mStopOnColor && !mStopOnNormal) {
                mStopOnNormal = true;  // make sure at least 1 option is selected
            }
        }
        sidePane.drawTooltipOnHover("When enabled, coloring will stop on a boundary with a different color.");
        if(ImGui::Checkbox("Stop on sharp edges", &mStopOnNormal)) {
            if(!mStopOnColor && !mStopOnNormal) {
                mStopOnColor = true;  // make sure at least 1 option is selected
            }
        }
        sidePane.drawTooltipOnHover("When enabled, coloring will stop on a boundary with a sharp edge.");
        if(mStopOnNormal) {
            sidePane.drawIntDragger("Maximum angle", mStopOnNormalDegrees, 0.25f, 0, 180, "%.0f°", 40.0f);
            sidePane.drawTooltipOnHover(
                "The maximum angle (in degrees) that the coloring can pass through on a sharp edge.");
            sidePane.drawText("Angles to compare:");
            if(ImGui::RadioButton("With starting triangle", mNormalCompare == NormalAngleCompare::ABSOLUTE)) {
                mNormalCompare = NormalAngleCompare::ABSOLUTE;
            }
            if(ImGui::RadioButton("Neighbouring triangles", mNormalCompare == NormalAngleCompare::NEIGHBOURS)) {
                mNormalCompare = NormalAngleCompare::NEIGHBOURS;
            }
        }
    }
    sidePane.drawSeparator();
}

void PaintBucket::drawToModelView(ModelView &modelView) {
    if(mHoveredTriangleId && mGeometryCorrect) {
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
    const auto geometry = mApplication.getCurrentGeometry();
    if(geometry == nullptr) {
        return;
    }
    if(!mGeometryCorrect) {
        return;
    }

    const double angleRads = mStopOnNormalDegrees * glm::pi<double>() / 180.0;
    const NormalStopping normalFtor(geometry, glm::cos(angleRads),
                                    geometry->getTriangle(*mHoveredTriangleId).getNormal(), mNormalCompare);

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
        CommandManager<Geometry> *const commandManager = mApplication.getCommandManager();
        commandManager->execute(std::make_unique<CmdPaintSingleColor>(std::move(trianglesToPaint), currentColorIndex),
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
    if(!mGeometryCorrect) {
        return;
    }
    const ci::Ray cameraRay = modelView.getRayFromWindowCoordinates(event.getPos());
    const auto geometry = mApplication.getCurrentGeometry();
    if(geometry == nullptr) {
        mHoveredTriangleId = {};
        return;
    }
    mHoveredTriangleId = safeIntersectMesh(mApplication, cameraRay);
}

void PaintBucket::onNewGeometryLoaded(ModelView &modelView) {
    mGeometryCorrect = mApplication.getCurrentGeometry()->polyhedronValid();
    mHoveredTriangleId = {};
}

}  // namespace pepr3d