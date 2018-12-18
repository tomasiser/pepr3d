#include <cereal/archives/binary.hpp>
#ifdef _MSC_VER
// because cereal json does not conform to C++17
#pragma warning(push)
#pragma warning(disable : 4996)
#endif
#include <cereal/archives/json.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#include <cereal/types/memory.hpp>

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
#include "tools/SemiautomaticSegmentation.h"
#include "tools/Settings.h"
#include "tools/TextEditor.h"
#include "tools/Tool.h"
#include "tools/TrianglePainter.h"

#if defined(CINDER_MSW_DESKTOP)
#include "windows.h"
#endif

namespace pepr3d {

// At least 2 threads in thread pool must be created, or importing will never finish!
// std::thread::hardware_concurrency() may return 0
MainApplication::MainApplication()
    : mToolbar(*this),
      mSidePane(*this),
      mModelView(*this),
      mThreadPool(std::max<size_t>(3, std::thread::hardware_concurrency()) - 1) {}

void MainApplication::setup() {
    setWindowSize(950, 570);
    getWindow()->setTitle("Untitled - Pepr3D");
    setupIcon();
    gl::enableVerticalSync(true);
    disableFrameRate();

    getSignalWillResignActive().connect(bind(&MainApplication::willResignActive, this));
    getSignalDidBecomeActive().connect(bind(&MainApplication::didBecomeActive, this));

    mImGui.setup(this, getWindow());

    applyLightTheme(ImGui::GetStyle());

    // Uncomment the following lines to save mHotkeys on startup
    // (only useful for updating the .json file after a change in hotkeys):
    // mHotkeys.loadDefaults();
    // saveHotkeysToFile((getAssetPath("") / "hotkeys.json").string());
    try {
        loadHotkeysFromFile(getAssetPath("hotkeys.json").string());
    } catch(const cereal::Exception&) {
        CI_LOG_I("Failed to load hotkeys from hotkeys.json, using default hotkeys");
        mHotkeys.loadDefaults();
    }

    mGeometry = std::make_shared<Geometry>();
    mGeometry->loadNewGeometry(getAssetPath("models/defaultcube.stl").string(), mThreadPool);

    mCommandManager = std::make_unique<CommandManager<Geometry>>(*mGeometry);

    mTools.emplace_back(make_unique<TrianglePainter>(*this));
    mTools.emplace_back(make_unique<PaintBucket>(*this));
    mTools.emplace_back(make_unique<Brush>());
    mTools.emplace_back(make_unique<TextEditor>());
    mTools.emplace_back(make_unique<Segmentation>(*this));
    mTools.emplace_back(make_unique<SemiautomaticSegmentation>(*this));
    mTools.emplace_back(make_unique<DisplayOptions>(*this));
    mTools.emplace_back(make_unique<pepr3d::Settings>());
    mTools.emplace_back(make_unique<Information>());
    mTools.emplace_back(make_unique<LiveDebug>(*this));
    mCurrentToolIterator = mTools.begin();

    setupFonts();

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
    if(mGeometry == nullptr || !mDialogQueue.empty() || event.getFiles().size() < 1) {
        return;
    }
    openFile(event.getFile(0).string());
}

void MainApplication::keyDown(KeyEvent event) {
    const Hotkey hotkey{event.getCode(), event.isAccelDown()};
    const auto action = mHotkeys.findAction(hotkey);
    if(!action) {
        return;
    }
    switch(*action) {
    case HotkeyAction::Open: showImportDialog(supportedOpenExtensions); break;
    case HotkeyAction::Save: saveProject(); break;
    case HotkeyAction::Import: showImportDialog(supportedImportExtensions); break;
    case HotkeyAction::Export: showExportDialog(); break;
    case HotkeyAction::Undo: mCommandManager->undo(); break;
    case HotkeyAction::Redo: mCommandManager->redo(); break;
    case HotkeyAction::SelectTrianglePainter: setCurrentTool<TrianglePainter>(); break;
    case HotkeyAction::SelectPaintBucket: setCurrentTool<PaintBucket>(); break;
    case HotkeyAction::SelectBrush: setCurrentTool<Brush>(); break;
    case HotkeyAction::SelectTextEditor: setCurrentTool<TextEditor>(); break;
    case HotkeyAction::SelectSegmentation: setCurrentTool<Segmentation>(); break;
    case HotkeyAction::SelectSemiautomaticSegmentation: setCurrentTool<SemiautomaticSegmentation>(); break;
    case HotkeyAction::SelectDisplayOptions: setCurrentTool<DisplayOptions>(); break;
    case HotkeyAction::SelectSettings: setCurrentTool<pepr3d::Settings>(); break;
    case HotkeyAction::SelectInformation: setCurrentTool<Information>(); break;
    case HotkeyAction::SelectLiveDebug: setCurrentTool<LiveDebug>(); break;
    }
    std::size_t actionId = static_cast<std::size_t>(*action);
    if(mGeometry != nullptr && actionId >= static_cast<std::size_t>(HotkeyAction::SelectColor1) &&
       actionId <= static_cast<std::size_t>(HotkeyAction::SelectColor10)) {
        std::size_t colorId = actionId - static_cast<std::size_t>(HotkeyAction::SelectColor1);
        ColorManager& colorManager = mGeometry->getColorManager();
        if(colorId < colorManager.size()) {
            colorManager.setActiveColorIndex(colorId);
        }
    }
}

bool MainApplication::showLoadingErrorDialog() {
    const GeometryProgress& progress = mGeometryInProgress->getProgress();

    if(progress.importRenderPercentage < 1.0f || progress.importComputePercentage < 1.0f) {
        const std::string errorCaption = "Error: Invalid file";
        const std::string errorDescription =
            "You tried to import a file which did not contain correct geometry data that could be loaded in "
            "Pepr3D via the Assimp library. The supported files are valid .obj, .stl, and .ply.\n\nThe "
            "provided file could not be imported.";
        pushDialog(Dialog(DialogType::Error, errorCaption, errorDescription, "Cancel import"));
        mGeometryInProgress = nullptr;
        mProgressIndicator.setGeometryInProgress(nullptr);
        return false;
    }

    if(progress.buffersPercentage < 1.0f) {
        const std::string errorCaption = "Error: Failed to generate buffers";
        const std::string errorDescription =
            "Problems were found in the imported geometry. An error has occured while generating vertex, "
            "index, color, and normal buffers for rendering the geometry.\n\nThe provided file could not be "
            "imported.";
        pushDialog(Dialog(DialogType::Error, errorCaption, errorDescription, "Cancel import"));
        mGeometryInProgress = nullptr;
        mProgressIndicator.setGeometryInProgress(nullptr);
        return false;
    }

    if(progress.aabbTreePercentage < 1.0f) {
        const std::string errorCaption = "Error: Failed to build an AABB tree";
        const std::string errorDescription =
            "Problems were found in the imported geometry. An AABB tree could not be built using the data "
            "using the CGAL library.\n\nThe provided file could not be imported.";
        pushDialog(Dialog(DialogType::Error, errorCaption, errorDescription, "Cancel import"));
        mGeometryInProgress = nullptr;
        mProgressIndicator.setGeometryInProgress(nullptr);
        return false;
    }

    if(progress.polyhedronPercentage < 1.0f || !mGeometryInProgress->polyhedronValid()) {
        const std::string errorCaption = "Warning: Failed to build a polyhedron";
        const std::string errorDescription =
            "Problems were found in the imported geometry. We could not build a valid polyhedron data "
            "structure using the CGAL library.\n\nCertain tools (Paint Bucket, Segmentation) will be disabled. "
            "You can still edit the imported model with the remaining tools.";
        pushDialog(Dialog(DialogType::Warning, errorCaption, errorDescription, "Continue"));
        return true;
    }
    return true;
}

void MainApplication::loadHotkeysFromFile(const std::string& path) {
    std::ifstream is(path);
    cereal::JSONInputArchive loadArchive(is);
    loadArchive(cereal::make_nvp("hotkeys", mHotkeys));
}

void MainApplication::saveHotkeysToFile(const std::string& path) {
    std::ofstream os(path);
    cereal::JSONOutputArchive saveArchive(os);
    saveArchive(cereal::make_nvp("hotkeys", mHotkeys));
}

void MainApplication::openFile(const std::string& path) {
    if(mGeometryInProgress != nullptr) {
        return;  // disallow loading new geometry while another is already being loaded
    }

    mLastVersionSaved = 0;
    mIsGeometryDirty = false;

    mGeometryInProgress = std::make_shared<Geometry>();
    mProgressIndicator.setGeometryInProgress(mGeometryInProgress);

    fs::path fsPath(path);
    fs::path ext = fsPath.extension();

    // Lambda that will be called once the loading finishes.
    // Put all updates to saved states here.
    auto onLoadingComplete = [path, this]() {
        // Handle errors
        const bool isLoadedCorrectly = showLoadingErrorDialog();
        if(!isLoadedCorrectly) {
            return;
        }

        // Swap geometry if no errors occured
        mGeometry = mGeometryInProgress;
        mGeometryInProgress = nullptr;
        mGeometryFileName = path;
        mShouldSaveAs = true;
        mIsGeometryDirty = false;
        mCommandManager = std::make_unique<CommandManager<Geometry>>(*mGeometry);
        fs::path fsPath(path);
        getWindow()->setTitle(fsPath.stem().string() + std::string(" - Pepr3D"));
        mProgressIndicator.setGeometryInProgress(nullptr);
        for(auto& tool : mTools) {
            tool->onNewGeometryLoaded(mModelView);
        }
        CI_LOG_I("Loading complete.");
    };

    if(ext == ".p3d" || ext == ".P3D" || ext == ".p3D" || ext == ".P3d") {
        CI_LOG_I("Loading project from " + path);
        {
            std::ifstream is(path, std::ios::binary);
            cereal::BinaryInputArchive loadArchive(is);
            // CAREFUL! Replaces the shared_ptr in mGeometryInProgress!
            try {
                loadArchive(mGeometryInProgress);
            } catch(const cereal::Exception&) {
                const std::string errorCaption = "Error: Pepr project file (.p3d) corrupted";
                const std::string errorDescription =
                    "The project file you attempted to open is corrupted and cannot be loaded. "
                    "Try loading an earlier backup version, which might not be corrupted yet.";
                pushDialog(Dialog(DialogType::Error, errorCaption, errorDescription, "Cancel import"));
                mGeometryInProgress = nullptr;
                mProgressIndicator.setGeometryInProgress(nullptr);
                return;
            }
            // Pointer changed, replace it in progress indicator
            mProgressIndicator.setGeometryInProgress(mGeometryInProgress);
        }
        auto asyncCalculation = [onLoadingComplete, path, this]() {
            mGeometryInProgress->recomputeFromData(mThreadPool);
            // Call the lambda to swap the geometry and command manager pointers, etc.
            // onLoadingComplete Gets called at the beginning of the next draw() cycle.
            dispatchAsync(onLoadingComplete);
        };
        mThreadPool.enqueue(asyncCalculation);
    } else {
        CI_LOG_I("Importing a new model from " + path);

        // Queue the loading of the new geometry
        auto importNewModel = [onLoadingComplete, path, this]() {
            // Load the geometry
            mGeometryInProgress->loadNewGeometry(path, mThreadPool);

            // Call the lambda to swap the geometry and command manager pointers, etc.
            // onLoadingComplete Gets called at the beginning of the next draw() cycle.
            dispatchAsync(onLoadingComplete);
        };
        mThreadPool.enqueue(importNewModel);
    }
}

void MainApplication::saveFile(const std::string& filePath, const std::string& fileName, const std::string& fileType) {
    if(mGeometry == nullptr) {
        return;
    }

    mProgressIndicator.setGeometryInProgress(mGeometry);
    mThreadPool.enqueue([filePath, fileName, fileType, this]() {
        mGeometry->exportGeometry(filePath, fileName, fileType);
        dispatchAsync([this]() { mProgressIndicator.setGeometryInProgress(nullptr); });
    });
}

void MainApplication::update() {
    // verify that a selected tool is enabled, otherwise select Triangle Painter, which is always enabled:
    if(!(*mCurrentToolIterator)->isEnabled()) {
        mCurrentToolIterator = mTools.begin();
    }

#if defined(CINDER_MSW_DESKTOP)
    // on Microsoft Windows, when window is not focused, periodically check
    // if it is obscured (not visible) every 2 seconds
    if(!mIsFocused) {
        if((mShouldSkipDraw && (getElapsedFrames() % 4) == 0) || (!mShouldSkipDraw && (getElapsedFrames() % 48) == 0)) {
            if(isWindowObscured()) {
                mShouldSkipDraw = true;
                setFrameRate(2.0f);  // cannot set to 0.0f because then the window would never wake up again
            }
        }
    }
#endif
    if(!mIsGeometryDirty && mLastVersionSaved != mCommandManager->getVersionNumber()) {
        mIsGeometryDirty = true;
        fs::path path(mGeometryFileName);

        std::string title = path.has_stem() ? path.stem().string() : "Untitled";
        title += std::string("* - Pepr3D");
        getWindow()->setTitle(title);
    }
}

void MainApplication::draw() {
    if(mShouldSkipDraw) {
        return;
    }

    gl::clear(ColorA::hex(0xFCFCFC));

    // draw highest priority dialog:
    if(!mDialogQueue.empty()) {
        const bool shouldClose = mDialogQueue.top().draw();
        const bool isTopDialogFatal = mDialogQueue.top().isFatalError();

        if(shouldClose) {
            if(isTopDialogFatal) {
                quit();
            }
            mDialogQueue.pop();
        }

        // Do not draw anything if the fault is fatal
        if(isTopDialogFatal) {
            return;
        }
    }

    if(mShowDemoWindow) {
        ImGui::ShowDemoWindow();
    }

    mToolbar.draw();
    mSidePane.draw();
    mModelView.draw();
    mProgressIndicator.draw();

    if(mShowExportDialog) {
        drawExportDialog();
    }
}

void MainApplication::setupFonts() {
    ImFontAtlas* fontAtlas = ImGui::GetIO().Fonts;

    fontAtlas->Clear();

    std::vector<ImWchar> textRange = {0x0001, 0x00FF, 0};
    ImFontConfig fontConfig;
    fontConfig.GlyphExtraSpacing.x = -0.2f;
    mFontStorage.mRegularFont = fontAtlas->AddFontFromFileTTF(
        getAssetPath("fonts/SourceSansPro-SemiBold.ttf").string().c_str(), 18.0f, &fontConfig, textRange.data());
    mFontStorage.mSmallFont = fontAtlas->AddFontFromFileTTF(
        getAssetPath("fonts/SourceSansPro-SemiBold.ttf").string().c_str(), 16.0f, &fontConfig, textRange.data());

    ImVector<ImWchar> iconsRange;
    ImFontAtlas::GlyphRangesBuilder iconsRangeBuilder;
    for(auto& tool : mTools) {
        iconsRangeBuilder.AddText(tool->getIcon().c_str());
    }
    iconsRangeBuilder.AddText(ICON_MD_ARROW_DROP_DOWN);
    iconsRangeBuilder.AddText(ICON_MD_FOLDER_OPEN);
    iconsRangeBuilder.AddText(ICON_MD_UNDO);
    iconsRangeBuilder.AddText(ICON_MD_REDO);
    iconsRangeBuilder.AddText(ICON_MD_CHILD_FRIENDLY);
    iconsRangeBuilder.BuildRanges(&iconsRange);
    fontConfig.GlyphExtraSpacing.x = 0.0f;
    mFontStorage.mRegularIcons = fontAtlas->AddFontFromFileTTF(
        getAssetPath("fonts/MaterialIcons-Regular.ttf").string().c_str(), 24.0f, &fontConfig, iconsRange.Data);
    mFontStorage.mRegularIcons->DisplayOffset.y = -1.0f;

    mImGui.refreshFontTexture();
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

void MainApplication::showImportDialog(const std::vector<std::string>& extensions) {
    dispatchAsync([extensions, this]() {
        fs::path initialPath(mGeometryFileName);
        initialPath.remove_filename();
        if(initialPath.empty()) {
            initialPath = getDocumentsDirectory();
        }

        auto path = getOpenFilePath(initialPath, extensions);

        if(!path.empty()) {
            openFile(path.string());
        }
    });
}

void MainApplication::drawExportDialog() {
    if(mGeometry == nullptr) {
        return;
    }

    bool exportStl = false;
    bool exportPly = false;
    bool createStlFolder = false;
    bool createPlyFolder = false;

    if(!ImGui::IsPopupOpen("##exportdialog")) {
        ImGui::OpenPopup("##exportdialog");
    }
    ImGuiWindowFlags window_flags = 0;
    window_flags |= ImGuiWindowFlags_NoTitleBar;
    window_flags |= ImGuiWindowFlags_NoScrollbar;
    window_flags |= ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoResize;
    window_flags |= ImGuiWindowFlags_NoCollapse;
    window_flags |= ImGuiWindowFlags_NoNav;
    const ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x / 2.0f, io.DisplaySize.y / 2.0f), ImGuiCond_Always,
                            ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400.0f, -1.0f));
    ImGui::SetNextWindowBgAlpha(1.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ci::ColorA::hex(0xFFFFFF));
    ImGui::PushStyleColor(ImGuiCol_Border, ci::ColorA::hex(0xEDEDED));
    ImGui::PushStyleColor(ImGuiCol_Text, ci::ColorA::hex(0x1C2A35));
    ImGui::PushStyleColor(ImGuiCol_Separator, ci::ColorA::hex(0xEDEDED));
    ImGui::PushStyleColor(ImGuiCol_Button, ci::ColorA::zero());
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ci::ColorA::hex(0xCFD5DA));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ci::ColorA::hex(0xA3B2BF));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, glm::vec2(12.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, glm::vec2(8.0f, 6.0f));
    if(ImGui::BeginPopupModal("##exportdialog", nullptr, window_flags)) {
        ImGui::Text("Exporting geometry");
        ImGui::Spacing();
        ImGui::Checkbox("Create a new folder for exported files", &mShouldExportInNewFolder);
        ImGui::Spacing();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text(".stl (stereolithography)");
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, ci::ColorA::hex(0x017BDA));
        ImGui::Text("(recommended for Prusa printers)");
        ImGui::PopStyleColor();
        ImGui::Spacing();
        ImGui::TextWrapped(
            "Exports multiple different .stl files, each with a separate color of the model. "
            "This is suitable for 3D printing with Prusa printers and Slic3r Prusa Edition.");

        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Button, ci::ColorA::hex(0x017BDA));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ci::ColorA::hex(0x0165B2));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ci::ColorA::hex(0x015699));
        ImGui::PushStyleColor(ImGuiCol_Text, ci::ColorA::hex(0xFFFFFF));
        exportStl = ImGui::Button("Export as multiple .stl files", glm::ivec2(ImGui::GetContentRegionAvailWidth(), 33));
        ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
                                            (ImColor)ci::ColorA::hex(0xEDEDED));
        ImGui::PopStyleColor(4);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text(".ply (Polygon File Format)");
        ImGui::Spacing();
        ImGui::TextWrapped("Exports multiple different .ply files, each with a separate color of the model.");
        ImGui::Spacing();

        exportPly = ImGui::Button("Export as multiple .ply files", glm::ivec2(ImGui::GetContentRegionAvailWidth(), 33));
        ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
                                            (ImColor)ci::ColorA::hex(0xEDEDED));

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        const bool shouldClose = ImGui::Button("Cancel", glm::ivec2(ImGui::GetContentRegionAvailWidth(), 33));
        ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
                                            (ImColor)ci::ColorA::hex(0xEDEDED));

        if(shouldClose) {
            mShowExportDialog = false;
            ImGui::CloseCurrentPopup();
        } else if(exportStl || exportPly) {
            mShowExportDialog = false;
            ImGui::CloseCurrentPopup();
            dispatchAsync([exportStl, exportPly, this]() {
                fs::path initialPath(mGeometryFileName);
                std::string name = initialPath.filename().replace_extension().string();
                initialPath.remove_filename();
                if(initialPath.empty()) {
                    initialPath = getDocumentsDirectory();
                    name = "untitled";
                } else {
                    name += "_exported";
                }

                std::string fileType;
                std::vector<std::string> extensions;
                if(exportStl) {
                    extensions.emplace_back("stl");
                    name += ".stl";
                    fileType = "stl";
                } else if(exportPly) {
                    extensions.emplace_back("ply");
                    name += ".ply";
                    fileType = "ply";
                }

                auto path = getSaveFilePath(initialPath.append(name), extensions);

                if(!path.empty()) {
                    std::string fileName =
                        path.filename().string().substr(0, path.filename().string().find_last_of("."));
                    std::string filePath = path.parent_path().string();
                    if(mShouldExportInNewFolder) {
                        filePath += std::string("/") + std::string(fileName);
                    }

                    if(!fs::is_directory(filePath) || !fs::exists(filePath)) {  // check if folder exists
                        fs::create_directory(filePath);
                    }

                    saveFile(filePath, fileName, fileType);
                }
            });
        }

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(7);
}

