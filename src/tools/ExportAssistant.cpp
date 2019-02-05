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
    auto* const geometry = mApplication.getCurrentGeometry();
    assert(geometry != nullptr);

    sidePane.drawText("Export Type:");
    if(ImGui::RadioButton("Surfaces only", isSurfaceExport())) {
        mExportType = ExportType::Surface;
        validateExportType();
        mScenes.clear();
        resetOverride();
        setOverride();
    }
    sidePane.drawTooltipOnHover("Export only colored surfaces of the model.", "",
                                "Very fast export, but usually unusable for 3D printing directly. Use this to export "
                                "models for further editing in 3D editors.");
    if(ImGui::RadioButton("Depth extrusion", !isSurfaceExport())) {
        if(geometry->polyhedronValid() && geometry->isSdfComputed()) {
            mExportType = ExportType::PolyExtrusionWithSDF;
        } else {
            mExportType = ExportType::PolyExtrusion;
        }
        validateExportType();
        mIsPreviewUpToDate = false;
    }
    sidePane.drawTooltipOnHover(
        "Extrude colors inside the model in specified depths.", "",
        "Use this to export models directly for 3D printing. Suitable for Prusa printers and Slic3r Prusa Edition.");

    sidePane.drawSeparator();

    if(!isSurfaceExport()) {
        bool updateButtonClicked = false;
        if(mIsPreviewUpToDate) {
            updateButtonClicked = sidePane.drawButton("Update extrusion preview!");
        } else {
            updateButtonClicked =
                sidePane.drawColoredButton("Update extrusion preview!", ci::ColorA::hex(0xEB5757), 3.0f);
        }
        sidePane.drawTooltipOnHover("Update the preview of the exported model on the left side.");
        if(updateButtonClicked) {
            updateExtrusionPreview();
        }

        sidePane.drawSeparator();

        float minHeightPercent = mPreviewMinMaxHeight.x * 100.0f;
        float maxHeightPercent = mPreviewMinMaxHeight.y * 100.0f;
        sidePane.drawText("Preview Height");
        ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth());
        if(ImGui::DragFloatRange2("##range", &minHeightPercent, &maxHeightPercent, 0.25f, 0.0f, 100.0f, "Min: %.1f %%",
                                  "Max: %.1f %%")) {
            minHeightPercent = std::min(minHeightPercent, 99.9f);
            maxHeightPercent = std::max(0.1f, maxHeightPercent);
            mPreviewMinMaxHeight.x = minHeightPercent / 100.0f;
            mPreviewMinMaxHeight.y = maxHeightPercent / 100.0f;
            modelView.setPreviewMinMaxHeight(mPreviewMinMaxHeight);
        }
        sidePane.drawTooltipOnHover(
            "How much of the model will be shown in preview.", "",
            "Use this to see inside the model to verify that the extrusion is not too shallow or too deep.");
        ImGui::PopItemWidth();

        sidePane.drawSeparator();

        {
            auto* const geometry = mApplication.getCurrentGeometry();
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
                const glm::ivec2 cursorPos = ImGui::GetCursorScreenPos();
                ImGui::InvisibleButton("color",
                                       glm::vec2(boxSize, boxSize));  // we need the invisible button for the tooltip
                sidePane.drawTooltipOnHover("Color " + std::to_string(i + 1));
                const glm::vec4 color = colorManager.getColor(i);
                const glm::vec4 boxColor(color.r, color.g, color.b, 1.0);
                drawList->AddRectFilled(cursorPos + glm::ivec2(0, 0), cursorPos + glm::ivec2(boxSize, boxSize),
                                        (ImColor)boxColor);
                ImGui::NextColumn();
                if(ImGui::Checkbox("##show", &mSettingsPerColor[i].isShown)) {
                    resetOverride();
                    setOverride();
                }
                sidePane.drawTooltipOnHover(
                    "Show or hide this color in the preview.", "",
                    "Use this to see inside the model to verify that the extrusion is not too shallow or too deep.");
                ImGui::NextColumn();
                ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth());
                if(ImGui::DragFloat("##depth", &mSettingsPerColor[i].depth, 0.25f, 0.0f, 100.0f, "%.2f %%")) {
                    mIsPreviewUpToDate = false;
                }
                sidePane.drawTooltipOnHover("How deep should this color be extruded inside the model.", "",
                                            "Use this to adjust the extrusion so it is not too shallow or too "
                                            "deep.\n\nValue of 0% disables extrusion of this color. This is useful for "
                                            "closed parts of the model that you want to be filled completely.");

                ImGui::PopItemWidth();
                ImGui::NextColumn();
                ImGui::PopID();
            }
            ImGui::Columns(1);
            ImGui::Separator();

            if(geometry->polyhedronValid() &&
               (geometry->sdfValuesValid() == nullptr || *(geometry->sdfValuesValid()))) {
                sidePane.drawText("Depth values are:");
                if(ImGui::RadioButton("absolute", mExportType == ExportType::PolyExtrusion)) {
                    mExportType = ExportType::PolyExtrusion;
                    mIsPreviewUpToDate = false;
                }
                sidePane.drawTooltipOnHover(
                    "Extrusion depths will be exactly as you set them.", "",
                    "Use this for simple models or if SDF extrusion gives wrong results. For complex models with "
                    "varying thickness, consider instead using the relative SDF option.");
                ImGui::SameLine();
                if(ImGui::RadioButton("relative to SDF", mExportType == ExportType::PolyExtrusionWithSDF)) {
                    mExportType = ExportType::PolyExtrusionWithSDF;
                    mIsPreviewUpToDate = false;
                }
                sidePane.drawTooltipOnHover(
                    "Extrusion depths will depend on local thickness of parts of the model.", "",
                    "Use this for complex models with varying thickness, e.g., models with spikes, little details, "
                    "etc.\n\nWarning! Initial SDF calculation for big models may be slow!");
            }
        }
    }

    sidePane.drawSeparator();

    {
        sidePane.drawText("Export As:");
        if(ImGui::RadioButton(".stl", mExportFileType == "stl")) {
            mExportFileType = "stl";
        }
        sidePane.drawTooltipOnHover(
            "Export as separate .stl files.", "",
            "This is a binary format suitable for 3D printing with Prusa printers and Slic3r Prusa Edition.");
        ImGui::SameLine();
        if(ImGui::RadioButton(".ply", mExportFileType == "ply")) {
            mExportFileType = "ply";
        }
        sidePane.drawTooltipOnHover("Export as separate .ply files.", "",
                                    "This is a binary Stanford Triangle Format supported by standard 3D editors.");
        ImGui::SameLine();
        if(ImGui::RadioButton(".obj", mExportFileType == "obj")) {
            mExportFileType = "obj";
        }
        sidePane.drawTooltipOnHover("Export as separate .obj and .mat files.", "",
                                    "This is a simple non-binary format supported by standard 3D editors.");

        ImGui::Checkbox("Create a new folder", &mShouldExportInNewFolder);
        sidePane.drawTooltipOnHover("If checked, a new separate folder will be created for the exported files.");

        if(sidePane.drawButton("Export files")) {
            exportFiles();
        }
        sidePane.drawTooltipOnHover("Save the exported model in separate files using the options above.");
    }

    mIsFirstFrame = false;
}

