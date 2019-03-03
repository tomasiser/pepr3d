#include "commands/CmdPaintBrush.h"
#include "geometry/Geometry.h"
#include "ui/MainApplication.h"
#include "ui/ModelView.h"

namespace pepr3d {
void Brush::onModelViewMouseDown(ModelView& modelView, ci::app::MouseEvent event) {
    if(!event.isLeft()) {
        return;
    }
    mLastRay = modelView.getRayFromWindowCoordinates(event.getPos());
    paint();
}

void Brush::onModelViewMouseUp(ModelView& modelView, ci::app::MouseEvent event) {
    if(!event.isLeft()) {
        return;
    }
    stopPaint();
}

void Brush::onModelViewMouseDrag(ModelView& modelView, ci::app::MouseEvent event) {
    if(!event.isLeftDown()) {
        return;
    }
    updateRay(modelView, event);
    paint();
    updateHighlight(modelView, event);
}

void Brush::onModelViewMouseMove(ModelView& modelView, ci::app::MouseEvent event) {
    mLastRay = modelView.getRayFromWindowCoordinates(event.getPos());

    updateHighlight(modelView, event);
}

void Brush::onToolSelect(ModelView& modelView) {
    mPaintedAnything = false;
}

void Brush::onToolDeselect(ModelView& modelView) {
    mApplication.getCurrentGeometry()->hideHighlight();
    if(mPaintedAnything) {
        mApplication.getCurrentGeometry()->updateTemporaryDetailedData();
    }
}

void Brush::paint() {
    // Prevents blocking the rendering if painting takes too long
    if(mPaintsSinceDraw >= MAX_PAINTS_WITHOUT_DRAW) {
        return;
    }

    mPaintsSinceDraw++;

    mBrushSettings.color = mApplication.getCurrentGeometry()->getColorManager().getActiveColorIndex();
    auto* commandManager = mApplication.getCommandManager();
    if(commandManager) {
        commandManager->execute(std::make_unique<CmdPaintBrush>(mLastRay, mBrushSettings), mGroupCommands);
    }

    mGroupCommands = true;
    mPaintedAnything = true;
}

void Brush::stopPaint() {
    mGroupCommands = false;
}

void Brush::updateHighlight(ModelView& modelView, ci::app::MouseEvent event) const {
    if(mBrushSettings.spherical) {
        mApplication.getCurrentGeometry()->highlightArea(mLastRay, mBrushSettings);
    } else {
    }
}

void Brush::updateRay(ModelView& modelView, ci::app::MouseEvent event) {
    mLastRay = modelView.getRayFromWindowCoordinates(event.getPos());
    auto intersection = mApplication.getCurrentGeometry()->intersectMesh(mLastRay, mLastIntersection);
}

bool Brush::isEnabled() const {
    return mApplication.getCurrentGeometry()->polyhedronValid();
}

void Brush::drawToSidePane(SidePane& sidePane) {
    sidePane.drawColorPalette();
    sidePane.drawSeparator();

    sidePane.drawFloatDragger("Size", mBrushSettings.size, mMaxSize / SIZE_SLIDER_STEPS, 0.0001f, mMaxSize, "%.02f",
                              140.f);

    sidePane.drawIntDragger("Segments", mBrushSettings.segments, 0.1, 3, 50, "%d", 140.f);
    sidePane.drawTooltipOnHover("Higher number of segments increases \"roundness\" of the brush.");

    sidePane.drawCheckbox("Paint backfaces", mBrushSettings.paintBackfaces);
    sidePane.drawTooltipOnHover("Paint triangles even if they are facing away from the camera.");

    sidePane.drawSeparator();
    sidePane.drawText("Brush shape:");
    if(ImGui::RadioButton("Sphere", mBrushSettings.spherical)) {
        mBrushSettings.spherical = true;
    }
    sidePane.drawTooltipOnHover(
        "Spherical brush paints everything within a radius, and will create additional edges to smooth transitions "
        "over triangle boundaries.");
    if(ImGui::RadioButton("Flat", !mBrushSettings.spherical)) {
        mBrushSettings.spherical = false;
        mApplication.getCurrentGeometry()->hideHighlight();
    }
    sidePane.drawTooltipOnHover(
        "Flat brush paints the shape directly to the surface, ignoring any distance limitations.");

    if(mBrushSettings.spherical) {
        sidePane.drawText("Spherical brush settings");

        sidePane.drawCheckbox("Continuous", mBrushSettings.continuous);
        sidePane.drawTooltipOnHover(
            "Paint only triangles that are connected inside the painting radius. This prevents accidentaly painting "
            "two parts connected only via an air-gap.");

        sidePane.drawCheckbox("Respect original triangles", mBrushSettings.respectOriginalTriangles);
        sidePane.drawTooltipOnHover(
            "Prevent brush from creating new triangles. This makes Brush tool behave like a Triangle Painter tool with "
            "a radius.");

        if(mBrushSettings.respectOriginalTriangles) {
            sidePane.drawCheckbox("Paint outer ring", mBrushSettings.paintOuterRing);
            sidePane.drawTooltipOnHover("Paint the whole triangle, even if it is not fully inside the brush.");
        }
    }

    if(!mBrushSettings.spherical) {
        sidePane.drawText("Flat brush settings");
        sidePane.drawCheckbox("Align to normal", mBrushSettings.alignToNormal);
        sidePane.drawTooltipOnHover("Align painting direction to the surface normal.");
    }
    sidePane.drawSeparator();
    mPaintsSinceDraw = 0;
}

void Brush::drawToModelView(ModelView& modelView) {
    Geometry* geometry = mApplication.getCurrentGeometry();
    if(geometry) {
        if(mBrushSettings.respectOriginalTriangles && geometry->getAreaHighlight().enabled) {
            for(size_t triIdx : geometry->getAreaHighlight().triangles) {
                modelView.drawTriangleHighlight(triIdx);
            }
        }
    }
}

void Brush::onNewGeometryLoaded(ModelView& modelView) {
    mMaxSize = modelView.getMaxSize();
    mBrushSettings.size = mMaxSize / 10;
}
}  // namespace pepr3d
