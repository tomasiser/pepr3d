#include "tools/TextEditor.h"

#include <exception>
#include <glm/gtx/rotate_vector.hpp>
#include "imgui_stdlib.h"

namespace pepr3d {

void TextEditor::createPreviewMesh() const {
    auto& modelView = mApplication.getModelView();
    modelView.resetPreview();

    for(auto& letter : mRenderedText) {
        for(auto& t : letter) {
            modelView.previewTriangles.push_back(glm::vec3(t.a.x, t.a.y, t.a.z));
            modelView.previewTriangles.push_back(glm::vec3(t.b.x, t.b.y, t.b.z));
            modelView.previewTriangles.push_back(glm::vec3(t.c.x, t.c.y, t.c.z));
        }
    }

    const size_t triangleCount = static_cast<uint32_t>(modelView.previewTriangles.size());

    for(size_t i = 0; i < triangleCount; ++i) {
        modelView.previewIndices.push_back(static_cast<uint32_t>(i));
    }

    for(size_t i = 0; i < triangleCount; ++i) {
        modelView.previewNormals.push_back(glm::vec3(1, 1, 0));
    }

    const auto currentColor = mApplication.getCurrentGeometry()->getColorManager().getActiveColor();

    for(uint32_t i = 0; i < triangleCount; ++i) {
        modelView.previewColors.push_back(currentColor);
    }
}

std::vector<std::vector<FontRasterizer::Tri>> TextEditor::triangulateText() const {
    if(mFontPath == "") {
        throw std::runtime_error("Invalid font specified");
    }

    FontRasterizer fontTriangulate(mFontPath);
    assert(fontTriangulate.isValid());

    std::vector<std::vector<pepr3d::FontRasterizer::Tri>> result =
        fontTriangulate.rasterizeText(mText, mFontSize, mBezierSteps);

    CI_LOG_I("Text triangulated, " + std::to_string(result.size()) + " letters.");

    return result;
}

void TextEditor::transformToModelSpace(std::vector<std::vector<FontRasterizer::Tri>>& result) const {
    const ModelView& modelView = mApplication.getModelView();
    const glm::mat4 modelMatrix = modelView.getModelMatrix();
    const glm::mat4 modelMatrixInverse = glm::inverse(modelMatrix);
    const ci::CameraPersp& camera = modelView.getCamera();
    const glm::mat4 inverseView = camera.getInverseViewMatrix();

    const glm::mat4 finalRotate = modelMatrixInverse * inverseView;

    for(auto& letter : result) {
        for(auto& t : letter) {
            t.a = finalRotate * glm::vec4(t.a, 1);
            t.b = finalRotate * glm::vec4(t.b, 1);
            t.c = finalRotate * glm::vec4(t.c, 1);
        }
    }
}

void TextEditor::rotateText(std::vector<std::vector<FontRasterizer::Tri>>& text) {
    if(text.empty())
        return;

    assert(mLastIntersection);
    Geometry* geometry = mApplication.getCurrentGeometry();

    const glm::vec3 direction = -geometry->getTriangle(*mLastIntersection).getNormal();
    const glm::vec3 origin = getPreviewOrigin(direction);
    const glm::vec3 planeBase1 = getPlaneBaseVector(direction);
    const glm::vec3 planeBase2 = glm::cross(planeBase1, direction);

    glm::mat3 rotationMat(planeBase1, planeBase2, direction);

    for(auto& letter : text) {
        for(auto& tri : letter) {
            tri.a = origin + rotationMat * tri.a;
            tri.b = origin + rotationMat * tri.b;
            tri.c = origin + rotationMat * tri.c;
        }
    }
}

void TextEditor::generateAndUpdate() {
    mTriangulatedText = triangulateText();
    updateTextPreview();
}

void TextEditor::updateTextPreview() {
    mRenderedText = mTriangulatedText;
    rescaleText(mRenderedText);
    rotateText(mRenderedText);
    createPreviewMesh();
}

void TextEditor::rescaleText(std::vector<std::vector<FontRasterizer::Tri>>& result) {
    for(auto& letter : result) {
        for(auto& t : letter) {
            t.a *= mFontScale;
            t.b *= mFontScale;
            t.c *= mFontScale;
        }
    }

    std::pair<float, float> max = {std::numeric_limits<float>::min(), std::numeric_limits<float>::min()};
    std::pair<float, float> min = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};

    auto updateMinMax = [&min, &max](glm::vec3 pt) {
        if(pt.x > max.first) {
            max.first = pt.x;
        }
        if(pt.y > max.second) {
            max.second = pt.y;
        }

        if(pt.x < min.first) {
            min.first = pt.x;
        }
        if(pt.y < min.second) {
            min.second = pt.y;
        }
    };

    for(auto& letter : result) {
        for(auto& t : letter) {
            updateMinMax(t.a);
            updateMinMax(t.b);
            updateMinMax(t.c);
        }
    }

