#pragma once
#include <string>
#include <vector>

#include <cinder/app/AppBase.h>
#include "geometry/FontRasterizer.h"
#include "tools/Tool.h"
#include "ui/IconsMaterialDesign.h"
#include "ui/MainApplication.h"
#include "ui/SidePane.h"

namespace pepr3d {

/// A tool used for painting texts on a model
class TextEditor : public Tool {
   public:
    TextEditor(MainApplication& app) : mApplication(app) {
        auto fontPath = cinder::app::getAssetPath("fonts/OpenSans-Regular.ttf");
        mFont = fontPath.filename().string();
        mFontPath = fontPath.string();
    }

    virtual std::string getName() const override {
        return "Text Editor";
    }

    virtual std::optional<Hotkey> getHotkey(const Hotkeys& hotkeys) const override {
        return hotkeys.findHotkey(HotkeyAction::SelectTextEditor);
    }

    virtual std::string getIcon() const override {
        return ICON_MD_TEXT_FIELDS;
    }

    virtual bool isEnabled() const override;

    virtual void drawToSidePane(SidePane& sidePane) override;
    virtual void onModelViewMouseDown(ModelView& modelView, ci::app::MouseEvent event) override;
    virtual void onModelViewMouseMove(ModelView& modelView, ci::app::MouseEvent event) override;
    virtual void drawToModelView(ModelView& modelView) override;

    virtual void onToolSelect(ModelView& modelView) override {
        if(!mTriangulatedText.empty() && mSelectedIntersection) {
            updateTextPreview();
        }
    }

    virtual void onToolDeselect(ModelView& modelView) override {
        modelView.resetPreview();
    }

    virtual void onNewGeometryLoaded(ModelView& modelView) override {
        mCurrentIntersection = {};
        mSelectedIntersection = {};
    }

    /// Paint prepared text onto the model
    void paintText();

   private:
    MainApplication& mApplication;

    /// Intersection and point used for for text projection
    ci::Ray mSelectedRay{};
    std::optional<size_t> mSelectedIntersection{};
    glm::vec3 mSelectedIntersectionPoint{};

    /// Intersection and point used for cursor highlight
    ci::Ray mCurrentRay{};
    std::optional<size_t> mCurrentIntersection{};
    glm::vec3 mCurrentIntersectionPoint{};

    std::string mFont;
    std::string mFontPath;
    std::string mText = "Pepr3D";
    int mFontSize = 12;
    int mBezierSteps = 3;
    float mFontScale = 0.2f;

    /// Rotation of text in degrees
    float mTextRotation = 0.f;

    /// How far should the text preview placed from the model (normalized by model scale)
    const float TEXT_DISTANCE_SCALE = 0.02f;

    std::vector<std::vector<FontRasterizer::Tri>> mTriangulatedText;
    std::vector<std::vector<FontRasterizer::Tri>> mRenderedText;

    std::vector<std::vector<FontRasterizer::Tri>> triangulateText() const;

    /// Update modelView's preview data with new triangles to render
    void createPreviewMesh() const;

    /// Generate text and update preview
    void generateAndUpdate();

    /// Update text preview without generating again
    void updateTextPreview();

    /// Rotate to face ray direction
    void rotateText(std::vector<std::vector<FontRasterizer::Tri>>& text);

    void rescaleText(std::vector<std::vector<FontRasterizer::Tri>>& result);

    /// Get vector perpendicular to the direction, that is pointing towards the right halfplane
    glm::vec3 getPlaneBaseVector(const glm::vec3& direction) const;

    /// Get origin point of the text preview
    glm::vec3 getPreviewOrigin(const glm::vec3& direction) const;
};
}  // namespace pepr3d
