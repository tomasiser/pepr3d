#pragma once
#include "geometry/Geometry.h"
#include "tools/Tool.h"
#include "ui/IconsMaterialDesign.h"
#include "ui/SidePane.h"

namespace pepr3d {

class PaintBucket : public ITool {
   public:
    PaintBucket(MainApplication& app) : mApplication(app) {}

    virtual std::string getName() const override {
        return "Paint Bucket";
    }

    virtual std::string getIcon() const override {
        return ICON_MD_FORMAT_COLOR_FILL;
    }

    virtual void drawToSidePane(SidePane& sidePane) override;
    virtual void onModelViewMouseDown(ModelView& modelView, ci::app::MouseEvent event) override;
    virtual void onModelViewMouseDrag(ModelView& modelView, ci::app::MouseEvent event) override;

   private:
    MainApplication& mApplication;

    struct DoNotStop {
        const Geometry* geo;

        DoNotStop(const Geometry* g) : geo(g) {}

        bool operator()(const size_t a, const size_t b) const {
            return true;
        }
    };

    struct ColorStopping {
        const Geometry* geo;

        ColorStopping(const Geometry* g) : geo(g) {}

        bool operator()(const size_t a, const size_t b) const {
            if(geo->getTriangle(a).getColor() == geo->getTriangle(b).getColor()) {
                return true;
            } else {
                return false;
            }
        }
    };

    struct NormalStopping {
        const Geometry* geo;
        const double threshold;
        const glm::vec3 startNormal;

        NormalStopping(const Geometry* g, const double thresh, const glm::vec3 normal)
            : geo(g), threshold(thresh), startNormal(normal) {}

        bool operator()(const size_t a, const size_t b) const {
            const auto& newNormal = geo->getTriangle(a).getNormal();

            const double cosAngle = glm::dot(glm::normalize(newNormal), glm::normalize(startNormal));
            if(cosAngle < threshold) {
                return false;
            } else {
                return true;
            }
        }
    };
};
}  // namespace pepr3d
