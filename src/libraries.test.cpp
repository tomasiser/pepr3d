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

#endif