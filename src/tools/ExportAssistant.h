#pragma once
#include <map>
#include <memory>
#include <optional>
#include <unordered_map>
#include "commands/CommandManager.h"
#include "geometry/Geometry.h"
#include "geometry/ModelExporter.h"
#include "tools/Tool.h"
#include "ui/IconsMaterialDesign.h"
#include "ui/ModelView.h"
#include "ui/SidePane.h"

namespace pepr3d {

class ExportAssistant : public Tool {
   public:
    explicit ExportAssistant(MainApplication& app) : mApplication(app) {}

    virtual std::string getName() const override {
        return "Export Assistant";
    }

    virtual std::string getDescription() const override {
        return "Export the model so it can be printed, used in slicers, or in other 3D editors.";
    }

    virtual std::optional<Hotkey> getHotkey(const Hotkeys& hotkeys) const override {
        return hotkeys.findHotkey(HotkeyAction::Export);
    }

    virtual std::string getIcon() const override {
        return ICON_MD_ARCHIVE;
    }

    virtual bool isEnabled() const override {
        return true;
    }

    virtual void drawToSidePane(SidePane& sidePane) override;
    // virtual void drawToModelView(ModelView& modelView) override;
    // virtual void onModelViewMouseDown(ModelView& modelView, ci::app::MouseEvent event) override;
    // virtual void onModelViewMouseUp(ModelView& modelView, ci::app::MouseEvent event) override;
    // virtual void onModelViewMouseDrag(ModelView& modelView, ci::app::MouseEvent event) override;
    // virtual void onModelViewMouseMove(ModelView& modelView, ci::app::MouseEvent event) override;
    virtual void onToolSelect(ModelView& modelView) override;
    virtual void onToolDeselect(ModelView& modelView) override;
    virtual void onNewGeometryLoaded(ModelView& modelView) override;

   private:
    MainApplication& mApplication;
    glm::vec2 mPreviewMinMaxHeight = glm::vec2(0.0f, 1.0f);
    bool mIsFirstFrame = true;

    /// Map of colors (color index) and Assimp scenes representing them
    std::map<size_t, std::unique_ptr<aiScene>> mScenes;

    struct SettingsPerColor {
        bool isShown = true;
        float depth = 2.5f;
    };

    /// Settings per color, indexed by the color index
    std::vector<SettingsPerColor> mSettingsPerColor;

    void resetOverride();
    void setOverride();
    void updateSettings();
    void validateExportType();
    void exportFiles();
    void updateExtrusionPreview();
    void prepareExport();
    void pushErrorDialog(const std::string& errorDetails);

    ExportType mExportType = ExportType::PolyExtrusion;

    bool isSurfaceExport() {
        return mExportType == ExportType::Surface || mExportType == ExportType::NonPolySurface;
    }

    std::unique_ptr<ModelExporter> mExporter;

    size_t mLastVersionPreviewed = std::numeric_limits<size_t>::max();
    bool mIsPreviewUpToDate = false;
    bool mShouldExportInNewFolder = false;
    std::string mExportFileType = "stl";
};
}  // namespace pepr3d
