#ifdef _TEST_

#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Polygon_2.h>
#include <gtest/gtest.h>
#include "geometry/GeometryUtils.h"
using glm::vec3;
using pepr3d::GeometryUtils;

namespace pepr3d {
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

TEST(GeometryUtils, ShoelaceOrientationTest) {
    using K = CGAL::Exact_predicates_exact_constructions_kernel;
    using Polygon = CGAL::Polygon_2<K>;
    using Point2 = CGAL::Point_2<K>;

    Polygon pgn;

    pgn.push_back(Point2(0, 0));
    pgn.push_back(Point2(1, 0));
    pgn.push_back(Point2(1, 1));

    ASSERT_TRUE(pgn.is_counterclockwise_oriented());
    EXPECT_TRUE(GeometryUtils::shoelaceOrientationTest(pgn) == CGAL::COUNTERCLOCKWISE);

    pgn.reverse_orientation();
    ASSERT_TRUE(pgn.is_clockwise_oriented());
    EXPECT_TRUE(GeometryUtils::shoelaceOrientationTest(pgn) == CGAL::CLOCKWISE);
}

TEST(GeometryUtils, SimplifyPolygon) {
    using K = CGAL::Simple_cartesian<double>;
    using Polygon = CGAL::Polygon_2<K>;
    using Point2 = CGAL::Point_2<K>;

    Polygon pgn;
    pgn.push_back(Point2(1, 1));
    pgn.push_back(Point2(0, 0));
    pgn.push_back(Point2(0.5, 0));
    pgn.push_back(Point2(1, 0));
    pgn.push_back(Point2(1, 0.25));
    pgn.push_back(Point2(1, 0.35));

    ASSERT_TRUE(pgn.is_simple());
    ASSERT_TRUE(pgn.is_counterclockwise_oriented());

    ASSERT_EQ(pgn.size(), 6);

    const bool returnVal = GeometryUtils::simplifyPolygon(pgn);
    EXPECT_TRUE(returnVal);

    EXPECT_EQ(pgn.size(), 3);
    EXPECT_TRUE(pgn.is_simple());
    EXPECT_TRUE(pgn.is_counterclockwise_oriented());

    EXPECT_FALSE(GeometryUtils::simplifyPolygon(pgn));
}
}  // namespace pepr3d
#endif