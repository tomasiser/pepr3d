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
            sidePane.drawTooltipOnHover("The angles are compared with respect to the initial triangle.");
            if(ImGui::RadioButton("Neighbouring triangles", mNormalCompare == NormalAngleCompare::NEIGHBOURS)) {
                mNormalCompare = NormalAngleCompare::NEIGHBOURS;
            }
            sidePane.drawTooltipOnHover(
                "The angles are compared with respect to the actual neighbours of the individual triangles.");
        }
    }
    sidePane.drawSeparator();
}

void PaintBucket::drawToModelView(ModelView &modelView) {
    const ci::Ray cameraRay = modelView.getRayFromWindowCoordinates(mLastMousePos);
    const auto geometry = mApplication.getCurrentGeometry();
    std::optional<DetailedTriangleId> hoveredTriangleId = safeIntersectDetailedMesh(mApplication, cameraRay);

    if(hoveredTriangleId && mGeometryCorrect) {
        modelView.drawTriangleHighlight(*hoveredTriangleId);
    }
}

void PaintBucket::onModelViewMouseDown(ModelView &modelView, ci::app::MouseEvent event) {
    if(!event.isLeftDown()) {
        return;
    }
    const ci::Ray cameraRay = modelView.getRayFromWindowCoordinates(event.getPos());
    const auto geometry = mApplication.getCurrentGeometry();
    if(geometry == nullptr) {
        return;
    }
    if(!mGeometryCorrect) {
        return;
    }

    // Get a fresh triangle intersection so we can be sure the ID is valid if it's set
    std::optional<DetailedTriangleId> hoveredTriangleId = safeIntersectDetailedMesh(mApplication, cameraRay);
    if(!hoveredTriangleId) {
        return;
    }

    const double angleRads = mStopOnNormalDegrees * glm::pi<double>() / 180.0;
    const NormalStopping normalFtor(geometry, glm::cos(angleRads),
                                    geometry->getTriangle(*hoveredTriangleId).getNormal(), mNormalCompare);

    const ColorStopping colorFtor(geometry);

    auto combinedCriterion = [&normalFtor, &colorFtor, this](const DetailedTriangleId a,
                                                             const DetailedTriangleId b) -> bool {
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

    std::vector<DetailedTriangleId> trianglesToPaint = geometry->bucket(*hoveredTriangleId, combinedCriterion);

    try {
        trianglesToPaint = geometry->bucket(*hoveredTriangleId, combinedCriterion);
    } catch(std::exception &e) {
        const std::string errorCaption = "Error: Failed to bucket paint";
        const std::string errorDescription =
            "An internal error occured while bucket painting. If the problem persists, try re-loading the mesh.\n\n"
            "Please report this bug to the developers. The full description of the problem is:\n";
        mApplication.pushDialog(Dialog(DialogType::Error, errorCaption, errorDescription + e.what(), "OK"));
        return;
    }

    if(trianglesToPaint.empty()) {
        return;
    }

    const size_t currentColorIndex = geometry->getColorManager().getActiveColorIndex();
    const bool hoverOverSameTriangle = geometry->getTriangleColor(*hoveredTriangleId) == currentColorIndex;

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

    mLastMousePos = event.getPos();
}

void PaintBucket::onNewGeometryLoaded(ModelView &modelView) {
    mGeometryCorrect = mApplication.getCurrentGeometry()->polyhedronValid();
}

}  // namespace pepr3d