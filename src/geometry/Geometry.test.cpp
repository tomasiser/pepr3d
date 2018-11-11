#ifdef _TEST_

#include <gtest/gtest.h>

#include "geometry/Geometry.h"

TEST(Geometry, initialize) {
    pepr3d::Geometry geo;
    EXPECT_EQ(geo.getTriangleCount(), 12);

    const auto vertexBuffer = geo.getVertexBuffer();
    EXPECT_EQ(vertexBuffer.size(), 36);
    const auto indexBuffer = geo.getIndexBuffer();
    EXPECT_EQ(indexBuffer.size(), 36);
    const auto normalBuffer = geo.getNormalBuffer();
    EXPECT_EQ(normalBuffer.size(), 36);
    const auto colorBuffer = geo.getColorBuffer();
    EXPECT_EQ(colorBuffer.size(), 36);
}

TEST(Geometry, getColor) {
    pepr3d::Geometry geo;

    EXPECT_EQ(geo.getTriangleColor(0), 0);
    EXPECT_EQ(geo.getTriangleColor(1), 0);

    const auto colorBuffer = geo.getColorBuffer();
    EXPECT_EQ(colorBuffer.size(), 36);
    pepr3d::Geometry::ColorIndex colorIndex = colorBuffer.at(0);

    for(int i = 1; i < 36; ++i) {
        EXPECT_EQ(colorIndex, colorBuffer.at(i));
    }
}

TEST(Geometry, setColor) {
    pepr3d::Geometry geo;

    EXPECT_EQ(geo.getTriangleColor(0), 0);
    EXPECT_EQ(geo.getTriangleColor(1), 0);

    auto& colorBuffer = geo.getColorBuffer();
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