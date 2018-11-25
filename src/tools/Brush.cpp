#include "Brush.h"
#include "ui/ModelView.h"
#include "geometry/Geometry.h"
#include "ui/MainApplication.h"

void pepr3d::Brush::onModelViewMouseDown(ModelView& modelView, ci::app::MouseEvent event) {
    paint();
}

void pepr3d::Brush::onModelViewMouseUp(ModelView& modelView, ci::app::MouseEvent event) {
    stopPaint();
}

void pepr3d::Brush::onModelViewMouseDrag(ModelView& modelView, ci::app::MouseEvent event) {
    mLastRay = modelView.getRayFromWindowCoordinates(event.getPos());
    paint();
    updateHighlight();
}

void pepr3d::Brush::onModelViewMouseMove(ModelView& modelView, ci::app::MouseEvent event) {
    mLastRay = modelView.getRayFromWindowCoordinates(event.getPos());
    updateHighlight();
}

void pepr3d::Brush::paint() {
}

void pepr3d::Brush::stopPaint() {
}

void pepr3d::Brush::updateHighlight() const {
    mApplication.getCurrentGeometry()->highlightArea(mLastRay, mBrushSettings);
}