void MainApplication::drawTooltipOnHover(const std::string& label, const std::string& shortcut,
                                         const std::string& description, const std::string& disabled,
                                         glm::vec2 position, glm::vec2 pivot) {
    if(ImGui::IsItemHovered()) {
        ImGui::PushFont(mFontStorage.getRegularFont());
        ImGui::PushStyleColor(ImGuiCol_PopupBg, ci::ColorA::hex(0x1C2A35));
        ImGui::PushStyleColor(ImGuiCol_Border, ci::ColorA::hex(0x1C2A35));
        ImGui::PushStyleColor(ImGuiCol_Text, ci::ColorA::hex(0xFFFFFF));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, glm::vec2(12.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, glm::vec2(8.0f, 6.0f));

        const bool isStaticPosition = position.x > -1.0f || position.y > -1.0f;  // do not follow mouse
        if(isStaticPosition) {
            if(position.x < 6.0f) {
                position.x = 6.0f;
            }
            const float bottomY = position.y + (1.0f - pivot.y) * mLastTooltipSize.y;
            if(bottomY > ImGui::GetIO().DisplaySize.y - 6.0f) {
                position.y = ImGui::GetIO().DisplaySize.y - 6.0f;
                pivot.y = 1.0f;
            }
            ImGui::SetNextWindowPos(position, ImGuiCond_Always, pivot);
        }

        ImGui::BeginTooltip();

        mLastTooltipSize = ImGui::GetWindowSize();

        ImGui::PushTextWrapPos(200.0f);
        ImGui::TextUnformatted(label.c_str());
        ImGui::PopTextWrapPos();

        if(!shortcut.empty()) {
            ImGui::SameLine(0.0f, 4.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, ci::ColorA::hex(0xAAAAAA));
            ImGui::Text("(%s)", shortcut.c_str());
            ImGui::PopStyleColor();
        }

        if(!description.empty()) {
            ImGui::PushFont(mFontStorage.getSmallFont());
            ImGui::PushTextWrapPos(250.0f);
            ImGui::TextUnformatted(description.c_str());
            ImGui::PopTextWrapPos();
            ImGui::PopFont();
        }

        if(!disabled.empty()) {
            ImGui::PushFont(mFontStorage.getSmallFont());
            ImGui::PushStyleColor(ImGuiCol_Text, ci::ColorA::hex(0xEB5757));
            ImGui::PushTextWrapPos(250.0f);
            ImGui::TextUnformatted(disabled.c_str());
            ImGui::PopTextWrapPos();
            ImGui::PopStyleColor();
            ImGui::PopFont();
        }

        ImGui::EndTooltip();

        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(3);
        ImGui::PopFont();
    }
}

