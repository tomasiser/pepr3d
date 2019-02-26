#pragma once

#include <atomic>

namespace pepr3d {

/// Atomic values representing percentage progress of geometry import, export, and SDF computation
struct GeometryProgress {
    std::atomic<float> importRenderPercentage{-1.0f};
    std::atomic<float> importComputePercentage{-1.0f};
    std::atomic<float> buffersPercentage{-1.0f};
    std::atomic<float> aabbTreePercentage{-1.0f};
    std::atomic<float> polyhedronPercentage{-1.0f};

    void resetLoad() {
        importRenderPercentage = -1.0f;
        importComputePercentage = -1.0f;
        buffersPercentage = -1.0f;
        aabbTreePercentage = -1.0f;
        polyhedronPercentage = -1.0f;
    }

    std::atomic<float> createScenePercentage{-1.0f};
    std::atomic<float> exportFilePercentage{-1.0f};

    void resetSave() {
        createScenePercentage = -1.0f;
        exportFilePercentage = -1.0f;
    }

    std::atomic<float> sdfPercentage{-1.0f};

    void resetSdf() {
        sdfPercentage = -1.0f;
    }
};

}  // namespace pepr3d