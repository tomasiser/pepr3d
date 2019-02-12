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

/// A base class of all tools that the user can select and manipulate the Geometry and application with them
class Tool {
   public:
    Tool() = default;
    virtual ~Tool() = default;

    /// Returns a short name, displayed e.g., in a side pane and tooltip
    virtual std::string getName() const = 0;

    /// Returns a description, displayed e.g., in a tooltip
    virtual std::string getDescription() const {
        return "";
    }

    /// Returns an associated Hotkey or an empty value.
    /// Use hotkeys.findHotkey() method
    virtual std::optional<Hotkey> getHotkey(const Hotkeys& hotkeys) const {
        return {};
    }

    /// Returns a UTF-8 icon from an icon font
    virtual std::string getIcon() const = 0;

    /// Returns true if enabled and can be selected, otherwise false.
    /// This should be a fast method, it is called every frame!
    virtual bool isEnabled() const {
        return true;
    };

    /// Called every frame on a currently active Tool.
    /// Draw to SidePane from inside this method
    virtual void drawToSidePane(SidePane& sidePane){};

    /// Called every frame on a currently active Tool.
    /// Draw to ModelView from inside this method
    virtual void drawToModelView(ModelView& modelView){};

    /// Called on a currently active Tool when users presses a mouse button over a ModelView.
    virtual void onModelViewMouseDown(ModelView& modelView, ci::app::MouseEvent event){};

    /// Called on a currently active Tool when users drags a mouse button over a ModelView.
    virtual void onModelViewMouseDrag(ModelView& modelView, ci::app::MouseEvent event){};

    /// Called on a currently active Tool when users releases a mouse button over a ModelView.
    virtual void onModelViewMouseUp(ModelView& modelView, ci::app::MouseEvent event){};

    /// Called on a currently active Tool when users scrolls mouse wheel over a ModelView.
    virtual void onModelViewMouseWheel(ModelView& modelView, ci::app::MouseEvent event){};

    /// Called on a currently active Tool when users moves mouse cursor over a ModelView.
    /// Note: This method is not called when the user is dragging the mouse!
    virtual void onModelViewMouseMove(ModelView& modelView, ci::app::MouseEvent event){};

    /// Called on a Tool right after user selects it.
    /// Note: It may also be called when a user selects the same tool again!
    virtual void onToolSelect(ModelView& modelView){};

    /// Called on a Tool right after user selects another tool.
    /// Note: It may also be called when a user selects the same tool again!
    virtual void onToolDeselect(ModelView& modelView){};

    /// Called on EVERY Tool right after a new Geometry is loaded or imported.
    virtual void onNewGeometryLoaded(ModelView& modelView){};

    /// Returns an optional intersection (an index of a triangle) of a ci::Ray with current Geometry.
    /// This method is safe and if an exception occurs, an error dialog is automatically shown.
    virtual std::optional<std::size_t> safeIntersectMesh(MainApplication& mainApplication, const ci::Ray ray) final;

    /// Returns an optional intersection (DetailedTriangleId) of a ci::Ray with current Geometry.
    /// This method is safe and if an exception occurs, an error dialog is automatically shown.
    virtual std::optional<DetailedTriangleId> safeIntersectDetailedMesh(MainApplication& mainApplication,
                                                                        const ci::Ray ray) final;
    virtual bool safeComputeSdf(MainApplication& mainApplication) final;
};

}  // namespace pepr3d
