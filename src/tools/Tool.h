#pragma once
#include <string>

class ITool {
   public:
    ITool() = default;
    virtual ~ITool() = default;

    virtual std::string_view getName() const = 0;
    virtual std::string_view getIcon() const = 0;
    virtual void drawToSidePane(){};
};
