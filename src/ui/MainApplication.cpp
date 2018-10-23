#include "ui/MainApplication.h"

namespace pepr3d {

MainApplication::MainApplication()
    : mToolbar(*this), mSidePane(*this), mModelView(*this) {}

void MainApplication::setup() {
    setWindowSize(950, 570);
    auto uiOptions = ImGui::Options();
    std::vector<ImWchar> textRange = {0x0001, 0x00BF, 0};
    std::vector<ImWchar> iconsRange = {ICON_MIN_MD, ICON_MAX_MD, 0};
    uiOptions.fonts({make_pair<fs::path, float>(getAssetPath("fonts/SourceSansPro-SemiBold.ttf"), 1.0f * 18.0f),
                     make_pair<fs::path, float>(getAssetPath("fonts/MaterialIcons-Regular.ttf"), 1.0f * 24.0f)},
                    true);
    uiOptions.fontGlyphRanges("SourceSansPro-SemiBold", textRange);
    uiOptions.fontGlyphRanges("MaterialIcons-Regular", iconsRange);
    ImGui::initialize(uiOptions);
    applyLightTheme(ImGui::GetStyle());

    mTools.emplace_back(make_unique<TrianglePainter>());
    mTools.emplace_back(make_unique<Brush>());
    mTools.emplace_back(make_unique<PaintBucket>());
    mTools.emplace_back(make_unique<TextEditor>());
    mTools.emplace_back(make_unique<Segmentation>());
    mTools.emplace_back(make_unique<DisplayOptions>());
    mTools.emplace_back(make_unique<pepr3d::Settings>());
    mTools.emplace_back(make_unique<Information>());
    mTools.emplace_back(make_unique<LiveDebug>(*this));
    mCurrentToolIterator = --mTools.end();

    mModelView.setup();
}

void MainApplication::resize() {
    mModelView.resize();
}

void MainApplication::mouseDown(MouseEvent event) {
    // onModelViewMouseDown(mState, event);
}

void MainApplication::mouseDrag(MouseEvent event) {
    // onModelViewMouseDrag(mState, event);
}

void MainApplication::update() {}

void MainApplication::draw() {
    gl::clear(ColorA::hex(0xFCFCFC));

    if(mShowDemoWindow) {
        ImGui::ShowDemoWindow();
    }

    mToolbar.draw();
    mSidePane.draw();
    mModelView.draw();
}

}  // namespace pepr3d