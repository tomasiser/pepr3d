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

    const int angleDegrees = 30;
    const double angleRads = angleDegrees * glm::pi<double>() / 180.0;
    if(selectedTriangleId) {
        const NormalStopping normalFtor(geometry, glm::cos(angleRads),
                                        geometry->getTriangle(*selectedTriangleId).getNormal());

        const ColorStopping colorFtor(geometry);

        bool normalStopOn = true;
        bool colorStopOn = true;
        bool noStopOn = false;

        auto combinedCriterion = [&normalFtor, &colorFtor, normalStopOn, colorStopOn, noStopOn](
                                     const size_t a, const size_t b) -> bool {
            bool result = true;
            if(noStopOn) {
                return true;
            }
            if(normalStopOn) {
                result &= normalFtor(a, b);
            }
            if(colorStopOn) {
                result &= colorFtor(a, b);
            }
            return result;
        };

        geometry->bucket(*selectedTriangleId, combinedCriterion);
    }
}

void PaintBucket::onModelViewMouseDrag(class ModelView &modelView, ci::app::MouseEvent event) {
    onModelViewMouseDown(modelView, event);
}

}  // namespace pepr3d