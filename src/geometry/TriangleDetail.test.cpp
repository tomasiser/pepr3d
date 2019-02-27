#ifdef _TEST_
#include <gtest/gtest.h>

#include "geometry/GeometryUtils.h"
#include "geometry/GnuplotDebugHelper.h"
#include "geometry/TriangleDetail.h"

#include <CGAL/Boolean_set_operations_2.h>
#include <CGAL/Gps_traits_2.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Polygon_set_2.h>
#include <CGAL/Spherical_kernel_intersections.h>
#include <CGAL/partition_2.h>

#include <cereal/archives/json.hpp>
#include <cereal/types/map.hpp>
#include <random>
#include <set>

namespace pepr3d {
using Point2 = TriangleDetail::Point2;
using PolygonWithHoles = TriangleDetail::PolygonWithHoles;
using Polygon = TriangleDetail::Polygon;
using PolygonSet = TriangleDetail::PolygonSet;
using Triangle2 = TriangleDetail::Triangle2;

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
     *
     *  Patch available at https://github.com/CGAL/cgal/pull/3688
     */

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

TEST(TriangleDetail, TriangleDetailOperations) {
    /**
     * Runs a long chain of operations on TriangleDetail
     * Makes sure none of them fail debugchecks inserted in TriangleDetail code. (PEPR3D_EDGE_CONSISTENCY_CHECK)
     */
    static_assert(PEPR3D_EDGE_CONSISTENCY_CHECK, "Make sure your preprocessor definitions are set properly.");

    using HistoryEntry = TriangleDetail::HistoryEntry;
    using PolygonEntry = TriangleDetail::PolygonEntry;
    using ColorChangeEntry = TriangleDetail::ColorChangeEntry;
    using PointEntry = TriangleDetail::PointEntry;

    std::stringstream peprTriStream("-0.5 -0.5 0.5 0.5 -0.5 0.5 0.5 0.5 0.5");
    TriangleDetail::PeprTriangle peprTri;
    peprTriStream >> peprTri;

    const DataTriangle tri(TriangleDetail::toGlmVec(peprTri.vertex(0)), TriangleDetail::toGlmVec(peprTri.vertex(1)),
                           TriangleDetail::toGlmVec(peprTri.vertex(2)), glm::vec3(1, 0, 0), 0);

    std::ifstream testFile("./tests/addMissingPoints.json", std::ios::in);
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
            // Original testcase was containing errorneus data.
            // Do not reuse 'myPoints' from the test case
            auto myPoints = triDetail.findPointsOnEdge(pe.sharedEdge);
            ASSERT_NO_THROW(triDetail.addMissingPoints(myPoints, pe.theirPoints, pe.sharedEdge));
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

TEST(TriangleDetail, UpdatePolysFromTriangles) {
    /**
     * Test that updating polygon sets from exact triangles preserves correct edge position
     */
    using DataTriangle = pepr3d::DataTriangle;
    using Triangle2 = TriangleDetail::Triangle2;
    using PolygonSet = TriangleDetail::PolygonSet;

    // Set up the test case
    DataTriangle tri{};
    std::vector<TriangleDetail::ExactTriangle> coloredExactTriangles;

    // Load triangle data form file
    std::ifstream inFile("./tests/updatePolysFromTriangles.json");
    ASSERT_TRUE(inFile.good());
    {
        cereal::JSONInputArchive jsonArchive(inFile);
        ASSERT_NO_THROW(jsonArchive(tri, coloredExactTriangles));
    }

    ASSERT_TRUE(!coloredExactTriangles.empty());

    const TriangleDetail triDetail(tri);
    const Polygon bounds = triDetail.polygonFromTriangle(tri.getTri());

    // Create dummy polygon sets to make sure original triangle edges are traversable
    std::map<size_t, PolygonSet> dummyPolygonSets;
    for(size_t i = 0; i < coloredExactTriangles.size(); i++) {
        dummyPolygonSets[i] = PolygonSet(TriangleDetail::polygonFromTriangle(coloredExactTriangles[i].triangle));
    }

    ASSERT_TRUE(TriangleDetail::isEdgeTraversable(bounds.vertex(0), bounds.vertex(1), dummyPolygonSets));
    ASSERT_TRUE(TriangleDetail::isEdgeTraversable(bounds.vertex(1), bounds.vertex(2), dummyPolygonSets));
    ASSERT_TRUE(TriangleDetail::isEdgeTraversable(bounds.vertex(2), bounds.vertex(0), dummyPolygonSets));

    // Check that the polygonSets created by combining all these triangles are traversable too
    auto coloredPolygonSets = TriangleDetail::createPolygonSetsFromTriangles(coloredExactTriangles);
    EXPECT_TRUE(TriangleDetail::isEdgeTraversable(bounds.vertex(0), bounds.vertex(1), coloredPolygonSets));
    EXPECT_TRUE(TriangleDetail::isEdgeTraversable(bounds.vertex(1), bounds.vertex(2), coloredPolygonSets));
    EXPECT_TRUE(TriangleDetail::isEdgeTraversable(bounds.vertex(2), bounds.vertex(0), coloredPolygonSets));
}

TEST(TriangleDetail, UpdateTrianglesFromPolys) {
    /**
     * Test that triangulating polygon sets and converting back to polygon sets keeps the edges traversable
     */

    // Set up the test case
    std::stringstream peprTriStream("-0.5 -0.5 0.5 0.5 -0.5 0.5 0.5 0.5 0.5");
    TriangleDetail::PeprTriangle peprTri;
    peprTriStream >> peprTri;
    const DataTriangle tri(TriangleDetail::toGlmVec(peprTri.vertex(0)), TriangleDetail::toGlmVec(peprTri.vertex(1)),
                           TriangleDetail::toGlmVec(peprTri.vertex(2)), glm::vec3(1, 0, 0), 0);
    const TriangleDetail triDetail(tri);

    std::map<size_t, PolygonSet> coloredPolys;
    // Load colored polygon data form file
    std::ifstream inFile("./tests/updateTrianglesFromPolygons.json");

    ASSERT_TRUE(inFile.good());
    {
        cereal::JSONInputArchive jsonArchive(inFile);
        ASSERT_NO_THROW(jsonArchive(coloredPolys));
    }

    ASSERT_TRUE(!coloredPolys.empty());
    const Polygon bounds = triDetail.polygonFromTriangle(peprTri);

    // Make sure the original polygon sets are traversable
    ASSERT_TRUE(TriangleDetail::isEdgeTraversable(bounds.vertex(0), bounds.vertex(1), coloredPolys));
    ASSERT_TRUE(TriangleDetail::isEdgeTraversable(bounds.vertex(1), bounds.vertex(2), coloredPolys));
    ASSERT_TRUE(TriangleDetail::isEdgeTraversable(bounds.vertex(2), bounds.vertex(0), coloredPolys));

    // Triangulate all polygons
    std::vector<Triangle2> triangles;

    for(auto& polysetIt : coloredPolys) {
        std::vector<PolygonWithHoles> polys(polysetIt.second.number_of_polygons_with_holes());
        polysetIt.second.polygons_with_holes(polys.begin());

        for(auto& poly : polys) {
            auto newTriangles = TriangleDetail::triangulatePolygon(poly);
            triangles.insert(triangles.end(), newTriangles.begin(), newTriangles.end());
        }
    }

    // Create dummy polygon sets to test traversability
    std::map<size_t, PolygonSet> dummyPolygonSets;
    for(size_t i = 0; i < triangles.size(); i++) {
        dummyPolygonSets[i] = PolygonSet(TriangleDetail::polygonFromTriangle(triangles[i]));
    }

    // The triangles (and therefore the dummy polygon sets) should be edge traversable
    EXPECT_TRUE(TriangleDetail::isEdgeTraversable(bounds.vertex(0), bounds.vertex(1), dummyPolygonSets));
    EXPECT_TRUE(TriangleDetail::isEdgeTraversable(bounds.vertex(1), bounds.vertex(2), dummyPolygonSets));
    EXPECT_TRUE(TriangleDetail::isEdgeTraversable(bounds.vertex(2), bounds.vertex(0), dummyPolygonSets));
}

TEST(TriangleDetail, ValidPolygonWithHoles) {
    /**
     * This valid PolygonWithHoles causes a CGAL Precondition fail.
     * Yet needs to be fixed by CGAL
     */

    std::stringstream sstream(
        "6 130751869427618004517459717335977/324518553658426726783156020576256 "
        "353791692968004772063555887839347/1298074214633706907132624082305024 "
        "2645304850589539436106114470773927262167511609993091574559201417199200984029/"
        "6558786099572413598322881189648324404883475363173116541129455482126548860928 "
        "28497296479541294104539559232730816192534288787225501042682105773735261917017/"
        "104940577593158617573166099034373190478135605810769864658071287714024781774848 "
        "268021016322673564227226825666057/649037107316853453566312041152512 "
        "452798967222124161336841071511579/1298074214633706907132624082305024 "
        "130751869427618004517459717335977/324518553658426726783156020576256 "
        "353791692968004772063555887839347/1298074214633706907132624082305024 "
        "65375934713809005880151730975175/162259276829213363391578010288128 "
        "421330767918851579459364608829001/1298074214633706907132624082305024 28785953274349595/72057594037927936 "
        "872710306504136969421572480422272625804604625731/2923003274661805836407369665432566039311865085952  0");
    PolygonWithHoles poly;
    sstream >> poly;

    EXPECT_TRUE(CGAL::is_valid_polygon_with_holes(poly, TriangleDetail::Traits()));
}

TEST(TriangleDetail, ValidPolygonWithHoles2) {
    /**
     * Simplified version of a ValidPolygonWithHoles test
     *
     * This valid PolygonWithHoles causes a CGAL Precondition fail.
     * Yet needs to be fixed by CGAL
     */
    std::stringstream sstream("6  2 1  3 0  5 4  2 1  1 3  0 2  0");
    PolygonWithHoles poly;
    sstream >> poly;

    EXPECT_TRUE(CGAL::is_valid_polygon_with_holes(poly, TriangleDetail::Traits()));
}

}  // namespace pepr3d

#endif