void MainApplication::willResignActive() {
    setFrameRate(24.0f);
    mIsFocused = false;
}

void MainApplication::didBecomeActive() {
    disableFrameRate();
    mIsFocused = true;
    mShouldSkipDraw = false;
}

bool MainApplication::isWindowObscured() {
#if defined(CINDER_MSW_DESKTOP)
    auto dc = getWindow()->getDc();
    auto wnd = WindowFromDC(dc);

    if(IsIconic(wnd)) {
        return true;  // window is minimized (iconic)
    }

    RECT windowRect;
    if(GetWindowRect(wnd, &windowRect)) {
        // check if window is obscured by another window at 3 diagonal points (top left, center, bottom right):
        bool isObscuredAtDiagonal = true;
        POINT checkpoint;
        // check window top left:
        checkpoint.x = windowRect.left;
        checkpoint.y = windowRect.top;
        auto wndAtCheckpoint = WindowFromPoint(checkpoint);
        isObscuredAtDiagonal &= (wndAtCheckpoint != wnd);
        // check window center:
        checkpoint.x = windowRect.left + (windowRect.right - windowRect.left) / 2;
        checkpoint.y = windowRect.top + (windowRect.bottom - windowRect.top) / 2;
        wndAtCheckpoint = WindowFromPoint(checkpoint);
        isObscuredAtDiagonal &= (wndAtCheckpoint != wnd);
        // check window bottom right:
        checkpoint.x = windowRect.right - 1;
        checkpoint.y = windowRect.bottom - 1;
        wndAtCheckpoint = WindowFromPoint(checkpoint);
        isObscuredAtDiagonal &= (wndAtCheckpoint != wnd);
        if(isObscuredAtDiagonal) {
            return true;
        }
    }
#endif
    return false;
}

