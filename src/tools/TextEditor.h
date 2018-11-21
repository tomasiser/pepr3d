#pragma once
#include "tools/Tool.h"
#include "ui/IconsMaterialDesign.h"
#include "ui/SidePane.h"

namespace pepr3d {

class TextEditor : public ITool {
   public:
    virtual std::string getName() const override {
        return "Text Editor";
    }

    virtual std::string getIcon() const override {
        return ICON_MD_TEXT_FIELDS;
    }
    
    virtual bool isEnabled() const override {
        return false;
    }
};
}  // namespace pepr3d
