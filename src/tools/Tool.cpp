#include "tools/Tool.h"
#include "geometry/SdfValuesException.h"
#include "ui/MainApplication.h"

namespace pepr3d {

void pushErrorDialogCorruptedFile(MainApplication& mainApplication) {
    const std::string errorCaption = "Fatal Error: Pepr project file (.p3d) corrupted";
    const std::string errorDescription =
        "The project file you attempted to open is corrupted and was loaded incorrectly. "
        "The geometry data inside does not correspond to a valid geometry object and cannot be displayed or "
        "modified. "
        "Pepr3D will now exit. Try loading an earlier backup version, which might not be corrupted yet.";
    mainApplication.pushDialog(Dialog(DialogType::FatalError, errorCaption, errorDescription, "Exit Pepr3D"));
}

std::optional<std::size_t> Tool::safeIntersectMesh(MainApplication& mainApplication, const ci::Ray ray) {
    auto geometry = mainApplication.getCurrentGeometry();
    std::optional<std::size_t> hoveredTriangleId;

    try {
        hoveredTriangleId = geometry->intersectMesh(ray);
    } catch(...) {
        CI_LOG_E("Mesh intersection failed.");
        hoveredTriangleId = {};
        pushErrorDialogCorruptedFile(mainApplication);
    }
    return hoveredTriangleId;
}

std::optional<DetailedTriangleId> Tool::safeIntersectDetailedMesh(MainApplication& mainApplication, const ci::Ray ray) {
    auto* geometry = mainApplication.getCurrentGeometry();
    std::optional<DetailedTriangleId> hoveredTriangleId;

    try {
        hoveredTriangleId = geometry->intersectDetailedMesh(ray);
    } catch(...) {
        CI_LOG_E("Mesh detailed intersection failed.");
        hoveredTriangleId = {};
        pushErrorDialogCorruptedFile(mainApplication);
    }
    return hoveredTriangleId;
}

bool Tool::safeComputeSdf(MainApplication& mainApplication) {
    try {
        mainApplication.getCurrentGeometry()->computeSdfValues();
    } catch(SdfValuesException& e) {
        const std::string errorCaption = "Error: Failed to compute SDF";
        const std::string errorDescription =
            "The SDF values returned by the computation were not valid. This can happen when you use the "
            "segmentation on a flat surface. The segmentation tools will now get disabled for this model. "
            "Remember that the segmentation works based on the thickness of the object and thus a flat surface "
            "cannot be segmented.\n\nThe full description of the problem is:\n";
        mainApplication.dispatchAsync([&mainApplication, errorCaption, errorDescription, e]() {
            mainApplication.pushDialog(Dialog(DialogType::Error, errorCaption, errorDescription + e.what(), "OK"));
        });
        return false;
    } catch(std::exception& e) {
        const std::string errorCaption = "Error: Failed to compute SDF";
        const std::string errorDescription =
            "An internal error occured while computing the SDF values. If the problem persists, try re-loading "
            "the mesh.\n\n"
            "Please report this bug to the developers. The full description of the problem is:\n";
        mainApplication.dispatchAsync([&mainApplication, errorCaption, errorDescription, e]() {
            mainApplication.pushDialog(Dialog(DialogType::Error, errorCaption, errorDescription + e.what(), "OK"));
        });
        return false;
    }
    return true;
}

}  // namespace pepr3d