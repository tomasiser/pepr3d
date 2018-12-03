#include "tools/SemiautomaticSegmentation.h"
#include "commands/CmdPaintSingleColor.h"

namespace pepr3d {

void SemiautomaticSegmentation::drawToSidePane(SidePane& sidePane) {
    Geometry* const currentGeometry = mApplication.getCurrentGeometry();
    assert(currentGeometry != nullptr);

    const bool isSdfComputed = currentGeometry->isSdfComputed();
    if(!isSdfComputed) {
        sidePane.drawText("Warning: This computation may\ntake a long time to perform.");
        if(sidePane.drawButton("Compute SDF")) {
            currentGeometry->preSegmentation();
        }
    } else {
        sidePane.drawColorPalette(currentGeometry->getColorManager());
        sidePane.drawSeparator();

        if(mStartingTriangles.empty()) {
            sidePane.drawText("Draw with several colors to\nenable segmentation.");
        } else {
            sidePane.drawFloatDragger("Spread", mBucketSpread, .01f, 0.0f, 1.f, "%.02f", 70.f);

            // Update only if the bucket setting changed and we should re-fill
            if(mBucketSpread != mBucketSpreadLatest) {
                spreadColors();
            }

            if(sidePane.drawButton("Apply")) {
                CommandManager<Geometry>* const commandManager = mApplication.getCommandManager();

                for(auto& toPaint : mCurrentColoring) {
                    commandManager->execute(
                        std::make_unique<CmdPaintSingleColor>(std::move(toPaint.second), toPaint.first), false);
                }
                reset();
            }

            if(sidePane.drawButton("Cancel")) {
                reset();
            }
        }
    }
}

void SemiautomaticSegmentation::spreadColors() {
    Geometry* const currentGeometry = mApplication.getCurrentGeometry();
    mCurrentColoring.clear();
    mBucketSpreadLatest = mBucketSpread;
    auto& overrideBuffer = mApplication.getModelView().getOverrideColorBuffer();
    overrideBuffer = mBackupColorBuffer;

    // Collect all triangles of each color
    const std::unordered_map<std::size_t, std::vector<std::size_t>> trianglesByColor = collectTrianglesByColor();

    ///// Normal stopping init, uncomment in case we want to include it as a feature
    // const Geometry* const p = const_cast<const Geometry*>(currentGeometry);
    // const double angleRads = (1 - mBucketSpread) * 180.f * glm::pi<double>() / 180.0;
    // NormalStopping stoppingFtor(p, angleRads);

    // Bucket spread all the colors
    for(const auto& colorTriangles : trianglesByColor) {
        const size_t currentColor = colorTriangles.first;
        const std::vector<size_t>& startingTriangles = colorTriangles.second;
        if(startingTriangles.empty()) {
            continue;
        }

        std::vector<double> initialValues;
        initialValues.reserve(startingTriangles.size());
        for(size_t startI : startingTriangles) {
            initialValues.push_back(currentGeometry->getSdfValue(startI));
        }

        SDFStopping SDFStopping(currentGeometry, initialValues, mBucketSpread);
        const std::vector<size_t> ret = currentGeometry->bucket(startingTriangles, SDFStopping);

        // Remember the coloring
        mCurrentColoring.insert({currentColor, ret});

        // Render it
        if(mApplication.getModelView().isColorOverride()) {
            for(const size_t tri : ret) {
                const auto rgbTriangleColor = currentGeometry->getColorManager().getColor(currentColor);
                overrideBuffer[3 * tri] = rgbTriangleColor;
                overrideBuffer[3 * tri + 1] = rgbTriangleColor;
                overrideBuffer[3 * tri + 2] = rgbTriangleColor;
            }
        }
    }
}

std::unordered_map<std::size_t, std::vector<std::size_t>> SemiautomaticSegmentation::collectTrianglesByColor() {
    std::unordered_map<std::size_t, std::vector<std::size_t>> result;
    const Geometry* const currentGeometry = mApplication.getCurrentGeometry();

    for(size_t i = 0; i < currentGeometry->getColorManager().size(); ++i) {
        result.insert({i, {}});
    }

    for(const auto& colorTris : mStartingTriangles) {
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

    mBackupColorBuffer = newOverrideBuffer;
    mApplication.getModelView().getOverrideColorBuffer() = newOverrideBuffer;
    mApplication.getModelView().setColorOverride(true);
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

        if(mApplication.getModelView().isColorOverride()) {
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
    reset();
}

void SemiautomaticSegmentation::onNewGeometryLoaded(ModelView& modelView) {
    reset();
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
    }
}

void SemiautomaticSegmentation::onModelViewMouseDrag(ModelView& modelView, ci::app::MouseEvent event) {
    onModelViewMouseMove(modelView, event);
    onModelViewMouseDown(modelView, event);
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

void SemiautomaticSegmentation::reset() {
    mBucketSpread = 0.0f;
    mBucketSpreadLatest = 0.0f;
    mNormalStop = false;

    mHoveredTriangleId = {};
    mStartingTriangles.clear();
    mBackupColorBuffer.clear();

    mApplication.getModelView().getOverrideColorBuffer().clear();
    mApplication.getModelView().setColorOverride(false);
}

}  // namespace pepr3d