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

namespace pepr3d {
class Tool;
class Geometry;
using cinder::app::FileDropEvent;
using cinder::app::KeyEvent;
using cinder::app::MouseEvent;

/// The main Cinder-based application, represents a window, handles events, thread pool, active tools, geometry, etc.
class MainApplication : public cinder::app::App {
   public:
    /// Called by Cinder.
    /// Perform any application setup after the Renderer has been initialized.
    void setup() override;

    /// Called by Cinder.
    /// Perform any once-per-loop computation.
    void update() override;

    /// Called by Cinder.
    /// Perform any rendering once-per-loop or in response to OS-prompted requests for refreshes.
    void draw() override;

    /// Called by Cinder.
    /// Receive window resize events.
    void resize() override;

    /// Called by Cinder.
    /// Receive mouse-down events.
    void mouseDown(MouseEvent event) override;

    /// Called by Cinder.
    /// Receive mouse-drag events.
    void mouseDrag(MouseEvent event) override;

    /// Called by Cinder.
    /// Receive mouse-up events.
    void mouseUp(MouseEvent event) override;

    /// Called by Cinder.
    /// Receive mouse-wheel events.
    void mouseWheel(MouseEvent event) override;

    /// Called by Cinder.
    /// Receive mouse-move events.
    void mouseMove(MouseEvent event) override;

    /// Called by Cinder.
    /// Receive file-drop events.
    void fileDrop(FileDropEvent event) override;

    /// Called by Cinder.
    /// Receive key-down events.
    void keyDown(KeyEvent event) override;

    MainApplication();

    /// Called by Cinder.
    /// Prepare settings before setting up the application.
    static void prepareSettings(Settings* settings) {
        assert(settings != nullptr);
#if defined(CINDER_MSW_DESKTOP) && !defined(NDEBUG)
        settings->setConsoleWindowEnabled(true);
#endif
    }

    /// Returns a single instance of the ThreadPool.
    static ::ThreadPool& getThreadPool() {
        return sThreadPool;
    }

    /// Returns a reference to the Toolbar.
    Toolbar& getToolbar() {
        return mToolbar;
    }

    /// Returns a reference to the SidePane.
    SidePane& getSidePane() {
        return mSidePane;
    }

    /// Returns a reference to the ModelView.
    ModelView& getModelView() {
        return mModelView;
    }

    /// Returns true if the ImGui demo window is shown.
    bool isDemoWindowShown() {
        return mShowDemoWindow;
    }

    /// Set if the ImGui demo window should be shown.
    void showDemoWindow(bool show = true) {
        mShowDemoWindow = show;
    }

    /// Tries to open a file in the specified path and use it as the new Geometry.
    void openFile(const std::string& path);
    void saveFile(const std::string& filePath, const std::string& fileName, const std::string& fileType);

    using ToolsVector = std::vector<std::unique_ptr<Tool>>;

    /// Get the begin iterator of the current vector of Tool.
    ToolsVector::iterator getToolsBegin() {
        return mTools.begin();
    }

    /// Get the end iterator of the current vector of Tool.
    ToolsVector::iterator getToolsEnd() {
        return mTools.end();
    }

    /// Get the iterator of the currently selected Tool.
    ToolsVector::iterator getCurrentToolIterator() {
        assert(mTools.size() > 0);
        assert(mCurrentToolIterator != mTools.end());
        return mCurrentToolIterator;
    }

    /// Get a pointer of the currently selected Tool.
    Tool* getCurrentTool() {
        if(mTools.size() < 1 || mCurrentToolIterator == mTools.end()) {
            return nullptr;
        }
        return (*mCurrentToolIterator).get();
    }

    /// Set the iterator of the Tool you want to be selected.
    void setCurrentToolIterator(ToolsVector::iterator tool) {
        assert(mTools.size() > 0);
        assert(tool != mTools.end());
        assert((*tool)->isEnabled());
        (*mCurrentToolIterator)->onToolDeselect(mModelView);
        mCurrentToolIterator = tool;
        (*mCurrentToolIterator)->onToolSelect(mModelView);
    }

