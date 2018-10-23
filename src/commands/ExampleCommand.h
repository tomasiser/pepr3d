#pragma once

#include "Command.h"
#include "CommandManager.h"

namespace pepr3d {

struct IntegerState {
    int mInnerValue = 0;

    int saveState() const {
        return mInnerValue;
    };

    void loadState(int newState) {
        mInnerValue = newState;
    }
};

class AddValueCommand : public ICommandBase<IntegerState> {
   public:
    virtual std::string_view getDescription() const override {
        return "Add integer value to a state";
    }

    AddValueCommand(int addedValue = 1) : mAddedValue(addedValue) {}

   protected:
    virtual void run(IntegerState& target) const override {
        target.mInnerValue += mAddedValue;
    }

    int mAddedValue;
};

}  // namespace pepr3d