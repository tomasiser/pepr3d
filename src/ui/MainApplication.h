#pragma once

//#if defined(NDEBUG)
//#define CI_MIN_LOG_LEVEL 3 // warnings
//#endif

#include <algorithm>
#include <queue>

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/TextureFont.h"
#include "cinder/gl/gl.h"

#include "peprimgui.h"

#include "ThreadPool.h"

#include "AssetNotFoundException.h"
#include "Dialog.h"
#include "FontStorage.h"
#include "Hotkeys.h"
#include "ModelView.h"
#include "ProgressIndicator.h"
#include "SidePane.h"
#include "Toolbar.h"
#include "commands/CommandManager.h"
#include "geometry/ExportType.h"

namespace pepr3d {
class Tool;
class Geometry;
using cinder::app::FileDropEvent;
using cinder::app::KeyEvent;
using cinder::app::MouseEvent;

class MainApplication : public cinder::app::App {
   public:
    void setup() override;
    void update() override;
    void draw() override;
    void resize() override;
    void mouseDown(MouseEvent event) override;
    void mouseDrag(MouseEvent event) override;
    void mouseUp(MouseEvent event) override;
    void mouseWheel(MouseEvent event) override;
    void mouseMove(MouseEvent event) override;
    void fileDrop(FileDropEvent event) override;
    void keyDown(KeyEvent event) override;

    MainApplication();

    static void prepareSettings(Settings* settings) {
        assert(settings != nullptr);
#if defined(CINDER_MSW_DESKTOP) && !defined(NDEBUG)
        settings->setConsoleWindowEnabled(true);
#endif
    }

    static ::ThreadPool& getThreadPool() {
        return sThreadPool;
    }

    Toolbar& getToolbar() {
        return mToolbar;
    }

    SidePane& getSidePane() {
        return mSidePane;
    }

    ModelView& getModelView() {
        return mModelView;
    }

    bool isDemoWindowShown() {
        return mShowDemoWindow;
    }

    void showDemoWindow(bool show = true) {
        mShowDemoWindow = show;
    }

    void openFile(const std::string& path);
    void saveFile(const std::string& filePath, const std::string& fileName, const std::string& fileType,
                  ExportType exportType);

    using ToolsVector = std::vector<std::unique_ptr<Tool>>;

    ToolsVector::iterator getToolsBegin() {
        return mTools.begin();
    }

    ToolsVector::iterator getToolsEnd() {
        return mTools.end();
    }

    ToolsVector::iterator getCurrentToolIterator() {
        assert(mTools.size() > 0);
        assert(mCurrentToolIterator != mTools.end());
        return mCurrentToolIterator;
    }

    Tool* getCurrentTool() {
        if(mTools.size() < 1 || mCurrentToolIterator == mTools.end()) {
            return nullptr;
        }
        return (*mCurrentToolIterator).get();
    }

    void setCurrentToolIterator(ToolsVector::iterator tool) {
        assert(mTools.size() > 0);
        assert(tool != mTools.end());
        assert((*tool)->isEnabled());
        (*mCurrentToolIterator)->onToolDeselect(mModelView);
        mCurrentToolIterator = tool;
        (*mCurrentToolIterator)->onToolSelect(mModelView);
    }

    template <typename Tool>
    void setCurrentTool() {
        const auto toolIterator = std::find_if(mTools.begin(), mTools.end(),
                                               [](auto& tool) { return dynamic_cast<Tool*>(tool.get()) != nullptr; });
        if(toolIterator != mTools.end()) {
            setCurrentToolIterator(toolIterator);
        }
    }

    Geometry* getCurrentGeometry() {
        return mGeometry.get();
    }

    CommandManager<Geometry>* getCommandManager() {
        return mCommandManager.get();
    }

    const std::vector<std::string> supportedImportExtensions = {"stl", "obj", "ply"};
    const std::vector<std::string> supportedOpenExtensions = {"p3d"};
    void showImportDialog(const std::vector<std::string>& extensions);