    /// Set the Tool (in template) as selected.
    /// Usage: setCurrentTool<PaintBucket>()
    template <typename Tool>
    void setCurrentTool() {
        const auto toolIterator = std::find_if(mTools.begin(), mTools.end(),
                                               [](auto& tool) { return dynamic_cast<Tool*>(tool.get()) != nullptr; });
        if(toolIterator != mTools.end()) {
            setCurrentToolIterator(toolIterator);
        }
    }

    /// Returns a pointer to the current Geometry.
    Geometry* getCurrentGeometry() {
        return mGeometry.get();
    }

    /// Returns a pointer to the current CommandManager.
    CommandManager<Geometry>* getCommandManager() {
        return mCommandManager.get();
    }

    const std::vector<std::string> supportedImportExtensions = {"stl", "obj", "ply"};
    const std::vector<std::string> supportedOpenExtensions = {"p3d"};
    /// Opens the file dialog for import.
    void showImportDialog(const std::vector<std::string>& extensions);

    void showExportDialog() {
        mShowExportDialog = true;
    }

    /// Draws a tooltip if the previous ImGui item was hovered.
    /// Optionally you can set a custom position and pivot point, if they are not provided, the tooltip will hover above the item.
    void drawTooltipOnHover(const std::string& label, const std::string& shortcut = "",
                            const std::string& description = "", const std::string& disabled = "",
                            glm::vec2 position = glm::vec2(-1.0f), glm::vec2 pivot = glm::vec2(0.0f));

    /// Returns a reference to the FontStorage.
    FontStorage& getFontStorage() {
        return mFontStorage;
    }

    /// Returns a reference to the Hotkeys.
    const Hotkeys& getHotkeys() const {
        return mHotkeys;
    }

    /// Pushes a new dialog to the priority queue of the application.
    /// If it is a fatal error dialog, all other rendering is stopped (but only since the next frame).
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

    /// Saves the .p3d serialized file of the current Geometry to the same file as was used previously.
    /// If it was not saved yet, behaves the same as saveProjectAs.
    void saveProject();

    /// Opens a file dialog to save the .p3d serialized file of the current Geometry.
    void saveProjectAs();

    /// Behaves same as Cinder's getAssetPath, but in this case, if the asset is not found, a fatal error Dialog is shown to the user.
    /// The fatal error Dialog also causes the application to terminate rendering.
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

    /// Behaves same as Cinder's loadAsset, but in this case, if the asset is not found, a fatal error Dialog is shown to the user.
    /// The fatal error Dialog also causes the application to terminate rendering.
    ci::DataSourceRef loadRequiredAsset(const ci::fs::path& relativePath) {
        getRequiredAssetPath(relativePath);
        return loadAsset(relativePath);
    }

   private:
    /// Setups Cinder logging (warnings and errors in Release) and FatalLogger.
    void setupLogging();

    /// Setups ImGui fonts and FontStorage.
    void setupFonts();

    /// Setups a WinAPI icon of the main window (only on Windows).
    void setupIcon();
    void drawExportDialog();

    /// Called when the main window loses focus.
    void willResignActive();

    /// Called when the main window becomes focused.
    void didBecomeActive();

    /// Returns true if the main window is obscured by another window or windows (only on Windows).
    /// Window is obscured if one of the following is true:
    /// 1) if the window is minimized, and/or
    /// 2) if the top left corner, middle, and bottom right corner are obscured by a different window.
    bool isWindowObscured();

    /// Detects if any error happened during the loading of new Geometry.
    /// Returns true if no errors occured (or only a warning occured).
    bool showLoadingErrorDialog();

    /// Loads Hotkeys from a file.
    void loadHotkeysFromFile(const std::string& path);

    /// Saves Hotkeys to a file.
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
