#include "tools/PaintBucket.h"
#include "ui/MainApplication.h"

namespace pepr3d {

void PaintBucket::drawToSidePane(SidePane &sidePane) {
    const size_t triSize = mApplication.getCurrentGeometry()->getTriangleCount();
    sidePane.drawText("Number of triangles: " + std::to_string(triSize) + "\n");
    sidePane.drawText("Polyhedron closed 0/1: " + std::to_string(mApplication.getCurrentGeometry()->polyClosedCheck()) +
                      "\n");
    sidePane.drawText("Vertex count: " + std::to_string(mApplication.getCurrentGeometry()->polyVertCount()) + "\n");
}

void PaintBucket::onModelViewMouseDown(ModelView &modelView, ci::app::MouseEvent event) {
    if(!event.isLeftDown()) {
        return;
    }

    glm::vec2 lastClick = event.getPos();
    const ci::Ray lastRay = modelView.getRayFromWindowCoordinates(event.getPos());
    const auto geometry = mApplication.getCurrentGeometry();
    if(geometry == nullptr) {
        return;
    }
    std::optional<std::size_t> selectedTriangleId = geometry->intersectMesh(lastRay);

    if(selectedTriangleId) {
        const NormalStopping ftor(geometry, 0.866);
        geometry->bucket(*selectedTriangleId, ftor);
    }
}

void PaintBucket::onModelViewMouseDrag(class ModelView &modelView, ci::app::MouseEvent event) {
    onModelViewMouseDown(modelView, event);
}

}  // namespace pepr3d