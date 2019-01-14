#include "tools/TextEditor.h"

#include <exception>
#include "imgui_stdlib.h"

namespace pepr3d {

void TextEditor::renderText(std::vector<std::vector<FontRasterizer::Tri>>& result, bool reset) const {
    auto& modelView = mApplication.getModelView();

    if(reset) {
        modelView.debugTriangles.clear();
        modelView.debugIndices.clear();
        modelView.debugColors.clear();
        modelView.debugNormals.clear();
    }
    const size_t alreadyThere = modelView.debugTriangles.size();

    for(auto& letter : result) {
        for(auto& t : letter) {
            modelView.debugTriangles.push_back(glm::vec3(t.a.x, t.a.y, t.a.z));
            modelView.debugTriangles.push_back(glm::vec3(t.b.x, t.b.y, t.b.z));
            modelView.debugTriangles.push_back(glm::vec3(t.c.x, t.c.y, t.c.z));
        }
    }

    const size_t added = modelView.debugTriangles.size() - alreadyThere;

    for(uint32_t i = 0; i < added; ++i) {
        modelView.debugIndices.push_back(alreadyThere + i);
    }

    for(int i = 0; i < added; ++i) {
        modelView.debugNormals.push_back(glm::vec3(1, 1, 0));
    }

    for(uint32_t i = 0; i < added; ++i) {
        if(reset) {
            modelView.debugColors.push_back(glm::vec4(0.2, 1, 0.2, 1));
        } else {
            modelView.debugColors.push_back(glm::vec4(1, 0.2, 0.2, 1));
        }
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

void TextEditor::rotateText(std::vector<std::vector<FontRasterizer::Tri>>& result) const {
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

void TextEditor::processText() {
    auto triPerLetter = triangulateText();

    rescaleText(triPerLetter);

    renderText(triPerLetter, true);

    rotateText(triPerLetter);

    renderText(triPerLetter, false);

    mRenderedText = std::move(triPerLetter);
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
    sidePane.drawIntDragger("Font size", mFontSize, 1, 10, 200, "%i", 50.f);
    sidePane.drawIntDragger("Bezier steps", mBezierSteps, 1, 1, 8, "%i", 50.f);

    ImGui::InputText("Text", &mText);

    sidePane.drawFloatDragger("Font scale", mFontScale, .01f, 0.01f, 1.f, "%.02f", 50.f);

    if(sidePane.drawButton("Go")) {
        processText();
    }

    sidePane.drawSeparator();

    sidePane.drawText(mText);
    sidePane.drawText(mFontPath);
}
}  // namespace pepr3d