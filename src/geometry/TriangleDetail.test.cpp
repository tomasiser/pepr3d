#ifdef _TEST_
#include <gtest/gtest.h>

#define assert(x) ASSERT_TRUE(x);

#include "geometry/GeometryUtils.h"
#include "geometry/TriangleDetail.h"

#include <CGAL/Boolean_set_operations_2.h>
#include <CGAL/Gps_traits_2.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Polygon_set_2.h>
#include <CGAL/Spherical_kernel_intersections.h>
#include <CGAL/partition_2.h>

#include <cereal/archives/json.hpp>
#include <random>
#include <set>

namespace pepr3d {
using Point2 = TriangleDetail::Point2;
using PolygonWithHoles = TriangleDetail::PolygonWithHoles;
using Polygon = TriangleDetail::Polygon;
using Point2 = TriangleDetail::Point2;

TEST(TriangleDetail, ConstructionSteps) {
    auto mTraits = std::make_unique<TriangleDetail::PolygonSet::Traits_2>();

    const size_t a = sizeof(TriangleDetail::PolygonSet::Traits_2);

    TriangleDetail::Polygon pgn;
    pgn.push_back(Point2(-0.5, -0.5));
    pgn.push_back(Point2(0.5, -0.5));
    pgn.push_back(Point2(-0.5, 0.5));

    ASSERT_TRUE(pgn.is_simple());
    ASSERT_TRUE(pgn.is_counterclockwise_oriented());
    ASSERT_TRUE(CGAL::is_valid_polygon(pgn, *mTraits));

    // We have a completely valid polygon now
    // Lets add it to a PolygonSet

    TriangleDetail::PolygonSet pset(pgn);

    // Are polygons of this set valid?
    std::vector<PolygonWithHoles> polygonsWithHoles(pset.number_of_polygons_with_holes());
    pset.polygons_with_holes(polygonsWithHoles.begin());

    for(const PolygonWithHoles& poly : polygonsWithHoles) {
        EXPECT_TRUE(CGAL::is_valid_polygon_with_holes(poly, *mTraits));
    }
}

template <typename CGalType>
CGalType cgalTypeFromString(const std::string& str) {
    std::stringstream stream(str);
    CGalType value;
    stream >> value;

    return value;
}

TEST(TriangleDetail, PolygonWithHolesBadOrientation) {
    auto traits = std::make_unique<TriangleDetail::PolygonSet::Traits_2>();

    std::string polygonString(
        "6 -9924612878548874890304733981971/20282409603651670423947251286016 "
        "17188006152006536578823865513759/81129638414606681695789005144064 "
        "-4017296560458822758487345961369/10141204801825835211973625643008 "
        "3667759669370854531428921126399/40564819207303340847894502572032 "
        "-14112493806255234993030698361967/40564819207303340847894502572032 "
        "2857271125160752150753227128691/40564819207303340847894502572032 "
        "-9924612878548874890304733981971/20282409603651670423947251286016 "
        "17188006152006536578823865513759/81129638414606681695789005144064 "
        "-9519368606443823699966886983117/20282409603651670423947251286016 "
        "6637310640423212698853225948775/40564819207303340847894502572032 "
        "-8874718064193764428030739181071/20282409603651670423947251286016 "
        "1239265188467743494005293520325/10141204801825835211973625643008  0");

    PolygonWithHoles poly = cgalTypeFromString<PolygonWithHoles>(polygonString);

    EXPECT_TRUE(CGAL::is_valid_polygon_with_holes(poly, *traits));
}

TEST(TriangleDetail, PolygonMerging) {
    /**
     *  These two valid polygons when merged form a polygon set with invalid PolygonWithHoles
     *  when using unpatched CGAL Gps_traits_adaptor.h
     */
    using K = CGAL::Exact_predicates_exact_constructions_kernel;
    using PolygonSet = CGAL::Polygon_set_2<K>;
    using Polygon = CGAL::Polygon_2<K>;
    using PolygonWithHoles = CGAL::Polygon_with_holes_2<K>;

    auto traits = std::make_unique<PolygonSet::Traits_2>();

    std::string firstPolyString(
        "3 -9924612878548874890304733981971/20282409603651670423947251286016 "
        "17188006152006536578823865513759/81129638414606681695789005144064 "
        "-9519368606443823699966886983117/20282409603651670423947251286016 "
        "6637310640423212698853225948775/40564819207303340847894502572032 "
        "-8874718064193764428030739181071/20282409603651670423947251286016 "
        "1239265188467743494005293520325/10141204801825835211973625643008");

    std::string secondPolyString(
        "3 -14112493806255234993030698361967/40564819207303340847894502572032 "
        "2857271125160752150753227128691/40564819207303340847894502572032 "
        "-9924612878548874890304733981971/20282409603651670423947251286016 "
        "17188006152006536578823865513759/81129638414606681695789005144064 "
        "-4017296560458822758487345961369/10141204801825835211973625643008 "
        "3667759669370854531428921126399/40564819207303340847894502572032");

    Polygon firstPoly(cgalTypeFromString<Polygon>(firstPolyString));
    Polygon secondPoly(cgalTypeFromString<Polygon>(secondPolyString));

    ASSERT_TRUE(CGAL::is_valid_unknown_polygon(firstPoly, *traits));
    ASSERT_TRUE(CGAL::is_valid_unknown_polygon(secondPoly, *traits));

    // Create polygon set form the first polygon
    PolygonSet pSet;
    pSet.join(firstPoly);

    // Test that polygonset is made of valid polygons
    {
        std::vector<PolygonWithHoles> polys(pSet.number_of_polygons_with_holes());
        pSet.polygons_with_holes(polys.begin());
        for(PolygonWithHoles& poly : polys) {
            EXPECT_TRUE(CGAL::is_valid_polygon_with_holes(poly, *traits));
        }
    }

    // Join second polygon
    pSet.join(secondPoly);

    // Test that polygonset is made of valid polygons
    {
        std::vector<PolygonWithHoles> polys(pSet.number_of_polygons_with_holes());
        pSet.polygons_with_holes(polys.begin());
        for(PolygonWithHoles& poly : polys) {
            EXPECT_TRUE(CGAL::is_valid_polygon_with_holes(poly, *traits));
        }
    }
}

TEST(TriangleDetail, PolygonMergingShoelaceOrientationTest) {
    /**
     *  Tests validy of merged polygons when using shoelace orientation test instead of default CGAL orientation test.
     */
    using K = CGAL::Exact_predicates_exact_constructions_kernel;
    using PolygonSet = CGAL::Polygon_set_2<K>;
    using Polygon = CGAL::Polygon_2<K>;
    using PolygonWithHoles = CGAL::Polygon_with_holes_2<K>;

    auto traits = std::make_unique<PolygonSet::Traits_2>();

    std::string firstPolyString(
        "3 -9924612878548874890304733981971/20282409603651670423947251286016 "
        "17188006152006536578823865513759/81129638414606681695789005144064 "
        "-9519368606443823699966886983117/20282409603651670423947251286016 "
        "6637310640423212698853225948775/40564819207303340847894502572032 "
        "-8874718064193764428030739181071/20282409603651670423947251286016 "
        "1239265188467743494005293520325/10141204801825835211973625643008");

    std::string secondPolyString(
        "3 -14112493806255234993030698361967/40564819207303340847894502572032 "
        "2857271125160752150753227128691/40564819207303340847894502572032 "
        "-9924612878548874890304733981971/20282409603651670423947251286016 "
        "17188006152006536578823865513759/81129638414606681695789005144064 "
        "-4017296560458822758487345961369/10141204801825835211973625643008 "
        "3667759669370854531428921126399/40564819207303340847894502572032");

    Polygon firstPoly(cgalTypeFromString<Polygon>(firstPolyString));
    Polygon secondPoly(cgalTypeFromString<Polygon>(secondPolyString));

    ASSERT_TRUE(CGAL::is_valid_unknown_polygon(firstPoly, *traits));
    ASSERT_TRUE(CGAL::is_valid_unknown_polygon(secondPoly, *traits));

    // Create polygon set form the first polygon
    PolygonSet pSet;
    pSet.join(firstPoly);

    // Test that polygonset is made of valid polygons
    {
        std::vector<PolygonWithHoles> polys(pSet.number_of_polygons_with_holes());
        pSet.polygons_with_holes(polys.begin());
        for(PolygonWithHoles& poly : polys) {
            EXPECT_TRUE(GeometryUtils::is_valid_polygon_with_holes(poly, *traits));
        }
    }

    // Join second polygon
    pSet.join(secondPoly);

    // Test that polygonset is made of valid polygons
    {
        std::vector<PolygonWithHoles> polys(pSet.number_of_polygons_with_holes());
        pSet.polygons_with_holes(polys.begin());
        for(PolygonWithHoles& poly : polys) {
            EXPECT_TRUE(GeometryUtils::is_valid_polygon_with_holes(poly, *traits));
        }
    }
}

TEST(TriangleDetail, AddMissingPoints) {
    using HistoryEntry = TriangleDetail::HistoryEntry;
    using PolygonEntry = TriangleDetail::PolygonEntry;
    using ColorChangeEntry = TriangleDetail::ColorChangeEntry;
    using PointEntry = TriangleDetail::PointEntry;

    std::stringstream peprTriStream("-0.5 -0.5 0.5 0.5 -0.5 0.5 0.5 0.5 0.5");
    TriangleDetail::PeprTriangle peprTri;
    peprTriStream >> peprTri;

    const DataTriangle tri(TriangleDetail::toGlmVec(peprTri.vertex(0)), TriangleDetail::toGlmVec(peprTri.vertex(1)),
                           TriangleDetail::toGlmVec(peprTri.vertex(2)), glm::vec3(1, 0, 0), 0);

    std::ifstream testFile("../tests/addMissingPoints.json", std::ios::in);
    ASSERT_TRUE(testFile.good());

    std::vector<HistoryEntry> history;

    TriangleDetail triDetail(tri);

    {
        cereal::JSONInputArchive jsonArchive(testFile);
        ASSERT_NO_THROW(jsonArchive(history));
    }

    for(size_t i = 0; i < history.size(); i++) {
        HistoryEntry& entry = history[i];

        if(boost::get<PolygonEntry>(&entry)) {
            PolygonEntry& pe = boost::get<PolygonEntry>(entry);
            ASSERT_NO_THROW(triDetail.addPolygon(pe.polygon, pe.color));
            continue;
        }
        if(boost::get<PointEntry>(&entry)) {
            PointEntry& pe = boost::get<PointEntry>(entry);
            ASSERT_NO_THROW(triDetail.addMissingPoints(pe.myPoints, pe.theirPoints, pe.sharedEdge));
            triDetail.updateTrianglesFromPolygons();
            continue;
        }
        if(boost::get<ColorChangeEntry>(&entry)) {
            ColorChangeEntry& ce = boost::get<ColorChangeEntry>(entry);
            ASSERT_NO_THROW(triDetail.setColor(ce.detailIdx, ce.color));
            continue;
        }

        ASSERT_TRUE(false);
    }
}

}  // namespace pepr3d

#endif
