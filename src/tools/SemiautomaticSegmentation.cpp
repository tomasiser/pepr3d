#include "tools/SemiautomaticSegmentation.h"
#include "commands/CmdPaintSingleColor.h"
#include "geometry/SdfValuesException.h"

namespace pepr3d {

void SemiautomaticSegmentation::drawToSidePane(SidePane& sidePane) {
    if(!mGeometryCorrect) {
        sidePane.drawText("Polyhedron not built, since the geometry was damaged. Tool disabled.");
        return;
    }

    Geometry* const currentGeometry = mApplication.getCurrentGeometry();
    assert(currentGeometry != nullptr);
    if(currentGeometry == nullptr) {
        return;
    }

    const bool isSdfComputed = currentGeometry->isSdfComputed();
    if(!isSdfComputed) {
        sidePane.drawText("Warning: This computation may take a long time to perform.");
        if(sidePane.drawButton("Compute SDF")) {
            mApplication.enqueueSlowOperation([this]() { safeComputeSdf(mApplication); }, []() {}, true);
        }
        sidePane.drawTooltipOnHover("Compute the shape diameter function of the model to enable the segmentation.");
        sidePane.drawSeparator();
    } else {
        sidePane.drawColorPalette();
        sidePane.drawSeparator();

        if(mStartingTriangles.empty()) {
            sidePane.drawText("Draw with several colors to enable segmentation.");
            sidePane.drawSeparator();
        } else {
            sidePane.drawFloatDragger("Spread", mBucketSpread, 0.25f, 0.0f, 100.0f, "%.0f %%", 70.f);
            sidePane.drawTooltipOnHover(
                "The amount of growth each region will do. If your regions are small, increase this number.");

            // Normal stopping can be enabled by uncommenting this region.
            // Disabled for the time because it is wonky.
            // if(ImGui::RadioButton("Normals", mCriterionUsed == Criteria::NORMAL)) {
            //    mCriterionUsed = Criteria::NORMAL;
            //}
            // if(ImGui::RadioButton("SDF", mCriterionUsed == Criteria::SDF)) {
            //    mCriterionUsed = Criteria::SDF;
            //}

            if(mCriterionUsed == Criteria::SDF) {
                sidePane.drawCheckbox("Hard edges", mHardEdges);
                sidePane.drawTooltipOnHover(
                    "The growth will stop once meeting another color and will neither go under the color nor overwrite "
                    "the color.");
                if(!mHardEdges) {
                    sidePane.drawCheckbox("Region overlap", mRegionOverlap);
                    sidePane.drawTooltipOnHover("The colors will overwrite each other on the borders.");
                }
            }

            if(mBucketSpread != mBucketSpreadLatest || mHardEdges != mHardEdgesLatest ||
               mRegionOverlap != mRegionOverlapLatest) {
                mBucketSpreadLatest = mBucketSpread;
                mHardEdgesLatest = mHardEdges;
                mRegionOverlapLatest = mRegionOverlap;

                try {
                    spreadColors();  // Recompute the new color spread
                } catch(std::exception& e) {
                    const std::string errorCaption = "Error: Failed to spread the colors";
                    const std::string errorDescription =
                        "An internal error occured while spreading the colors. If the problem persists, try re-loading "
                        "the mesh. The coloring will now be reset.\n\n"
                        "Please report this bug to the developers. The full description of the problem is:\n";
                    mApplication.pushDialog(Dialog(DialogType::Error, errorCaption, errorDescription + e.what(), "OK"));
                    reset();
                    return;
                }
            }

            if(sidePane.drawButton("Apply")) {
                CommandManager<Geometry>* const commandManager = mApplication.getCommandManager();

                for(auto& toPaint : mCurrentColoring) {
                    if(toPaint.second.empty()) {
                        continue;
                    }
                    commandManager->execute(
                        std::make_unique<CmdPaintSingleColor>(std::move(toPaint.second), toPaint.first), false);
                }
                reset();
            }
            sidePane.drawTooltipOnHover("Apply the results to the model.");

            if(sidePane.drawButton("Cancel")) {
                reset();
            }
            sidePane.drawTooltipOnHover("Revert the model back to the previous coloring.");

            sidePane.drawSeparator();
        }
    }
}

size_t SemiautomaticSegmentation::findClosestColorFromSDF(
    const size_t triangle, const size_t color1, const size_t color2,
    const std::unordered_map<std::size_t, std::vector<double>>& sdfValuesPerColor) {
    const Geometry* const currentGeometry = mApplication.getCurrentGeometry();
    assert(currentGeometry != nullptr);

    double triangleSdfValue = currentGeometry->getSdfValue(triangle);

    auto findClosestSDFValue = [](double target, const std::vector<double>& values) -> double {
        const auto initialSdfValue = std::lower_bound(values.begin(), values.end(), target);
        double closest1 = -1;
        double closest2 = -1;
        if(initialSdfValue == values.end()) {
            closest1 = closest2 = values.back();
        } else {
            closest1 = *initialSdfValue;
            if(*initialSdfValue == values.front()) {
                closest2 = *initialSdfValue;
            } else {
                closest2 = *(initialSdfValue - 1);
            }
        }
        const double delta1 = abs(target - closest1);
        const double delta2 = abs(target - closest2);
        if(delta1 < delta2) {
            return closest1;
        } else {
            return closest2;
        }
    };

    const auto find1 = sdfValuesPerColor.find(color1);
    const auto find2 = sdfValuesPerColor.find(color2);

    if(find1 == sdfValuesPerColor.end() && find2 == sdfValuesPerColor.end()) {
        assert(false);
        return std::numeric_limits<size_t>::max();
    } else if(find1 == sdfValuesPerColor.end() || find1->second.empty()) {
        return color2;
    } else if(find2 == sdfValuesPerColor.end() || find2->second.empty()) {
        return color1;
    } else {
        const double closeToColor1 = findClosestSDFValue(triangleSdfValue, find1->second);
        const double closeToColor2 = findClosestSDFValue(triangleSdfValue, find2->second);
        const double delta1 = abs(triangleSdfValue - closeToColor1);
        const double delta2 = abs(triangleSdfValue - closeToColor2);
        if(delta1 < delta2) {
            return color1;
        } else {
            return color2;
        }
    }
}

bool SemiautomaticSegmentation::postprocess(
    const std::unordered_map<std::size_t, std::vector<double>>& sdfValuesPerColor) {
    std::unordered_map<std::size_t, std::size_t> triangleToColor;
    // Go through each color, create an assignment triangle->color
    for(const auto& oneColor : mCurrentColoring) {
        // Go through each triangle assigned to a single color
        for(const auto& tri : oneColor.second) {
            auto findTri = triangleToColor.find(tri);
            // If we did not assign this triangle before, assign it to currentColor
            if(findTri == triangleToColor.end()) {
                triangleToColor.insert({tri, oneColor.first});
            } else {  // If we did assign, check the SDF values and assign it to the closest color
                const size_t oldColor = findTri->second;
                const size_t currentColor = oneColor.first;
                const size_t retVal = findClosestColorFromSDF(tri, oldColor, currentColor, sdfValuesPerColor);
                if(retVal == std::numeric_limits<size_t>::max()) {
                    return false;
                }
                findTri->second = retVal;
            }
        }
    }

    // Re-collect the triangles by colors to get the resulting
    mCurrentColoring = collectTrianglesByColor(triangleToColor);
    return true;
}

void SemiautomaticSegmentation::spreadColors() {
    Geometry* const currentGeometry = mApplication.getCurrentGeometry();
    assert(currentGeometry != nullptr);

    mCurrentColoring.clear();
    auto& overrideBuffer = mApplication.getModelView().getOverrideColorBuffer();
    overrideBuffer = mBackupColorBuffer;

    // Collect all triangles of each color
    const std::unordered_map<std::size_t, std::vector<std::size_t>> trianglesByColor =
        collectTrianglesByColor(mStartingTriangles);

    /// Normal stopping init, uncomment in case we want to include it as a feature
    const Geometry* const p = const_cast<const Geometry*>(currentGeometry);
    const double angleRads = (100.0f - mBucketSpread) / 100.0f * 180.f * glm::pi<double>() / 180.0;
    NormalStopping stoppingFtor(p, angleRads);

    // Precompute data - gather all starting triangle SDF data into sdfValuesPerColor
    std::unordered_map<std::size_t, std::vector<double>> sdfValuesPerColor;
    for(const auto& colorTriangles : trianglesByColor) {
        const size_t currentColor = colorTriangles.first;
        const std::vector<size_t>& startingTriangles = colorTriangles.second;

        sdfValuesPerColor.insert({currentColor, {}});
        assert(sdfValuesPerColor.find(currentColor) != sdfValuesPerColor.end());
        std::vector<double>& initialValues = sdfValuesPerColor.find(currentColor)->second;

        initialValues.reserve(startingTriangles.size());
        for(size_t startI : startingTriangles) {
            initialValues.push_back(currentGeometry->getSdfValue(startI));
        }
    }

    // Compute the best region for each triangle
    std::unordered_map<std::size_t, std::size_t> triangleToBestRegion;
    for(size_t i = 0; i < currentGeometry->getTriangleCount(); ++i) {
        triangleToBestRegion.insert({i, 0});
        for(const auto& region : trianglesByColor) {
            const size_t regionColor = region.first;
            const std::vector<size_t>& regionTriangles = region.second;
            auto find = triangleToBestRegion.find(i);
            assert(find != triangleToBestRegion.end());
            find->second = findClosestColorFromSDF(i, find->second, regionColor, sdfValuesPerColor);
        }
    }

    // Bucket spread all the colors
    for(const auto& colorTriangles : trianglesByColor) {
        const size_t currentColor = colorTriangles.first;
        const std::vector<size_t>& startingTriangles = colorTriangles.second;
        if(startingTriangles.empty()) {
            continue;
        }

        assert(sdfValuesPerColor.find(currentColor) != sdfValuesPerColor.end());
        std::vector<double>& initialValues = sdfValuesPerColor.find(currentColor)->second;
        std::vector<size_t> ret;
        if(mCriterionUsed == Criteria::SDF) {
            SDFStopping SDFStopping(currentGeometry, initialValues, mBucketSpread / 100.0f, triangleToBestRegion,
                                    mHardEdges);
            ret = currentGeometry->bucket(startingTriangles, SDFStopping);
        } else {
            assert(mCriterionUsed == Criteria::NORMAL);
            ret = currentGeometry->bucket(startingTriangles, stoppingFtor);
        }

        // Remember the coloring
        mCurrentColoring.insert({currentColor, ret});
    }

    // Postprocess the newly calculated colorings
    if(!mRegionOverlap && mCriterionUsed == Criteria::SDF) {
        bool isPostOk = postprocess(sdfValuesPerColor);
        if(!isPostOk) {
            const std::string errorCaption = "Error: Failed to spread colors";
            const std::string errorDescription =
                "An invalid color setup was detected and the semiautomatic segmentation will be reset. "
                "Please report this bug to the developers.";
            mApplication.pushDialog(Dialog(DialogType::Error, errorCaption, errorDescription, "Reset segmentation"));
            reset();
            return;
        }
    }

    // Render all new colorings
    for(const auto& coloring : mCurrentColoring) {
        if(mApplication.getModelView().isMeshOverriden()) {
            for(const size_t tri : coloring.second) {
                const auto rgbTriangleColor = currentGeometry->getColorManager().getColor(coloring.first);
                overrideBuffer[3 * tri] = rgbTriangleColor;
                overrideBuffer[3 * tri + 1] = rgbTriangleColor;
                overrideBuffer[3 * tri + 2] = rgbTriangleColor;
            }
        }
    }
}

std::unordered_map<std::size_t, std::vector<std::size_t>> SemiautomaticSegmentation::collectTrianglesByColor(
    const std::unordered_map<std::size_t, std::size_t>& sourceTriangles) {
    std::unordered_map<std::size_t, std::vector<std::size_t>> result;
    const Geometry* const currentGeometry = mApplication.getCurrentGeometry();
    assert(currentGeometry != nullptr);

    for(size_t i = 0; i < currentGeometry->getColorManager().size(); ++i) {
        result.insert({i, {}});
    }

    for(const auto& colorTris : sourceTriangles) {
        const size_t triangleIndex = colorTris.first;
        const size_t triangleColor = colorTris.second;

        auto sameColorTriangles = result.find(triangleColor);
        assert(sameColorTriangles != result.end());
        sameColorTriangles->second.push_back(triangleIndex);
    }

    return result;
}

void SemiautomaticSegmentation::drawToModelView(ModelView& modelView) {
    if(mHoveredTriangleId) {
        modelView.drawTriangleHighlight(*mHoveredTriangleId);
    }
}

void SemiautomaticSegmentation::setupOverride() {
    std::vector<glm::vec4> newOverrideBuffer;
    const auto currentGeometry = mApplication.getCurrentGeometry();
    newOverrideBuffer.resize(currentGeometry->getTriangleCount() * 3);
    for(size_t i = 0; i < currentGeometry->getTriangleCount(); i++) {
        const size_t triangleColor = currentGeometry->getTriangleColor(i);
        const auto rgbTriangleColor = currentGeometry->getColorManager().getColor(triangleColor);
        newOverrideBuffer[3 * i] = rgbTriangleColor;
        newOverrideBuffer[3 * i + 1] = rgbTriangleColor;
        newOverrideBuffer[3 * i + 2] = rgbTriangleColor;
    }

    for(const auto& repaint : mStartingTriangles) {
        const size_t triIndex = repaint.first;
        const size_t colorIndex = repaint.second;
        const auto rgbTriangleColor = currentGeometry->getColorManager().getColor(colorIndex);
        newOverrideBuffer[3 * triIndex] = rgbTriangleColor;
        newOverrideBuffer[3 * triIndex + 1] = rgbTriangleColor;
        newOverrideBuffer[3 * triIndex + 2] = rgbTriangleColor;
    }

    mApplication.getModelView().toggleMeshOverride(true);
    mApplication.getModelView().initOverrideFromBasicGeoemtry();
    mBackupColorBuffer = newOverrideBuffer;
    mApplication.getModelView().getOverrideColorBuffer() = newOverrideBuffer;
}

void SemiautomaticSegmentation::setTriangleColor() {
    const auto activeColor = mApplication.getCurrentGeometry()->getColorManager().getActiveColorIndex();
    auto findSameColorList = mStartingTriangles.find(*mHoveredTriangleId);

    // If we don't have this triangle assigned, or it is assigned to a different color, reassign.
    if(findSameColorList == mStartingTriangles.end() ||
       (findSameColorList != mStartingTriangles.end() && findSameColorList->second != activeColor)) {
        size_t triIndex = *mHoveredTriangleId;

        if(findSameColorList == mStartingTriangles.end()) {
            mStartingTriangles.insert({*mHoveredTriangleId, activeColor});
        } else {
            findSameColorList->second = activeColor;
        }

        if(mApplication.getModelView().isMeshOverriden()) {
            const auto rgbTriangleColor = mApplication.getCurrentGeometry()->getColorManager().getColor(activeColor);
            auto& overrideBuffer = mApplication.getModelView().getOverrideColorBuffer();
            overrideBuffer[3 * triIndex] = rgbTriangleColor;
            overrideBuffer[3 * triIndex + 1] = rgbTriangleColor;
            overrideBuffer[3 * triIndex + 2] = rgbTriangleColor;
            mBackupColorBuffer[3 * triIndex] = rgbTriangleColor;
            mBackupColorBuffer[3 * triIndex + 1] = rgbTriangleColor;
            mBackupColorBuffer[3 * triIndex + 2] = rgbTriangleColor;
        }
    }
}

void SemiautomaticSegmentation::onToolDeselect(ModelView& modelView) {
    if(!mGeometryCorrect) {
        return;
    }
    reset();
}

void SemiautomaticSegmentation::onNewGeometryLoaded(ModelView& modelView) {
    Geometry* const currentGeometry = mApplication.getCurrentGeometry();
    assert(currentGeometry != nullptr);
    if(currentGeometry == nullptr) {
        return;
    }
    mGeometryCorrect = currentGeometry->polyhedronValid();
    if(!mGeometryCorrect) {
        return;
    }
    reset();
    mSdfEnabled = currentGeometry->sdfValuesValid();
}

void SemiautomaticSegmentation::onModelViewMouseDown(ModelView& modelView, ci::app::MouseEvent event) {
    if(!event.isLeftDown()) {
        return;
    }
    if(!mHoveredTriangleId) {
        return;
    }
    auto geometry = mApplication.getCurrentGeometry();
    if(geometry == nullptr) {
        return;
    }

    const bool emptyBefore = mStartingTriangles.empty();

    setTriangleColor();

    // Added the first triangle, override the buffer.
    if(emptyBefore && !mStartingTriangles.empty()) {
        setupOverride();
    } else if(!mDragging) {  // Restore the pre-spread buffer, reset the setting
        mApplication.getModelView().getOverrideColorBuffer() = mBackupColorBuffer;
        mBucketSpread = 0.f;
        mBucketSpreadLatest = 0.f;

        mRegionOverlapLatest = mRegionOverlap;
        mHardEdgesLatest = mHardEdges;
    }
}

void SemiautomaticSegmentation::onModelViewMouseDrag(ModelView& modelView, ci::app::MouseEvent event) {
    if(!event.isLeftDown()) {
        return;
    }

    mDragging = true;
    onModelViewMouseMove(modelView, event);
    onModelViewMouseDown(modelView, event);
    mDragging = false;

    // Restore the pre-spread buffer, reset the setting
    mApplication.getModelView().getOverrideColorBuffer() = mBackupColorBuffer;
    mBucketSpread = 0.f;
    mBucketSpreadLatest = 0.f;

    mRegionOverlapLatest = mRegionOverlap;
    mHardEdgesLatest = mHardEdges;
}

void SemiautomaticSegmentation::onModelViewMouseMove(ModelView& modelView, ci::app::MouseEvent event) {
    const auto lastRay = modelView.getRayFromWindowCoordinates(event.getPos());
    const auto geometry = mApplication.getCurrentGeometry();
    if(geometry == nullptr) {
        mHoveredTriangleId = {};
        return;
    }
    mHoveredTriangleId = geometry->intersectMesh(lastRay);
}

bool SemiautomaticSegmentation::isEnabled() const {
    if(mSdfEnabled == nullptr) {
        return mGeometryCorrect;
    }
    return mGeometryCorrect && *mSdfEnabled;
}

void SemiautomaticSegmentation::reset() {
    mBucketSpread = 0.0f;
    mBucketSpreadLatest = 0.0f;

    mRegionOverlap = false;
    mRegionOverlapLatest = false;

    mHardEdges = false;
    mHardEdgesLatest = false;

    mGeometryCorrect = true;
    mNormalStop = false;

    mSdfEnabled = nullptr;

    mCriterionUsed = Criteria::SDF;

    mHoveredTriangleId = {};
    mStartingTriangles.clear();
    mBackupColorBuffer.clear();
    mCurrentColoring.clear();

    mApplication.getModelView().getOverrideColorBuffer().clear();
    mApplication.getModelView().toggleMeshOverride(false);
}

}  // namespace pepr3d