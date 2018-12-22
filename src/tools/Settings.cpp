#include "Settings.h"

#include "ui/MainApplication.h"

namespace pepr3d {

void Settings::drawToSidePane(SidePane& sidePane) {
    mColorPaletteCategory.draw(sidePane, [&sidePane, this]() { sidePane.drawColorPalette("", true); });
}

}  // namespace pepr3d