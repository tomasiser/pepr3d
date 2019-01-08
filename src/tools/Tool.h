#pragma once
#include <optional>
#include <string>

#include "cinder/Ray.h"
#include "cinder/app/MouseEvent.h"

#include "geometry/TrianglePrimitive.h"
#include "ui/Hotkeys.h"

namespace pepr3d {

class MainApplication;
class SidePane;
class ModelView;

class Tool {
   public:
    Tool() = default;
    virtual ~Tool() = default;

    virtual std::string getName() const = 0;
    virtual std::string getDescription() const {
        return "";
    }
    virtual std::optional<Hotkey> getHotkey(const Hotkeys& hotkeys) const {
        return {};
    }
    virtual std::string getIcon() const = 0;
    virtual bool isEnabled() const {
        return true;
    };

    virtual void drawToSidePane(SidePane& sidePane){};
    virtual void drawToModelView(ModelView& modelView){};
    virtual void onModelViewMouseDown(ModelView& modelView, ci::app::MouseEvent event){};
    virtual void onModelViewMouseDrag(ModelView& modelView, ci::app::MouseEvent event){};
    virtual void onModelViewMouseUp(ModelView& modelView, ci::app::MouseEvent event){};
    virtual void onModelViewMouseWheel(ModelView& modelView, ci::app::MouseEvent event){};
    virtual void onModelViewMouseMove(ModelView& modelView, ci::app::MouseEvent event){};
    virtual void onToolSelect(ModelView& modelView){};
    virtual void onToolDeselect(ModelView& modelView){};
    virtual void onNewGeometryLoaded(ModelView& modelView){};

    virtual std::optional<std::size_t> safeIntersectMesh(MainApplication& mainApplication, const ci::Ray ray) final;
    virtual std::optional<DetailedTriangleId> safeIntersectDetailedMesh(MainApplication& mainApplication,
                                                                        const ci::Ray ray) final;
};

}  // namespace pepr3d
