#pragma once
#include <string>

namespace pepr3d {

class SidePane;

class ITool {
   public:
    ITool() = default;
    virtual ~ITool() = default;

    virtual std::string getName() const = 0;
    virtual std::string getIcon() const = 0;
    virtual void drawToSidePane(SidePane& sidePane){};
};

}  // namespace pepr3d
