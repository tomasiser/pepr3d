#pragma once

#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/ProgressHandler.hpp>

namespace pepr3d {

/// Represents a progress of importing/exporting a model via Assimp
template <typename Progress>
class AssimpProgress : public Assimp::ProgressHandler {
    Progress* mProgress;

   public:
    AssimpProgress(Progress* progress) : mProgress(progress) {}

    /// Sets the progress to a certain value, usually -1 means unknown progress / not started yet, 0 means start of a
    /// progress and 1 means finished
    virtual bool Update(float percentage = -1.f) override {
        if(mProgress != nullptr) {
            *mProgress = percentage;
        }
        return true;
    }
};

}  // namespace pepr3d