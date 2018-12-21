#include "tools/LiveDebug.h"
#include "geometry/Geometry.h"
#include "geometry/Triangle.h"
#include "imgui.h"
#include "ui/MainApplication.h"

namespace pepr3d {

template <typename Pt>
double angleBetweenPoints(const Pt& a, const Pt& b, const Pt& c) {
    auto vec1 = a - b;
    auto vec2 = c - b;

    double cosAngle = (vec1 * vec2) / (CGAL::sqrt(vec1.squared_length()) * CGAL::sqrt(vec2.squared_length()));

    return 180 * (acos(cosAngle)) / M_PI;
}

template <typename vec>
string vecToString(const vec& v) {
    return string("Vec\n\tx: ") + to_string(v.x()) + "\n\ty:" + to_string(v.y()) + "\n\tz:" + to_string(v.z());
}

template <typename Tri>
string triAnglesToString(const Tri& tri) {
    auto a = tri.vertex(0);
    auto b = tri.vertex(1);
    auto c = tri.vertex(2);

    auto plane = tri.supporting_plane();
    auto projA = plane.to_2d(a);
    auto projB = plane.to_2d(b);
    auto projC = plane.to_2d(c);

    auto bkA = plane.to_3d(projA);
    auto bkB = plane.to_3d(projB);
    auto bkC = plane.to_3d(projC);

    auto distA = (bkA - a).squared_length();
    auto distB = (bkB - b).squared_length();
    auto distC = (bkC - c).squared_length();

    string angle3DStr = "Angles 3D\n\ta) " + to_string(angleBetweenPoints(a, b, c)) + "\n\tb) " +
                        to_string(angleBetweenPoints(b, c, a)) + "\n\tc) " + to_string(angleBetweenPoints(c, a, b));

    string angle2DStr = "\nAngles 2D\n\ta) " + to_string(angleBetweenPoints(projA, projB, projC)) + "\n\tb) " +
                        to_string(angleBetweenPoints(projB, projC, projA)) + "\n\tc) " +
                        to_string(angleBetweenPoints(projC, projA, projB));

    string normalStr =
        "\n3d Normal " + vecToString(plane.orthogonal_vector() / sqrt(plane.orthogonal_vector().squared_length()));

    string projDist =
        "\nProj Dist:\n\ta) " + to_string(distA) + "\n\tb) " + to_string(distB) + "\n\tc) " + to_string(distC);

    return angle3DStr + angle2DStr + projDist;
}

void LiveDebug::drawToSidePane(SidePane& sidePane) {
    ImGui::BeginChild("##sidepane-livedebug");

    ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth());
    if(sidePane.drawButton("Toggle ImGui demo window")) {
        mApplication.showDemoWindow(!mApplication.isDemoWindowShown());
    }
    ImGui::PopItemWidth();

    sidePane.drawSeparator();
    sidePane.drawText("Model view mouse pos:\nX: " + std::to_string(mMousePos.x) +
                      "\nY: " + std::to_string(mMousePos.y));

    const size_t triSize = mApplication.getCurrentGeometry()->getTriangleCount();
    sidePane.drawText("Vertex count: " + std::to_string(mApplication.getCurrentGeometry()->polyVertCount()) + "\n");
    sidePane.drawText("Triangle count: " + std::to_string(triSize) + "\n");
    sidePane.drawText("Polyhedron valid 0/1: " + std::to_string(mApplication.getCurrentGeometry()->polyhedronValid()) +
                      "\n");

    sidePane.drawSeparator();
    if(mTriangleUnderRay && mApplication.getCurrentGeometry()->getTriangleCount() > mTriangleUnderRay) {
        sidePane.drawText("Triangle debug");
        auto tri = mApplication.getCurrentGeometry()->getTriangle(*mTriangleUnderRay);
        sidePane.drawText(triAnglesToString(tri.getTri()));
    }
    sidePane.drawSeparator();

    static int addedValue = 1;
    ImGui::Text("Current value: %i", mIntegerState.mInnerValue);
    if(mIntegerManager.canUndo()) {
        if(ImGui::Button("Undo")) {
            mIntegerManager.undo();
        }
    }
    if(mIntegerManager.canRedo()) {
        if(ImGui::Button("Redo")) {
            mIntegerManager.redo();
        }
    }
    ImGui::SliderInt("##addedvalue", &addedValue, 1, 10);
    ImGui::SameLine();
    if(ImGui::Button("Add")) {
        mIntegerManager.execute(std::make_unique<AddValueCommand>(addedValue));
    }

    ImGui::EndChild();
}

void LiveDebug::drawToModelView(ModelView& modelView) {
    if(mTriangleUnderRay && mApplication.getCurrentGeometry()->getTriangleCount() > mTriangleUnderRay) {
        modelView.drawTriangleHighlight(*mTriangleUnderRay);

        const auto& triangle = mApplication.getCurrentGeometry()->getTriangle(*mTriangleUnderRay);
        const glm::vec3 center = (triangle.getVertex(0) + triangle.getVertex(1) + triangle.getVertex(2)) / 3.f;

        const auto base1 = triangle.getTri().supporting_plane().base1();
        const auto base2 = triangle.getTri().supporting_plane().base2();

        modelView.drawLine(center, center + glm::vec3(base1.x(), base1.y(), base1.z()), ci::Color(1.f, 0.f, 0.f));
        modelView.drawLine(center, center + glm::vec3(base2.x(), base2.y(), base2.z()), ci::Color(0.f, 1.f, 0.f));
    }
}

void LiveDebug::onModelViewMouseMove(ModelView& modelView, ci::app::MouseEvent event) {
    mMousePos = event.getPos();
    auto ray = modelView.getRayFromWindowCoordinates(event.getPos());
    mTriangleUnderRay = mApplication.getCurrentGeometry()->intersectMesh(ray);
}
}  // namespace pepr3d
