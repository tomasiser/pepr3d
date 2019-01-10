#include "tools/TextEditor.h"

#include "imgui_stdlib.h"

namespace pepr3d {
void TextEditor::triangulateText() const {
    CI_LOG_I("Text triangulated.");
}

void TextEditor::drawToSidePane(SidePane& sidePane) {
    sidePane.drawText("Font: " + mFont);
    if(sidePane.drawButton("Load new font")) {
        std::vector<std::string> extensions = {"ttf"};

        mApplication.dispatchAsync([extensions, this]() {
            auto path = getOpenFilePath("", {"ttf"});
            mFontPath = path.string();
            mFont = path.filename().string();
        });
    }
    sidePane.drawIntDragger("Font size", mFontSize, 1, 0, 200, "%i", 50.f);
    sidePane.drawIntDragger("Bezier steps", mBezierSteps, 1, 0, 8, "%i", 50.f);

    ImGui::InputText("Text", &mText);

    if(sidePane.drawButton("Go")) {
        triangulateText();
    }

    sidePane.drawSeparator();

    sidePane.drawText(mText);
    sidePane.drawText(mFontPath);
}
}  // namespace pepr3d