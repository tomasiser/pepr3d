#pragma once

#include "peprimgui.h"

namespace pepr3d {

class FontStorage {
    friend class MainApplication;

    ImFont* mRegularFont;
    ImFont* mRegularIcons;
    ImFont* mSmallFont;

   public:
    ImFont* getRegularFont() {
        assert(mRegularFont != nullptr);
        return mRegularFont;
    }

    ImFont* getRegularIconFont() {
        assert(mRegularIcons != nullptr);
        return mRegularIcons;
    }

    ImFont* getSmallFont() {
        assert(mSmallFont != nullptr);
        return mSmallFont;
    }
};

}  // namespace pepr3d