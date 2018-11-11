#include "tools/TrianglePainter.h"
#include <random>
#include "geometry/Geometry.h"
#include "ui/MainApplication.h"

namespace pepr3d {

void TrianglePainter::drawToSidePane(SidePane& sidePane) {
    sidePane.drawColorPalette(mApplication.getCurrentGeometry()->getColorManager());
    sidePane.drawSeparator();

    sidePane.drawText("Last click:\nX: " + std::to_string(mLastClick.x) + "\nY: " + std::to_string(mLastClick.y));
    const size_t triSize = mApplication.getCurrentGeometry()->getTriangleCount();
    sidePane.drawText("Number of triangles: " + std::to_string(triSize) + "\n");
    if(mHoveredTriangleId) {
        sidePane.drawText("Hovered triangle ID: " + std::to_string(*mHoveredTriangleId) + "\n");
    }
}

void TrianglePainter::drawToModelView(ModelView& modelView) {
    if (mHoveredTriangleId) {
        modelView.drawTriangleHighlight(*mHoveredTriangleId);
    }
}

void TrianglePainter::onModelViewMouseDown(ModelView& modelView, ci::app::MouseEvent event) {
    if(!event.isLeftDown()) {
        return;
    }
    mLastClick = event.getPos();
    if(!mHoveredTriangleId) {
        return;
    }
    auto geometry = mApplication.getCurrentGeometry();
    if(geometry == nullptr) {
        return;
    }
    geometry->setTriangleColor(*mHoveredTriangleId, geometry->getColorManager().getActiveColorIndex());
}

void TrianglePainter::onModelViewMouseDrag(ModelView& modelView, ci::app::MouseEvent event) {
    onModelViewMouseMove(modelView, event);
    onModelViewMouseDown(modelView, event);
}

void TrianglePainter::onModelViewMouseMove(ModelView& modelView, ci::app::MouseEvent event) {
    mLastRay = modelView.getRayFromWindowCoordinates(event.getPos());
    auto geometry = mApplication.getCurrentGeometry();
    if(geometry == nullptr) {
        mHoveredTriangleId = {};
        return;
    }
    mHoveredTriangleId = geometry->intersectMesh(mLastRay);
}

}  // namespace pepr3d