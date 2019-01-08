#include "Settings.h"

#include "ui/MainApplication.h"
#include "ui/ModelView.h"

namespace pepr3d {

void Settings::drawToSidePane(SidePane& sidePane) {
    mColorPaletteCategory.draw(sidePane, [&sidePane, this]() { sidePane.drawColorPalette("", true); });
    mUiCategory.draw(sidePane, [&sidePane, this]() { drawUiSettings(sidePane); });
}

void Settings::drawUiSettings(SidePane& sidePane) {
    int width = static_cast<int>(sidePane.getWidth());
    if(sidePane.drawIntDragger("Side pane width", width, 0.25f, 200, mApplication.getWindowSize().x - 250, "%.0f px",
                               60.0f)) {
        sidePane.setWidth(static_cast<float>(width));
        mApplication.resize();
    }
    sidePane.drawTooltipOnHover("Adjust the width of the side pane.");
}

}  // namespace pepr3d