void MainApplication::saveProjectAs() {
    dispatchAsync([this]() {
        fs::path initialPath(mGeometryFileName);
        initialPath.remove_filename();
        if(initialPath.empty()) {
            initialPath = getDocumentsDirectory();
        }
        std::string name = "Untitled";
        if(mGeometryFileName != "") {
            fs::path dirToSave = mGeometryFileName;
            name = dirToSave.stem().string();
        }

        auto path = getSaveFilePath(initialPath.append(name), {"p3d"});

        if(path.empty()) {
            return;
        }
        if(path.extension() == "") {
            path.replace_extension(".p3d");
        }

        std::string finalPath = path.string();
        CI_LOG_I("Saving project into " + finalPath);
        {
            std::ofstream os(finalPath, std::ios::binary);
            cereal::BinaryOutputArchive saveArchive(os);
            saveArchive(mGeometry);
        }

        mGeometryFileName = finalPath;
        mLastVersionSaved = mCommandManager->getVersionNumber();
        mIsGeometryDirty = false;
        getWindow()->setTitle(path.stem().string() + std::string(" - Pepr3D"));
        mShouldSaveAs = false;
    });
}

void MainApplication::saveProject() {
    if(mGeometryFileName == "" || mShouldSaveAs) {
        saveProjectAs();
        return;
    }
    fs::path dirToSave = mGeometryFileName;
    dirToSave.replace_extension(".p3d");
    std::string finalPath = dirToSave.string();
    CI_LOG_I("Saving project into " + finalPath);
    {
        std::ofstream os(finalPath, std::ios::binary);
        cereal::BinaryOutputArchive saveArchive(os);
        saveArchive(mGeometry);
    }
    mLastVersionSaved = mCommandManager->getVersionNumber();
    mIsGeometryDirty = false;
    getWindow()->setTitle(dirToSave.stem().string() + std::string(" - Pepr3D"));
}

}  // namespace pepr3d
