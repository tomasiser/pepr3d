#ifdef _TEST_

#include <gtest/gtest.h>
/**
 * Test that includes and library files are set up correctly
 */

#include <assimp/postprocess.h>  // Post processing flags
#include <assimp/scene.h>        // Output data structure
#include <assimp/Importer.hpp>   // C++ importer interface

TEST(Libraries, Assimp) {
    // Create an instance of the Importer class
    Assimp::Importer importer;
    // And have it read the given file with some example postprocessing
    // Usually - if speed is not the most important aspect for you - you'll
    // probably to request more postprocessing than we do in this example.
    const aiScene *scene = importer.ReadFile(
        "testfile.obj",  // - dummy file
        aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType);

    EXPECT_TRUE(true);
}

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/convex_hull_2.h>
#include <vector>
typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef K::Point_2 Point_2;
typedef std::vector<Point_2> Points;

TEST(Libraries, CGAL) {
    Points points, result;
    points.push_back(Point_2(0, 0));
    points.push_back(Point_2(10, 0));
    points.push_back(Point_2(10, 10));
    points.push_back(Point_2(6, 5));
    points.push_back(Point_2(4, 1));
    CGAL::convex_hull_2(points.begin(), points.end(), std::back_inserter(result));
    EXPECT_EQ(result.size(), 3);
}

#endif