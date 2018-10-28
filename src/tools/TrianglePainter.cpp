#include "tools/TrianglePainter.h"
#include "ui/MainApplication.h"

namespace pepr3d {

void TrianglePainter::drawToSidePane(SidePane& sidePane) {
    sidePane.drawText("Last click:\nX: " + std::to_string(mLastClick.x) + "\nY: " + std::to_string(mLastClick.y));
}

void TrianglePainter::onModelViewMouseDown(ModelView& modelView, ci::app::MouseEvent event) {
    if(!event.isLeftDown()) {
        return;
    }
    mLastClick = event.getPos();
    mLastRay = modelView.getRayFromWindowCoordinates(event.getPos());
    auto geometry = mApplication.getCurrentGeometry();
    if(geometry == nullptr) {
        return;
    }
    auto previousTriangle = mSelectedTriangleId;
    mSelectedTriangleId = geometry->intersectMesh(mLastRay);
    if(previousTriangle == mSelectedTriangleId) {
        return;  // no change
    }
    if(previousTriangle) {
        geometry->setTriangleColor(*previousTriangle, mSelectedTriangleOriginalColor);
    }
    if(mSelectedTriangleId) {
        mSelectedTriangleOriginalColor = geometry->getTriangleColor(*mSelectedTriangleId);
        auto inverseColor = ci::ColorA::white() - mSelectedTriangleOriginalColor;
        inverseColor[3] = 1.0f;
        geometry->setTriangleColor(*mSelectedTriangleId, inverseColor);
    }
}

void TrianglePainter::onModelViewMouseDrag(ModelView& modelView, ci::app::MouseEvent event) {
    onModelViewMouseDown(modelView, event);
}

}  // namespace pepr3d