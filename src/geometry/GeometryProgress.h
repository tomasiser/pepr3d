#pragma once

#include <atomic>

namespace pepr3d {

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
};

}  // namespace pepr3d