    for(auto& letter : result) {
        for(auto& t : letter) {
            t.a.x = t.a.x - min.first - max.first / 2.f;
            t.a.y = t.a.y - min.second - max.second / 2.f;
            t.a.z = -2.f;

            t.b.x = t.b.x - min.first - max.first / 2.f;
            t.b.y = t.b.y - min.second - max.second / 2.f;
            t.b.z = -2.f;

            t.c.x = t.c.x - min.first - max.first / 2.f;
            t.c.y = t.c.y - min.second - max.second / 2.f;
            t.c.z = -2.f;

            t.a.y *= -1;
            t.b.y *= -1;
            t.c.y *= -1;
        }
    }
}

glm::vec3 TextEditor::getPlaneBaseVector(const glm::vec3& direction) const {
    assert(glm::abs(glm::length(direction) - 1) < 0.01);  // Is normalized

    const glm::vec3 upVector(0.f, 0.f, 1.f);

    glm::vec3 otherDirection{};

    // Test if direction is already pointing upwards or downwards
    if(glm::abs(glm::dot(direction, upVector) > 0.98)) {
        otherDirection = glm::vec3(1.f, 0.f, 0.f);  // World right vector
    } else {
        otherDirection = upVector;
    }

    const auto baseVector = glm::cross(direction, otherDirection);
    return glm::rotate(baseVector, glm::radians(mTextRotation), direction);
}

glm::vec3 TextEditor::getPreviewOrigin(const glm::vec3& direction) const {
    assert(mLastIntersection);

    Geometry* geometry = mApplication.getCurrentGeometry();
    const float distFromModel = TEXT_DISTANCE_SCALE * mApplication.getModelView().getMaxSize();

    return mLastIntersectionPoint - direction * distFromModel;
}

void TextEditor::drawToSidePane(SidePane& sidePane) {
    sidePane.drawColorPalette();
    sidePane.drawSeparator();

    sidePane.drawText("Font: " + mFont);
    if(sidePane.drawButton("Load new font")) {
        std::vector<std::string> extensions = {"ttf"};

        mApplication.dispatchAsync([extensions, this]() {
            auto path = cinder::app::getOpenFilePath("", {"ttf"});
            mFontPath = path.string();
            mFont = path.filename().string();
        });
    }

    // -- Text settings --

    if(sidePane.drawIntDragger("Font size", mFontSize, 1, 10, 200, "%i", 50.f)) {
        generateAndUpdate();
    }

    if(sidePane.drawIntDragger("Bezier steps", mBezierSteps, 1, 1, 8, "%i", 50.f)) {
        generateAndUpdate();
    }

    if(ImGui::InputText("Text", &mText)) {
        generateAndUpdate();
    }

    // -- Preview settings --

    if(sidePane.drawFloatDragger("Font scale", mFontScale, .01f, 0.01f, 1.f, "%.02f", 50.f)) {
        updateTextPreview();
    }

    if(sidePane.drawFloatDragger("Text rotation", mTextRotation, 1.f, 0.f, 360.f, "%1.f", 50.f)) {
        updateTextPreview();
    }

    if(sidePane.drawButton("Place marker")) {
        mIsPlacingTextMarker = true;
        mLastIntersection = {};
    }

    if(sidePane.drawButton("Paint")) {
        paintText();
    }

    sidePane.drawSeparator();

    sidePane.drawText(mText);
    sidePane.drawText(mFontPath);
}

void TextEditor::onModelViewMouseDown(ModelView& modelView, ci::app::MouseEvent event) {
    if(!event.isLeft()) {
        return;
    }

    // Store current ray position if it is a hit
    if(mIsPlacingTextMarker && mLastIntersection) {
        mIsPlacingTextMarker = false;
        generateAndUpdate();
    }
}

void TextEditor::paintText() {
    if(mRenderedText.empty())
        return;

    // mApplication.getCommandManager()->execute(std::make_unique<CmdPaintText>(mRenderedText, ))
}

void TextEditor::onModelViewMouseMove(ModelView& modelView, ci::app::MouseEvent event) {
    if(mIsPlacingTextMarker) {
        mLastRay = modelView.getRayFromWindowCoordinates(event.getPos());
        Geometry* geometry = mApplication.getCurrentGeometry();
        mLastIntersection = geometry->intersectMesh(mLastRay, mLastIntersectionPoint);

        if(mLastIntersection && !mTriangulatedText.empty()) {
            updateTextPreview();
        }
    }
}

void TextEditor::drawToModelView(ModelView& modelView) {
    Geometry* geometry = mApplication.getCurrentGeometry();

    if(mLastIntersection) {
        const float modelSize = modelView.getMaxSize();
        const glm::vec3 triNormal = geometry->getTriangle(*mLastIntersection).getNormal();
        modelView.drawLine(mLastIntersectionPoint, mLastIntersectionPoint + triNormal * modelSize, ci::Color::black(),
                           mIsPlacingTextMarker ? 1.f : 2.f, true);

        const glm::vec3 direction = -triNormal;
        const glm::vec3 origin = getPreviewOrigin(direction);
        const glm::vec3 base1 = getPlaneBaseVector(direction);
        const glm::vec3 base2 = glm::cross(base1, direction);
        modelView.drawLine(origin, origin + base1, ci::Color(1.0f, 0.f, 0.f));
        modelView.drawLine(origin, origin + base2, ci::Color(0.0f, 1.f, 0.f));
    }
}
}  // namespace pepr3d