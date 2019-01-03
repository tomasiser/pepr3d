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

    std::vector<std::array<std::array<float, 2>, 3>> a = {
        {std::array<float, 2>({62.4063f, 0.f}), std::array<float, 2>({3.125f, 0.f}),
         std::array<float, 2>({39.7188, 11.7656})},
        {std::array<float, 2>({39.7188, 11.7656}), std::array<float, 2>({62.4063, 11.7656}),
         std::array<float, 2>({62.4063, 0})},
        {std::array<float, 2>({3.125, 0}), std::array<float, 2>({25.7969, 11.7656}),
         std::array<float, 2>({39.7188, 11.7656})},
        {std::array<float, 2>({3.125, 0}), std::array<float, 2>({3.125, 11.7656}),
         std::array<float, 2>({25.7969, 11.7656})},
        {std::array<float, 2>({25.7969, 11.7656}), std::array<float, 2>({39.7188, 78.4844}),
         std::array<float, 2>({39.7188, 11.7656})},
        {std::array<float, 2>({25.7969, 11.7656}), std::array<float, 2>({25.7969, 78.4844}),
         std::array<float, 2>({39.7188, 78.4844})}};

    EXPECT_TRUE(glm::epsilonEqual<float>(1.0f, 1.00001f, 0.5f));

    const float EPS = 0.0001f;

    for(size_t i = 0; i < trisByLetters.front().size(); ++i) {
        auto& tri = trisByLetters.front()[i];

        EXPECT_TRUE(glm::epsilonEqual<float>(tri.a.x, a[i][0][0], EPS));
        EXPECT_TRUE(glm::epsilonEqual<float>(tri.a.y, a[i][0][1], EPS));
        EXPECT_TRUE(glm::epsilonEqual<float>(tri.a.z, 0.f, EPS));

        EXPECT_TRUE(glm::epsilonEqual<float>(tri.b.x, a[i][1][0], EPS));
        EXPECT_TRUE(glm::epsilonEqual<float>(tri.b.y, a[i][1][1], EPS));
        EXPECT_TRUE(glm::epsilonEqual<float>(tri.b.z, 0.f, EPS));

        EXPECT_TRUE(glm::epsilonEqual<float>(tri.c.x, a[i][2][0], EPS));
        EXPECT_TRUE(glm::epsilonEqual<float>(tri.c.y, a[i][2][1], EPS));
        EXPECT_TRUE(glm::epsilonEqual<float>(tri.c.z, 0.f, EPS));
    }
}

}  // namespace pepr3d
#endif