void ExportAssistant::drawToModelView(ModelView& modelView) {
    std::string errorCaption;
    if(isSurfaceExport()) {
        errorCaption = "No preview available for surface export.";
    } else if(!mIsPreviewUpToDate) {
        errorCaption = "Please update the extrusion preview.";
    }
    modelView.drawCaption("Export Preview", errorCaption);
}

void ExportAssistant::onToolSelect(ModelView& modelView) {
    if(mExporter == nullptr) {
        onNewGeometryLoaded(modelView);
    }
    modelView.setPreviewMinMaxHeight(mPreviewMinMaxHeight);
    updateSettings();
    setOverride();

    auto* const commandManager = mApplication.getCommandManager();
    assert(commandManager != nullptr);
    if(mLastVersionPreviewed != commandManager->getVersionNumber()) {
        mIsPreviewUpToDate = false;
    }
}

void ExportAssistant::onToolDeselect(ModelView& modelView) {
    resetOverride();
}

void ExportAssistant::onNewGeometryLoaded(ModelView& modelView) {
    auto* const geometry = mApplication.getCurrentGeometry();
    assert(geometry != nullptr);
    mExporter = std::make_unique<ModelExporter>(geometry, &geometry->getProgress());
    mScenes.clear();
    mIsPreviewUpToDate = false;
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
    auto* const geometry = mApplication.getCurrentGeometry();
    assert(geometry != nullptr);

    uint32_t bufferOffset = 0;
    for(auto& scene : mScenes) {
        const size_t colorIndex = scene.first;
        assert(mSettingsPerColor.size() > colorIndex);
        if(!mSettingsPerColor[colorIndex].isShown) {
            continue;
        }
        const glm::vec4 color = geometry->getColorManager().getColor(colorIndex);
        aiScene* const sceneData = scene.second.get();
        assert(sceneData->mNumMeshes == 1);
        aiMesh* const mesh = sceneData->mMeshes[0];
        for(size_t i = 0; i < mesh->mNumVertices; ++i) {
            modelView.getOverrideColorBuffer().push_back(color);
            modelView.getOverrideNormalBuffer().emplace_back(mesh->mNormals[i].x, mesh->mNormals[i].y,
                                                             mesh->mNormals[i].z);
            modelView.getOverrideVertexBuffer().emplace_back(mesh->mVertices[i].x, mesh->mVertices[i].y,
                                                             mesh->mVertices[i].z);
        }
        for(size_t i = 0; i < mesh->mNumFaces; ++i) {
            const aiFace face = mesh->mFaces[i];
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
    auto* const geometry = mApplication.getCurrentGeometry();
    assert(geometry != nullptr);

    if(mSettingsPerColor.size() != geometry->getColorManager().size()) {
        mSettingsPerColor.resize(geometry->getColorManager().size());
    }

    validateExportType();
}

void ExportAssistant::validateExportType() {
    auto* const geometry = mApplication.getCurrentGeometry();
    assert(geometry != nullptr);

    if(geometry->polyhedronValid()) {
        if(mExportType == ExportType::NonPolySurface) {
            mExportType = ExportType::Surface;
        } else if(mExportType == ExportType::NonPolyExtrusion) {
            mExportType = ExportType::PolyExtrusion;
        } else if(mExportType == ExportType::PolyExtrusionWithSDF && geometry->sdfValuesValid() != nullptr &&
                  !*(geometry->sdfValuesValid())) {
            mExportType = ExportType::PolyExtrusion;
        }
    } else {
        if(mExportType == ExportType::Surface) {
            mExportType = ExportType::NonPolySurface;
        } else if(mExportType == ExportType::PolyExtrusion || mExportType == ExportType::PolyExtrusionWithSDF) {
            mExportType = ExportType::NonPolyExtrusion;
        }
    }
}

void ExportAssistant::exportFiles() {
    mApplication.dispatchAsync([this]() {
        cinder::fs::path initialPath(mApplication.getGeometryFileName());
        std::string name = initialPath.filename().replace_extension().string();
        initialPath.remove_filename();
        if(initialPath.empty()) {
            initialPath = ci::getDocumentsDirectory();
            name = "untitled";
        } else {
            name += "_exported";
        }

        std::string fileType;
        std::vector<std::string> extensions;
        extensions.emplace_back(mExportFileType);
        name += "." + mExportFileType;
        fileType = mExportFileType;

        const auto path = mApplication.getSaveFilePath(initialPath.append(name), extensions);

        if(!path.empty()) {
            const std::string fileName = path.filename().string().substr(0, path.filename().string().find_last_of("."));
            std::string filePath = path.parent_path().string();
            if(mShouldExportInNewFolder) {
                filePath += std::string("/") + std::string(fileName);
            }

            if(!cinder::fs::is_directory(filePath) || !cinder::fs::exists(filePath)) {  // check if folder exists
                cinder::fs::create_directory(filePath);
            }

            mApplication.enqueueSlowOperation(
                [filePath, fileName, fileType, this]() {
                    try {
                        prepareExport();
                        mExporter->saveModel(filePath, fileName, fileType, mExportType);
                    } catch(std::exception& e) {
                        pushErrorDialog(e.what());
                        updateSettings();
                    }
                },
                [this]() {}, true);
        }
    });
}

void ExportAssistant::updateExtrusionPreview() {
    mApplication.enqueueSlowOperation(
        [this]() {
            try {
                prepareExport();
                mScenes = mExporter->createScenes(mExportType);
            } catch(std::exception& e) {
                pushErrorDialog(e.what());
                updateSettings();
            }
        },
        [this]() {
            auto* const commandManager = mApplication.getCommandManager();
            assert(commandManager != nullptr);
            mLastVersionPreviewed = commandManager->getVersionNumber();
            mIsPreviewUpToDate = true;
            resetOverride();  // do not call this before enqueueSlowOperation(), otherwise ModelView would render the
                              // geometry, but that is not thread-safe as we modify it during prepareExport()
            setOverride();
        },
        true);
}

void ExportAssistant::prepareExport() {
    auto* const geometry = mApplication.getCurrentGeometry();
    assert(geometry != nullptr);

    if(!isSurfaceExport()) {
        if(mExportType == ExportType::PolyExtrusionWithSDF && !geometry->isSdfComputed()) {
            if(!safeComputeSdf(mApplication)) {
                throw std::runtime_error("Could not compute SDF. Please export using absolute depth values.");
            }
        }

        std::vector<float> extrusionCoefs;
        for(auto& colorSetting : mSettingsPerColor) {
            extrusionCoefs.push_back(colorSetting.depth / 100.0f);  // from [0, 100]% to [0, 1]
        }

        assert(mExporter != nullptr);
        mExporter->setExtrusionCoef(extrusionCoefs);
    }

    if(geometry->polyhedronValid()) {
        geometry->updateTemporaryDetailedData();
    }
}

void ExportAssistant::pushErrorDialog(const std::string& errorDetails) {
    const std::string errorCaption = "Error: Failed to export";
    const std::string errorDescription =
        "An error has occured while exporting the model or showing the export preview.\n\n";
    mApplication.pushDialog(Dialog(DialogType::Error, errorCaption, errorDescription + errorDetails));
}

}  // namespace pepr3d