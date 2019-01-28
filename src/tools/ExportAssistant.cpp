#include "tools/ExportAssistant.h"
#include <random>
#include <vector>
#include "commands/CmdPaintSingleColor.h"
#include "geometry/ModelExporter.h"
#include "geometry/SdfValuesException.h"
#include "ui/MainApplication.h"

namespace pepr3d {

void ExportAssistant::drawToSidePane(SidePane& sidePane) {
    auto& modelView = mApplication.getModelView();

    if(sidePane.drawColoredButton("Update preview!", ci::ColorA::hex(0xEB5757), 3.0f)) {
        resetOverride();

        auto* geometry = mApplication.getCurrentGeometry();
        assert(geometry != nullptr);
        auto exporter = geometry->exportGeometry(ModelExporter::ExportTypes::PolyWithSDF);
        mScenes = exporter.createScenes(ModelExporter::ExportTypes::PolyWithSDF);

        setOverride();
    }

    sidePane.drawSeparator();

    float minHeightPercent = mPreviewMinMaxHeight.x * 100.0f;
    float maxHeightPercent = mPreviewMinMaxHeight.y * 100.0f;
    sidePane.drawText("Preview Height");
    ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth());
    if(ImGui::DragFloatRange2("##range", &minHeightPercent, &maxHeightPercent, 0.25f, 0.0f, 100.0f, "Lowest: %.1f %%",
                              "Highest: %.1f %%")) {
        minHeightPercent = std::min(minHeightPercent, 99.9f);
        maxHeightPercent = std::max(0.1f, maxHeightPercent);
        mPreviewMinMaxHeight.x = minHeightPercent / 100.0f;
        mPreviewMinMaxHeight.y = maxHeightPercent / 100.0f;
        modelView.setPreviewMinMaxHeight(mPreviewMinMaxHeight);
    }
    ImGui::PopItemWidth();

    sidePane.drawSeparator();
    // sidePane.drawText("Per-color Settings");

    {
        auto* geometry = mApplication.getCurrentGeometry();
        assert(geometry != nullptr);
        const auto& colorManager = geometry->getColorManager();

        ImGui::Columns(3, "##percolorsettings");
        const float boxSize = ImGui::CalcTextSize("").y + 2.0f * ImGui::GetStyle().FramePadding.y;
        if(mIsFirstFrame) {
            ImGui::SetColumnWidth(0, 2.0f * boxSize);
        }
        ImGui::Separator();
        ImGui::Text("Color");
        ImGui::NextColumn();
        ImGui::Text("Preview");
        ImGui::NextColumn();
        ImGui::Text("Depth");
        ImGui::NextColumn();
        ImGui::Separator();
        ImDrawList* const drawList = ImGui::GetWindowDrawList();
        for(size_t i = 0; i < colorManager.size(); ++i) {
            ImGui::PushID(static_cast<int>(i));
            glm::ivec2 cursorPos = ImGui::GetCursorScreenPos();
            glm::vec4 color = colorManager.getColor(i);
            const glm::vec4 boxColor(color.r, color.g, color.b, 1.0);
            drawList->AddRectFilled(cursorPos + glm::ivec2(0, 0), cursorPos + glm::ivec2(boxSize, boxSize),
                                    (ImColor)boxColor);
            ImGui::NextColumn();
            if(ImGui::Checkbox("##show", &mSettingsPerColor[i].isShown)) {
                resetOverride();
                setOverride();
            }
            ImGui::NextColumn();
            if(ImGui::Checkbox("##extrude", &mSettingsPerColor[i].isShown)) {
            }
            ImGui::SameLine();
            float depth = 0.0f;
            ImGui::DragFloat("##depth", &depth, 0.25f, 0.0f, std::numeric_limits<float>::max(), "%.1f %%");
            ImGui::NextColumn();
            ImGui::PopID();
        }
        ImGui::Columns(1);
        ImGui::Separator();
        sidePane.drawText("Depth values are:");
        ImGui::RadioButton("absolute", false);
        ImGui::SameLine();
        ImGui::RadioButton("relative to SDF", true);
    }

    sidePane.drawSeparator();
    sidePane.drawText("Export to Files");

    {
        sidePane.drawText("File type:");
        bool yes = true;
        bool no = false;
        ImGui::RadioButton(".stl", yes);
        ImGui::SameLine();
        ImGui::RadioButton(".ply", no);
        ImGui::SameLine();
        ImGui::RadioButton(".obj", no);

        ImGui::Checkbox("Create a new folder", &no);

        sidePane.drawButton("Export");
    }

    mIsFirstFrame = false;
}

void ExportAssistant::onToolSelect(ModelView& modelView) {
    modelView.setPreviewMinMaxHeight(mPreviewMinMaxHeight);
    updateSettings();
    setOverride();
}

void ExportAssistant::onToolDeselect(ModelView& modelView) {
    resetOverride();
}

void ExportAssistant::resetOverride() {
    auto& modelView = mApplication.getModelView();
    modelView.setVertexNormalIndexOverride(false);
    modelView.setColorOverride(false);
    modelView.getOverrideColorBuffer().clear();
    modelView.getOverrideIndexBuffer().clear();
    modelView.getOverrideNormalBuffer().clear();
    modelView.getOverrideVertexBuffer().clear();
    modelView.setPreviewMinMaxHeight(glm::vec2(0.0f, 1.0f));
}

void ExportAssistant::setOverride() {
    auto& modelView = mApplication.getModelView();
    auto* geometry = mApplication.getCurrentGeometry();
    assert(geometry != nullptr);

    uint32_t bufferOffset = 0;
    for(auto& scene : mScenes) {
        size_t colorIndex = scene.first;
        assert(mSettingsPerColor.size() > colorIndex);
        if(!mSettingsPerColor[colorIndex].isShown) {
            continue;
        }
        glm::vec4 color = geometry->getColorManager().getColor(colorIndex);
        aiScene* sceneData = scene.second.get();
        assert(sceneData->mNumMeshes == 1);
        aiMesh* mesh = sceneData->mMeshes[0];
        for(size_t i = 0; i < mesh->mNumVertices; ++i) {
            modelView.getOverrideColorBuffer().push_back(color);
            modelView.getOverrideNormalBuffer().emplace_back(mesh->mNormals[i].x, mesh->mNormals[i].y,
                                                             mesh->mNormals[i].z);
            modelView.getOverrideVertexBuffer().emplace_back(mesh->mVertices[i].x, mesh->mVertices[i].y,
                                                             mesh->mVertices[i].z);
        }
        for(size_t i = 0; i < mesh->mNumFaces; ++i) {
            aiFace face = mesh->mFaces[i];
            assert(face.mNumIndices == 3);
            modelView.getOverrideIndexBuffer().push_back(bufferOffset + face.mIndices[0]);
            modelView.getOverrideIndexBuffer().push_back(bufferOffset + face.mIndices[1]);
            modelView.getOverrideIndexBuffer().push_back(bufferOffset + face.mIndices[2]);
        }
        bufferOffset += mesh->mNumVertices;
    }

    modelView.setColorOverride(true);
    modelView.setVertexNormalIndexOverride(true);
    modelView.setPreviewMinMaxHeight(mPreviewMinMaxHeight);
}

void ExportAssistant::updateSettings() {
    auto* geometry = mApplication.getCurrentGeometry();
    assert(geometry != nullptr);

    if(mSettingsPerColor.size() != geometry->getColorManager().size()) {
        mSettingsPerColor.resize(geometry->getColorManager().size());
    }
}

}  // namespace pepr3d