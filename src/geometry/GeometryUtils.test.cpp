#ifdef _TEST_

#include <gtest/gtest.h>
#include "geometry/GeometryUtils.h"
using glm::vec3;

TEST(GeometryUtils, segmentPointDistanceSquared) {
    const vec3 start(0, 0, 0);
    const vec3 end(10, 0, 0);

    const vec3 a(-1, 0, 0);
    auto dist = pepr3d::GeometryUtils::segmentPointDistanceSquared(start, end, a);
    EXPECT_FLOAT_EQ(dist, 1.f);

    const vec3 b(5, 2, 0);
    dist = pepr3d::GeometryUtils::segmentPointDistanceSquared(start, end, b);
    EXPECT_FLOAT_EQ(dist, 4.f);

    dist = pepr3d::GeometryUtils::segmentPointDistanceSquared(start, end, end);
    EXPECT_FLOAT_EQ(dist, 0.f);
}

#endif