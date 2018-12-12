#pragma once
#include <optional>
#include <string>
#include "cinder/app/MouseEvent.h"

#include "ui/Hotkeys.h"

namespace pepr3d {

class SidePane;
class ModelView;

class ITool {
   public:
    ITool() = default;
    virtual ~ITool() = default;

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
};

}  // namespace pepr3d
