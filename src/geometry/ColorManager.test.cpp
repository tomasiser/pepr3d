#ifdef _TEST_

#include <gtest/gtest.h>

#include "geometry/ColorManager.h"

TEST(ColorManager, constructor_basic) {
    pepr3d::ColorManager cm;
    EXPECT_EQ(cm.size(), 4);
    EXPECT_EQ(cm.getColor(0).r, 1);
}

TEST(ColorManager, setColor) {
    pepr3d::ColorManager cm;
    const cinder::ColorA red(1, 0, 0, 1);
    const cinder::ColorA yellow(1, 1, 0, 1);
    EXPECT_EQ(cm.getColor(0), red);

    const auto color1 = cm.getColor(1);
    const auto color2 = cm.getColor(2);
    const auto color3 = cm.getColor(3);

    cm.setColor(0, yellow);
    EXPECT_EQ(cm.getColor(0), yellow);
    EXPECT_EQ(cm.size(), 4);
    EXPECT_EQ(cm.getColor(1), color1);
    EXPECT_EQ(cm.getColor(2), color2);
    EXPECT_EQ(cm.getColor(3), color3);
}

TEST(ColorManger, constructor_iterator) {
    std::vector<cinder::ColorA> newColors;
    newColors.reserve(5);
    for(int i = 0; i < 5; ++i) {
        newColors.emplace_back(i / 5.f, 0, 0, 1);
    }

    pepr3d::ColorManager cm(newColors.begin(), newColors.end());

    EXPECT_EQ(cm.size(), 5);
    for(int i = 0; i < 5; ++i) {
        EXPECT_EQ(cm.getColor(i), cinder::ColorA(i / 5.f, 0, 0, 1));
    }

    EXPECT_EQ(newColors.size(), 5);
}

TEST(ColorManager, replaceColors_iterators) {
    std::vector<cinder::ColorA> newColors;
    newColors.reserve(5);
    for(int i = 0; i < 5; ++i) {
        newColors.emplace_back(i / 5.f, 0, 0, 1);
    }

    pepr3d::ColorManager cm;
    EXPECT_EQ(cm.size(), 4);

    cm.replaceColors(newColors.begin() + 1, newColors.end() - 1);

    EXPECT_EQ(cm.size(), 3);
    for(int i = 0; i < 3; ++i) {
        EXPECT_EQ(cm.getColor(i), cinder::ColorA((i + 1) / 5.f, 0, 0, 1));
    }

    EXPECT_EQ(newColors.size(), 5);
}

TEST(ColorManager, replaceColors_move_semantics) {
    std::vector<cinder::ColorA> newColors;
    newColors.reserve(5);
    for(int i = 0; i < 5; ++i) {
        newColors.emplace_back(i / 5.f, 0, 0, 1);
    }

    pepr3d::ColorManager cm;
    EXPECT_EQ(cm.size(), 4);

    cm.replaceColors(std::move(newColors));

    EXPECT_EQ(cm.size(), 5);
    for(int i = 0; i < 5; ++i) {
        EXPECT_EQ(cm.getColor(i), cinder::ColorA(i / 5.f, 0, 0, 1));
    }

    EXPECT_EQ(newColors.size(), 0);
}

TEST(ColorManager, constructor_color_generation) {
    pepr3d::ColorManager cm(10);
    EXPECT_EQ(cm.size(), 10);
}
#endif