    void showExportDialog() {
        mShowExportDialog = true;
    }

    void drawTooltipOnHover(const std::string& label, const std::string& shortcut = "",
                            const std::string& description = "", const std::string& disabled = "",
                            glm::vec2 position = glm::vec2(-1.0f), glm::vec2 pivot = glm::vec2(0.0f));

    FontStorage& getFontStorage() {
        return mFontStorage;
    }

    const Hotkeys& getHotkeys() const {
        return mHotkeys;
    }

    void pushDialog(const pepr3d::Dialog& dialog) {
        mDialogQueue.push(dialog);

        // log dialog details via Cinder:
        switch(dialog.getType()) {
        case DialogType::Information: CI_LOG_I(dialog.getCaption() << "\n" << dialog.getMessage()); break;
        case DialogType::Warning: CI_LOG_W(dialog.getCaption() << "\n" << dialog.getMessage()); break;
        case DialogType::Error: CI_LOG_E(dialog.getCaption() << "\n" << dialog.getMessage()); break;
        case DialogType::FatalError: CI_LOG_F(dialog.getCaption() << "\n" << dialog.getMessage()); break;
        }
    }

    void saveProject();
    void saveProjectAs();

    ci::fs::path getRequiredAssetPath(const ci::fs::path& relativePath) {
        ci::fs::path path = getAssetPath(relativePath);
        if(path.empty()) {
            std::string message;
            message += "Pepr3D could not find the following file:\n\n";
            message += (ci::fs::path("assets") / relativePath).string();
            message += "\n\nThis file is necessary and since it was not found, Pepr3D has to be terminated. ";
            message += "Downloading Pepr3D again or reinstalling it should solve this problem.";
            pushDialog(Dialog(DialogType::FatalError, "Could not find a required asset", message, "Exit Pepr3D"));
            throw AssetNotFoundException(
                std::string("Could not find required asset ").append(relativePath.string()).c_str());
        }
        return path;
    }

    ci::DataSourceRef loadRequiredAsset(const ci::fs::path& relativePath) {
        getRequiredAssetPath(relativePath);
        return loadAsset(relativePath);
    }

   private:
    void setupLogging();
    void setupFonts();
    void setupIcon();
    void drawExportDialog();
    void willResignActive();
    void didBecomeActive();
    bool isWindowObscured();
    bool showLoadingErrorDialog();
    void loadHotkeysFromFile(const std::string& path);
    void saveHotkeysToFile(const std::string& path);

    bool mShouldSkipDraw = false;
    bool mIsFocused = true;

    Hotkeys mHotkeys;

    glm::vec2 mLastTooltipSize = glm::vec2(0.0f);

    peprimgui::PeprImGui mImGui;  // ImGui wrapper for Cinder/Pepr3D
    FontStorage mFontStorage;

    Toolbar mToolbar;
    SidePane mSidePane;
    ModelView mModelView;
    ProgressIndicator mProgressIndicator;
    bool mShowDemoWindow = false;
    bool mShowExportDialog = false;
    bool mShouldExportInNewFolder = false;

    std::priority_queue<pepr3d::Dialog> mDialogQueue;

    ToolsVector mTools;
    ToolsVector::iterator mCurrentToolIterator;

    std::shared_ptr<Geometry> mGeometry;
    std::shared_ptr<Geometry>
        mGeometryInProgress;  // used for async loading of Geometry, is nullptr if nothing is being loaded
    std::unique_ptr<CommandManager<Geometry>> mCommandManager;

    std::string mGeometryFileName;
    bool mShouldSaveAs = true;
    std::size_t mLastVersionSaved = std::numeric_limits<std::size_t>::max();
    bool mIsGeometryDirty = false;

    static ::ThreadPool sThreadPool;
};

}  // namespace pepr3d
