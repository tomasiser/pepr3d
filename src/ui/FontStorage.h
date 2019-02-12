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
    /// Returns ImFont* of the regular font.
    ImFont* getRegularFont() {
        assert(mRegularFont != nullptr);
        return mRegularFont;
    }

    /// Returns ImFont* of the regular icon font.
    ImFont* getRegularIconFont() {
        assert(mRegularIcons != nullptr);
        return mRegularIcons;
    }

    /// Returns ImFont* of the small font (without icons).
    ImFont* getSmallFont() {
        assert(mSmallFont != nullptr);
        return mSmallFont;
    }
};

}  // namespace pepr3d