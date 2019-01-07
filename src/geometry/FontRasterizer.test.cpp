#include "geometry/FontRasterizer.h"

#include <glm/gtc/epsilon.hpp>
#include "cinder/app/App.h"
#include "cinder/app/Platform.h"

#ifdef _TEST_
#include <gtest/gtest.h>
#include <vector>

namespace pepr3d {

cinder::fs::path findAssetPath() {
    auto currentPath = cinder::fs::current_path();
    do {
        const auto possiblePath = currentPath / cinder::fs::path("assets");
        if(cinder::fs::exists(possiblePath) && cinder::fs::is_directory(possiblePath)) {
            return possiblePath;
        }
        currentPath = currentPath.parent_path();
    } while(currentPath.parent_path() != currentPath);

    return cinder::fs::path();
}

std::string getAssetPath(const cinder::fs::path& relativePath) {
    auto absolutePath = findAssetPath() / relativePath;
    if(cinder::fs::exists(absolutePath)) {
        return absolutePath.string();
    }
    return cinder::fs::path().string();
}

TEST(FontRasterizer, initialize) {
    /**
     * Test a simple load of a fontfile
     */

    FontRasterizer fr(getAssetPath("fonts/SourceSansPro-SemiBold.ttf"));
    EXPECT_TRUE(fr.isValid());

    FontRasterizer fr2(getAssetPath("fonts/MaterialIcons-Regular.ttf"));
    EXPECT_TRUE(fr2.isValid());
}

TEST(FontRasterizer, getCurrentFont) {
    /**
     * Test a simple getter of a current fontfile
     */

    FontRasterizer fr(getAssetPath("fonts/SourceSansPro-SemiBold.ttf"));
    EXPECT_TRUE(fr.isValid());

    auto current = fr.getCurrentFont();
    std::size_t pos = current.find("SourceSansPro-SemiBold.ttf");
    std::string str3 = current.substr(pos);

    EXPECT_EQ(str3, "SourceSansPro-SemiBold.ttf");
}

TEST(FontRasterizer, rasterizeText_simple) {
    /**
     * Test a simple rasterization of the letter "T"
     */

    FontRasterizer fr(getAssetPath("fonts/SourceSansPro-SemiBold.ttf"));
    EXPECT_TRUE(fr.isValid());

    auto trisByLetters = fr.rasterizeText("T", 90, 3);

    EXPECT_EQ(trisByLetters.size(), 1);          // one letter
    EXPECT_EQ(trisByLetters.front().size(), 6);  // six triangles
}

TEST(FontRasterizer, rasterizeText_curvedLetter) {
    /**
     * Test a simple rasterization of the letter "O"
     */

    FontRasterizer fr(getAssetPath("fonts/SourceSansPro-SemiBold.ttf"));
    EXPECT_TRUE(fr.isValid());

    auto trisByLetters = fr.rasterizeText("O", 90, 3);

    EXPECT_EQ(trisByLetters.size(), 1);  // one letter
    EXPECT_EQ(trisByLetters.front().size(), 84);
}

TEST(FontRasterizer, rasterizeText_multipleLetters) {
    /**
     * Test a simple rasterization of the word "TO"
     */

    FontRasterizer fr(getAssetPath("fonts/SourceSansPro-SemiBold.ttf"));
    EXPECT_TRUE(fr.isValid());

    auto trisByLetters = fr.rasterizeText("TO", 90, 3);

    EXPECT_EQ(trisByLetters.size(), 2);          // two letters
    EXPECT_EQ(trisByLetters.front().size(), 6);  // six triangles
    EXPECT_EQ(trisByLetters.back().size(), 84);
}

TEST(FontRasterizer, rasterizeText_reducedBezier) {
    /**
     * Test a simple rasterization of the word "TO", with reduced bezier approximation of 1
     */

    FontRasterizer fr(getAssetPath("fonts/SourceSansPro-SemiBold.ttf"));
    EXPECT_TRUE(fr.isValid());

    auto trisByLetters = fr.rasterizeText("TO", 90, 1);

    EXPECT_EQ(trisByLetters.size(), 2);          // two letters
    EXPECT_EQ(trisByLetters.front().size(), 6);  // six triangles
    EXPECT_EQ(trisByLetters.back().size(), 28);
}

TEST(FontRasterizer, rasterizeText_postprocessShift) {
    /**
     * Test a simple rasterization of the word "T", checking if the triangles are shifted correctly
     */

    FontRasterizer fr(getAssetPath("fonts/SourceSansPro-SemiBold.ttf"));
    EXPECT_TRUE(fr.isValid());

    auto trisByLetters = fr.rasterizeText("T", 90, 1);

    EXPECT_EQ(trisByLetters.size(), 1);          // one letters
    EXPECT_EQ(trisByLetters.front().size(), 6);  // six triangles

    for(auto& t : trisByLetters.front()) {
        EXPECT_GE(t.a.y, 0);
        EXPECT_GE(t.b.y, 0);
        EXPECT_GE(t.c.y, 0);
    }
}

TEST(FontRasterizer, rasterizeText_hardcodeCheck) {
    /**
     * Test a simple rasterization of the word "T", checking every vertex for the correct position
     */

    FontRasterizer fr(getAssetPath("fonts/SourceSansPro-SemiBold.ttf"));
    EXPECT_TRUE(fr.isValid());

    auto trisByLetters = fr.rasterizeText("T", 90, 1);

    EXPECT_EQ(trisByLetters.size(), 1);          // one letters
    EXPECT_EQ(trisByLetters.front().size(), 6);  // six triangles

    std::vector<std::array<std::array<float, 2>, 3>> correctPoints = {
        {std::array<float, 2>({62.4063f, 0.f}), std::array<float, 2>({3.125f, 0.f}),
         std::array<float, 2>({39.7188f, 11.7656f})},
        {std::array<float, 2>({39.7188f, 11.7656f}), std::array<float, 2>({62.4063f, 11.7656f}),
         std::array<float, 2>({62.4063f, 0.f})},
        {std::array<float, 2>({3.125f, 0.f}), std::array<float, 2>({25.7969f, 11.7656f}),
         std::array<float, 2>({39.7188f, 11.7656f})},
        {std::array<float, 2>({3.125f, 0.f}), std::array<float, 2>({3.125f, 11.7656f}),
         std::array<float, 2>({25.7969f, 11.7656f})},
        {std::array<float, 2>({25.7969f, 11.7656f}), std::array<float, 2>({39.7188f, 78.4844f}),
         std::array<float, 2>({39.7188f, 11.7656f})},
        {std::array<float, 2>({25.7969f, 11.7656f}), std::array<float, 2>({25.7969f, 78.4844f}),
         std::array<float, 2>({39.7188f, 78.4844f})}};

    EXPECT_TRUE(glm::epsilonEqual<float>(1.0f, 1.00001f, 0.5f));

    const float EPS = 0.0001f;

    for(size_t i = 0; i < trisByLetters.front().size(); ++i) {
        auto& tri = trisByLetters.front()[i];

        EXPECT_TRUE(glm::epsilonEqual<float>(tri.a.x, correctPoints[i][0][0], EPS));
        EXPECT_TRUE(glm::epsilonEqual<float>(tri.a.y, correctPoints[i][0][1], EPS));
        EXPECT_TRUE(glm::epsilonEqual<float>(tri.a.z, 0.f, EPS));

        EXPECT_TRUE(glm::epsilonEqual<float>(tri.b.x, correctPoints[i][1][0], EPS));
        EXPECT_TRUE(glm::epsilonEqual<float>(tri.b.y, correctPoints[i][1][1], EPS));
        EXPECT_TRUE(glm::epsilonEqual<float>(tri.b.z, 0.f, EPS));

        EXPECT_TRUE(glm::epsilonEqual<float>(tri.c.x, correctPoints[i][2][0], EPS));
        EXPECT_TRUE(glm::epsilonEqual<float>(tri.c.y, correctPoints[i][2][1], EPS));
        EXPECT_TRUE(glm::epsilonEqual<float>(tri.c.z, 0.f, EPS));
    }
}

TEST(FontRasterizer, addOneCharacter_kerningCheck) {
    /**
     * The original code had a static variable in AddOneCharacter, replaced it with member variables
     */

    std::vector<std::array<std::array<float, 2>, 3>> correctPoints = {
        {std::array<float, 2>({246.672f, 94.f}), std::array<float, 2>({265.063f, 0.f}),
         std::array<float, 2>({201.f, 163.f})},
        {std::array<float, 2>({265.063f, 0.f}), std::array<float, 2>({246.672f, 94.f}),
         std::array<float, 2>({265.734f, 43.9063f})},
        {std::array<float, 2>({265.063f, 0.f}), std::array<float, 2>({265.734f, 43.9063f}),
         std::array<float, 2>({272.719f, 20.5625f})},
        {std::array<float, 2>({265.063f, 0.f}), std::array<float, 2>({272.719f, 20.5625f}),
         std::array<float, 2>({280.922f, 0.f})},
        {std::array<float, 2>({272.719f, 20.5625f}), std::array<float, 2>({280.25f, 43.9063f}),
         std::array<float, 2>({280.922f, 0.f})},
        {std::array<float, 2>({280.922f, 0.f}), std::array<float, 2>({280.25f, 43.9063f}),
         std::array<float, 2>({299.094f, 94.f})},
        {std::array<float, 2>({344.656f, 163.f}), std::array<float, 2>({280.922f, 0.f}),
         std::array<float, 2>({299.094f, 94.f})},
        {std::array<float, 2>({304.969f, 110.953f}), std::array<float, 2>({344.656f, 163.f}),
         std::array<float, 2>({299.094f, 94.f})},
        {std::array<float, 2>({344.656f, 163.f}), std::array<float, 2>({304.969f, 110.953f}),
         std::array<float, 2>({325.141f, 163.f})},
        {std::array<float, 2>({299.094f, 94.f}), std::array<float, 2>({246.672f, 94.f}),
         std::array<float, 2>({304.969f, 110.953f})},
        {std::array<float, 2>({246.672f, 94.f}), std::array<float, 2>({240.016f, 110.953f}),
         std::array<float, 2>({304.969f, 110.953f})},
        {std::array<float, 2>({201.f, 163.f}), std::array<float, 2>({240.016f, 110.953f}),
         std::array<float, 2>({246.672f, 94.f})},
        {std::array<float, 2>({201.f, 163.f}), std::array<float, 2>({220.063f, 163.f}),
         std::array<float, 2>({240.016f, 110.953f})}};

    FontRasterizer fr(getAssetPath("fonts/OpenSans-Regular.ttf"));
    EXPECT_TRUE(fr.isValid());

    auto trisByLetters = fr.rasterizeText("WAR", 170, 1);

    EXPECT_EQ(trisByLetters.size(), 3);
    EXPECT_EQ(trisByLetters.front().size(), 18);
    EXPECT_EQ(trisByLetters.at(1).size(), 13);
    EXPECT_EQ(trisByLetters.at(2).size(), 18);

    const float EPS = 0.001f;

    for(size_t i = 0; i < trisByLetters.at(1).size(); ++i) {
        auto& tri = trisByLetters.at(1)[i];

        EXPECT_TRUE(glm::epsilonEqual<float>(tri.a.x, correctPoints[i][0][0], EPS));
        EXPECT_TRUE(glm::epsilonEqual<float>(tri.a.y, correctPoints[i][0][1], EPS));
        EXPECT_TRUE(glm::epsilonEqual<float>(tri.a.z, 0.f, EPS));

        EXPECT_TRUE(glm::epsilonEqual<float>(tri.b.x, correctPoints[i][1][0], EPS));
        EXPECT_TRUE(glm::epsilonEqual<float>(tri.b.y, correctPoints[i][1][1], EPS));
        EXPECT_TRUE(glm::epsilonEqual<float>(tri.b.z, 0.f, EPS));

        EXPECT_TRUE(glm::epsilonEqual<float>(tri.c.x, correctPoints[i][2][0], EPS));
        EXPECT_TRUE(glm::epsilonEqual<float>(tri.c.y, correctPoints[i][2][1], EPS));
        EXPECT_TRUE(glm::epsilonEqual<float>(tri.c.z, 0.f, EPS));
    }
}

}  // namespace pepr3d
#endif
