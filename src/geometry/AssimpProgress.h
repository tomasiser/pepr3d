#pragma once

#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/ProgressHandler.hpp>

namespace pepr3d {

template <typename Progress>
class AssimpProgress : public Assimp::ProgressHandler {
    Progress* mProgress;

   public:
    AssimpProgress(Progress* progress) : mProgress(progress) {}

    virtual bool Update(float percentage = -1.f) override {
        if(mProgress != nullptr) {
            *mProgress = percentage;
        }
        return true;
    }
};

}  // namespace pepr3d