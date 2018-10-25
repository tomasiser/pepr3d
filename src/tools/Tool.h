#pragma once
#include <string>
#include "cinder/app/MouseEvent.h"

namespace pepr3d {

class SidePane;
class ModelView;

class ITool {
   public:
    ITool() = default;
    virtual ~ITool() = default;

    virtual std::string getName() const = 0;
    virtual std::string getIcon() const = 0;
    virtual void drawToSidePane(SidePane& sidePane){};
    virtual void onModelViewMouseDown(ModelView& modelView, ci::app::MouseEvent event){};
    virtual void onModelViewMouseDrag(ModelView& modelView, ci::app::MouseEvent event){};
    virtual void onModelViewMouseUp(ModelView& modelView, ci::app::MouseEvent event){};
    virtual void onModelViewMouseWheel(ModelView& modelView, ci::app::MouseEvent event){};
};

}  // namespace pepr3d
