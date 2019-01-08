#include "tools//Tool.h"
#include "ui/MainApplication.h"

namespace pepr3d {

std::optional<std::size_t> Tool::safeIntersectMesh(MainApplication& mainApplication, const ci::Ray ray) {
    auto geometry = mainApplication.getCurrentGeometry();
    std::optional<std::size_t> hoveredTriangleId;

    try {
        hoveredTriangleId = geometry->intersectMesh(ray);
    } catch(...) {
        CI_LOG_E("Mesh intersection failed.");
        hoveredTriangleId = {};

        const std::string errorCaption = "Fatal Error: Pepr project file (.p3d) corrupted";
        const std::string errorDescription =
            "The project file you attempted to open is corrupted and was loaded incorrectly. "
            "The geometry data inside does not correspond to a valid geometry object and cannot be displayed or "
            "modified. "
            "Pepr3D will now exit. Try loading an earlier backup version, which might not be corrupted yet.";
        mainApplication.pushDialog(Dialog(DialogType::FatalError, errorCaption, errorDescription, "Exit Pepr3D"));
    }
    return hoveredTriangleId;
}

    std::optional<DetailedTriangleId> Tool::safeIntersectDetailedMesh(MainApplication& mainApplication, const ci::Ray ray) {
    auto* geometry = mainApplication.getCurrentGeometry();
        std::optional<DetailedTriangleId> hoveredTriangleId;

    try {
        hoveredTriangleId = geometry->intersectDetailedMesh(ray);
    } catch(...) {
        CI_LOG_E("Mesh intersection failed.");
        hoveredTriangleId = {};

        const std::string errorCaption = "Fatal Error: Pepr project file (.p3d) corrupted";
        const std::string errorDescription =
            "The project file you attempted to open is corrupted and was loaded incorrectly. "
            "The geometry data inside does not correspond to a valid geometry object and cannot be displayed or "
            "modified. "
            "Pepr3D will now exit. Try loading an earlier backup version, which might not be corrupted yet.";
        mainApplication.pushDialog(Dialog(DialogType::FatalError, errorCaption, errorDescription, "Exit Pepr3D"));
    }
    return hoveredTriangleId;
}

}  // namespace pepr3d