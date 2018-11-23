#include "ui/MainApplication.h"

#include "IconsMaterialDesign.h"
#include "LightTheme.h"

#include "commands/ExampleCommand.h"
#include "geometry/Geometry.h"

#include "tools/Brush.h"
#include "tools/DisplayOptions.h"
#include "tools/Information.h"
#include "tools/LiveDebug.h"
#include "tools/PaintBucket.h"
#include "tools/Segmentation.h"
#include "tools/Settings.h"
#include "tools/TextEditor.h"
#include "tools/Tool.h"
#include "tools/TrianglePainter.h"

#if defined(CINDER_MSW_DESKTOP)
#include "windows.h"
#endif

namespace pepr3d {

MainApplication::MainApplication() : mToolbar(*this), mSidePane(*this), mModelView(*this), mThreadPool(4) {}

void MainApplication::setup() {
    setWindowSize(950, 570);
    getWindow()->setTitle("Pepr3D - Unsaved project");
    setupIcon();

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

    mGeometry = std::make_shared<Geometry>();
    mGeometry->loadNewGeometry(getAssetPath("models/defaultcube.stl").string(), mThreadPool);

    mCommandManager = std::make_unique<CommandManager<Geometry>>(*mGeometry);

    mTools.emplace_back(make_unique<TrianglePainter>(*this));
    mTools.emplace_back(make_unique<PaintBucket>(*this));
    mTools.emplace_back(make_unique<Brush>());
    mTools.emplace_back(make_unique<TextEditor>());
    mTools.emplace_back(make_unique<Segmentation>());
    mTools.emplace_back(make_unique<DisplayOptions>(*this));
    mTools.emplace_back(make_unique<pepr3d::Settings>());
    mTools.emplace_back(make_unique<Information>());
    mTools.emplace_back(make_unique<LiveDebug>(*this));
    mCurrentToolIterator = mTools.begin();

    mModelView.setup();
}

void MainApplication::resize() {
    mModelView.resize();
}

void MainApplication::mouseDown(MouseEvent event) {
    mModelView.onMouseDown(event);
}

void MainApplication::mouseDrag(MouseEvent event) {
    mModelView.onMouseDrag(event);
}

void MainApplication::mouseUp(MouseEvent event) {
    mModelView.onMouseUp(event);
}

void MainApplication::mouseWheel(MouseEvent event) {
    mModelView.onMouseWheel(event);
}

void MainApplication::mouseMove(MouseEvent event) {
    mModelView.onMouseMove(event);
}

void MainApplication::fileDrop(FileDropEvent event) {
    if(mGeometry == nullptr || event.getFiles().size() < 1) {
        return;
    }
    openFile(event.getFile(0).string());
}

void MainApplication::openFile(const std::string& path) {
    if(mGeometryInProgress != nullptr) {
        return;  // disallow loading new geometry while another is already being loaded
    }

    std::shared_ptr<Geometry> geometry = mGeometryInProgress = std::make_shared<Geometry>();
    mProgressIndicator.setGeometryInProgress(geometry);
    mThreadPool.enqueue([geometry, path, this]() {
        std::cout << "opening file from thread pool" << std::endl;
        geometry->loadNewGeometry(path, mThreadPool);
        std::cout << "finished from thread pool" << std::endl;
        dispatchAsync([path, this]() {
            std::cout << "dispatch" << std::endl;
            mGeometry = mGeometryInProgress;
            mGeometryInProgress = nullptr;
            mGeometryFileName = path;
            mCommandManager = std::make_unique<CommandManager<Geometry>>(*mGeometry);
            getWindow()->setTitle(std::string("Pepr3D - ") + mGeometryFileName);
            mProgressIndicator.setGeometryInProgress(nullptr);
        });
    });
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
    mProgressIndicator.draw();

    // if(mGeometryInProgress != nullptr) {
    //     std::string progressStatus =
    //         "%% render: " + std::to_string(mGeometryInProgress->getProgress().importRenderPercentage);
    //     ImGui::Text(progressStatus.c_str());
    //     progressStatus = "%% compute: " + std::to_string(mGeometryInProgress->getProgress().importComputePercentage);
    //     ImGui::Text(progressStatus.c_str());
    //     progressStatus = "%% buffers: " + std::to_string(mGeometryInProgress->getProgress().buffersPercentage);
    //     ImGui::Text(progressStatus.c_str());
    //     progressStatus = "%% aabb: " + std::to_string(mGeometryInProgress->getProgress().aabbTreePercentage);
    //     ImGui::Text(progressStatus.c_str());
    //     progressStatus = "%% polyhedron: " + std::to_string(mGeometryInProgress->getProgress().polyhedronPercentage);
    //     ImGui::Text(progressStatus.c_str());
    // }
}

void MainApplication::setupIcon() {
#if defined(CINDER_MSW_DESKTOP)
    auto dc = getWindow()->getDc();
    auto wnd = WindowFromDC(dc);
    auto icon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(101));  // see resources/Resources.rc
    SendMessage(wnd, WM_SETICON, ICON_SMALL, (LPARAM)icon);
    SendMessage(wnd, WM_SETICON, ICON_BIG, (LPARAM)icon);
#endif
}

}  // namespace pepr3d