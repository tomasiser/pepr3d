#include "TrianglePainter.h"

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
}

}  // namespace pepr3d