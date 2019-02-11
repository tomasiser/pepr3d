#pragma once

#include "peprimgui.h"

namespace pepr3d {

/// Keeps pointers to all fonts used in the user interface (regular, small, and icons)
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