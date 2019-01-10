#ifdef _TEST_

#include <gtest/gtest.h>

#include "geometry/Geometry.h"

pepr3d::Geometry getGeometryWithCube() {
    std::vector<pepr3d::DataTriangle> triangles;
    // cube
    triangles.emplace_back(glm::vec3(-0.5, 0.5, -0.5), glm::vec3(-0.5, 0.5, 0.5), glm::vec3(0.5, 0.5, -0.5),
                           glm::vec3(0, 1, 0), 0);  // top
    triangles.emplace_back(glm::vec3(0.5, 0.5, -0.5), glm::vec3(0.5, 0.5, 0.5), glm::vec3(-0.5, 0.5, 0.5),
                           glm::vec3(0, 1, 0), 0);
    triangles.emplace_back(glm::vec3(-0.5, -0.5, -0.5), glm::vec3(-0.5, -0.5, 0.5), glm::vec3(0.5, -0.5, -0.5),
                           glm::vec3(0, -1, 0), 0);  // bottom
    triangles.emplace_back(glm::vec3(0.5, -0.5, -0.5), glm::vec3(0.5, -0.5, 0.5), glm::vec3(-0.5, -0.5, 0.5),
                           glm::vec3(0, -1, 0), 0);
    triangles.emplace_back(glm::vec3(0.5, -0.5, 0.5), glm::vec3(0.5, -0.5, -0.5), glm::vec3(0.5, 0.5, 0.5),
                           glm::vec3(1, 0, 0), 0);  // front
    triangles.emplace_back(glm::vec3(0.5, -0.5, -0.5), glm::vec3(0.5, 0.5, -0.5), glm::vec3(0.5, 0.5, 0.5),
                           glm::vec3(1, 0, 0), 0);
    triangles.emplace_back(glm::vec3(-0.5, -0.5, 0.5), glm::vec3(-0.5, -0.5, -0.5), glm::vec3(-0.5, 0.5, 0.5),
                           glm::vec3(-1, 0, 0), 0);  // back
    triangles.emplace_back(glm::vec3(-0.5, -0.5, -0.5), glm::vec3(-0.5, 0.5, -0.5), glm::vec3(-0.5, 0.5, 0.5),
                           glm::vec3(-1, 0, 0), 0);
    triangles.emplace_back(glm::vec3(-0.5, -0.5, 0.5), glm::vec3(0.5, -0.5, 0.5), glm::vec3(-0.5, 0.5, 0.5),
                           glm::vec3(0, 0, 1), 0);  // left
    triangles.emplace_back(glm::vec3(0.5, -0.5, 0.5), glm::vec3(0.5, 0.5, 0.5), glm::vec3(-0.5, 0.5, 0.5),
                           glm::vec3(0, 0, 1), 0);
    triangles.emplace_back(glm::vec3(-0.5, -0.5, -0.5), glm::vec3(0.5, -0.5, -0.5), glm::vec3(-0.5, 0.5, -0.5),
                           glm::vec3(0, 0, -1), 0);  // right
    triangles.emplace_back(glm::vec3(0.5, -0.5, -0.5), glm::vec3(0.5, 0.5, -0.5), glm::vec3(-0.5, 0.5, -0.5),
                           glm::vec3(0, 0, -1), 0);
    return pepr3d::Geometry(std::move(triangles));
}

TEST(Geometry, initialize) {
    pepr3d::Geometry geo(getGeometryWithCube());
    EXPECT_EQ(geo.getTriangleCount(), 12);

    const auto vertexBuffer = geo.getVertexBuffer();
    EXPECT_EQ(vertexBuffer.size(), 36);
    const auto indexBuffer = geo.getOpenGlData().indexBuffer;
    EXPECT_EQ(indexBuffer.size(), 36);
    const auto normalBuffer = geo.getOpenGlData().normalBuffer;
    EXPECT_EQ(normalBuffer.size(), 36);
    const auto colorBuffer = geo.getOpenGlData().colorBuffer;
    EXPECT_EQ(colorBuffer.size(), 36);
}

TEST(Geometry, getColor) {
    pepr3d::Geometry geo(getGeometryWithCube());

    EXPECT_EQ(geo.getTriangleColor(0), 0);
    EXPECT_EQ(geo.getTriangleColor(1), 0);

    const auto colorBuffer = geo.getOpenGlData().colorBuffer;
    EXPECT_EQ(colorBuffer.size(), 36);
    pepr3d::Geometry::ColorIndex colorIndex = colorBuffer.at(0);

    for(int i = 1; i < 36; ++i) {
        EXPECT_EQ(colorIndex, colorBuffer.at(i));
    }
}

TEST(Geometry, setColor) {
    pepr3d::Geometry geo(getGeometryWithCube());

    EXPECT_EQ(geo.getTriangleColor(0), 0);
    EXPECT_EQ(geo.getTriangleColor(1), 0);

    auto& colorBuffer = geo.getOpenGlData().colorBuffer();
    EXPECT_EQ(colorBuffer.size(), 36);
    const pepr3d::Geometry::ColorIndex colorIndex = colorBuffer.at(0);

    geo.setTriangleColor(1, 3);

    EXPECT_NE(colorBuffer.at(0), colorBuffer.at(4));
    const pepr3d::Geometry::ColorIndex newColorIndex = colorBuffer.at(4);

    for(int i = 0; i < 3; ++i) {
        EXPECT_EQ(colorBuffer.at(i), colorIndex);
    }

    for(int i = 3; i < 6; ++i) {
        EXPECT_EQ(colorBuffer.at(i), newColorIndex);
    }

    for(int i = 7; i < 36; ++i) {
        EXPECT_EQ(colorBuffer.at(i), colorIndex);
    }
}
#endif