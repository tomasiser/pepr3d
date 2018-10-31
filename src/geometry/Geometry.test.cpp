#ifdef _TEST_

#include <gtest/gtest.h>

#include "geometry/Geometry.h"

TEST(Geometry, initialize) {
    pepr3d::Geometry geo;
    EXPECT_EQ(geo.getTriangleCount(), 2);

    const auto vertexBuffer = geo.getVertexBuffer();
    EXPECT_EQ(vertexBuffer.size(), 6);
    const auto indexBuffer = geo.getIndexBuffer();
    EXPECT_EQ(indexBuffer.size(), 6);
    const auto normalBuffer = geo.getNormalBuffer();
    EXPECT_EQ(normalBuffer.size(), 6);
    const auto colorBuffer = geo.getColorBuffer();
    EXPECT_EQ(colorBuffer.size(), 6);
}

TEST(Geometry, getColor) {
    pepr3d::Geometry geo;

    EXPECT_EQ(geo.getTriangleColor(0), 0);
    EXPECT_EQ(geo.getTriangleColor(1), 0);

    const auto colorBuffer = geo.getColorBuffer();
    EXPECT_EQ(colorBuffer.size(), 6);
    ci::ColorA color = colorBuffer.at(0);

    for(int i = 1; i < 6; ++i) {
        EXPECT_EQ(color, colorBuffer.at(i));
    }
}

TEST(Geometry, setColor) {
    pepr3d::Geometry geo;

    EXPECT_EQ(geo.getTriangleColor(0), 0);
    EXPECT_EQ(geo.getTriangleColor(1), 0);

    std::vector<ci::ColorA>& colorBuffer = geo.getColorBuffer();
    EXPECT_EQ(colorBuffer.size(), 6);
    const ci::ColorA color = colorBuffer.at(0);

    geo.setTriangleColor(1, 3);

    EXPECT_NE(colorBuffer.at(0), colorBuffer.at(4));
    const ci::ColorA newColor = colorBuffer.at(4);

    for(int i = 0; i < 3; ++i) {
        EXPECT_EQ(colorBuffer.at(i), color);
    }

    for(int i = 3; i < 6; ++i) {
        EXPECT_EQ(colorBuffer.at(i), newColor);
    }
}
#endif