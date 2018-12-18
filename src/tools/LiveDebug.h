#pragma once
#include <memory>
#include "tools/Tool.h"
#include "ui/IconsMaterialDesign.h"
#include "ui/SidePane.h"

#include "commands/CommandManager.h"
#include "commands/ExampleCommand.h"

namespace pepr3d {

class MainApplication;

class LiveDebug : public Tool {
   public:
    LiveDebug(MainApplication& app) : mApplication(app), mIntegerManager(mIntegerState) {}

    virtual std::string getName() const override {
        return "Live Debug";
    }

    virtual std::optional<Hotkey> getHotkey(const Hotkeys& hotkeys) const override {
        return hotkeys.findHotkey(HotkeyAction::SelectLiveDebug);
    }

    virtual std::string getIcon() const override {
        return ICON_MD_DEVELOPER_BOARD;
    }

    virtual void drawToSidePane(SidePane& sidePane) override;

    virtual void onModelViewMouseMove(ModelView& modelView, ci::app::MouseEvent event) override;

   private:
    MainApplication& mApplication;
    IntegerState mIntegerState;
    CommandManager<IntegerState> mIntegerManager;
    glm::ivec2 mMousePos;
};

}  // namespace pepr3d
