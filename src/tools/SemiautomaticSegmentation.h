#pragma once

#include <algorithm>

#include "tools/Tool.h"
#include "ui/IconsMaterialDesign.h"
#include "ui/MainApplication.h"

namespace pepr3d {

class SemiautomaticSegmentation : public ITool {
   public:
    SemiautomaticSegmentation(MainApplication& app) : mApplication(app) {}

    virtual std::string getName() const override {
        return "Manual Segmentation";
    }

    virtual std::string getIcon() const override {
        return ICON_MD_WIDGETS;
    }

    virtual void drawToSidePane(SidePane& sidePane) override;
    virtual void drawToModelView(ModelView& modelView) override;
    virtual void onModelViewMouseDown(ModelView& modelView, ci::app::MouseEvent event) override;
    virtual void onModelViewMouseDrag(ModelView& modelView, ci::app::MouseEvent event) override;
    virtual void onModelViewMouseMove(ModelView& modelView, ci::app::MouseEvent event) override;
    virtual void onNewGeometryLoaded(ModelView& modelView) override;
    virtual void onToolDeselect(ModelView& modelView) override;

   private:
    MainApplication& mApplication;

    std::optional<std::size_t> mHoveredTriangleId = {};
    std::unordered_map<std::size_t, std::size_t> mStartingTriangles;
    std::vector<glm::vec4> mBackupColorBuffer = {};
    std::unordered_map<std::size_t, std::vector<std::size_t>> mCurrentColoring = {};

    float mBucketSpread = 0.0f;
    float mBucketSpreadLatest = 0.0f;
    bool mNormalStop = false;

    void reset();
    void setupOverride();
    void setTriangleColor();
    std::unordered_map<std::size_t, std::vector<std::size_t>> collectTrianglesByColor();
    void spreadColors();

    struct NormalStopping {
        const Geometry* geo;
        const double threshold;

        NormalStopping(const Geometry* g, const double thresh) : geo(g), threshold(thresh) {}

        bool operator()(const size_t a, const size_t b) const {
            double cosAngle = 0.0;
            const auto& newNormal1 = geo->getTriangle(a).getNormal();
            const auto& newNormal2 = geo->getTriangle(b).getNormal();
            cosAngle = glm::dot(glm::normalize(newNormal1), glm::normalize(newNormal2));

            if(cosAngle < threshold) {
                return false;
            } else {
                return true;
            }
        }
    };

    struct SDFStopping {
        const Geometry* const geo;
        double maximumDifference;
        std::vector<double> initialValues;

        SDFStopping(const Geometry* const g, const std::vector<double> initialVals, const double maxDiff)
            : geo(g), maximumDifference(maxDiff), initialValues(initialVals) {
            std::sort(initialValues.begin(), initialValues.end());
        }

        bool operator()(const size_t neighbour, const size_t current) const {
            assert(geo->isSdfComputed());
            if(!geo->isSdfComputed()) {
                return false;
            }

            const double sdfNeighbour = geo->getSdfValue(neighbour);
            const auto initialSdfValue = std::lower_bound(initialValues.begin(), initialValues.end(), sdfNeighbour);
            double closest1 = -1;
            double closest2 = -1;
            if(initialSdfValue == initialValues.end()) {
                closest1 = closest2 = initialValues.back();
            } else {
                closest1 = *initialSdfValue;
                if(*initialSdfValue == initialValues.front()) {
                    closest2 = *initialSdfValue;
                } else {
                    closest2 = *(initialSdfValue - 1);
                }
            }
            if((abs(sdfNeighbour - closest1) < maximumDifference) ||
               (abs(sdfNeighbour - closest2) < maximumDifference)) {
                return true;
            } else {
                return false;
            }
        }
    };
};
}  // namespace pepr3d