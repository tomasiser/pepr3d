#pragma once

#include "Command.h"
#include "CommandManager.h"

namespace pepr3d {

/// Example state used for debugging purposes of the CommandManager system, contains a single integer
struct IntegerState {
    int mInnerValue = 0;

    int saveState() const {
        return mInnerValue;
    };

    void loadState(int newState) {
        mInnerValue = newState;
    }
};

/// Example command used for debugging purposes of the CommandManager system, adds an integer to IntegerState
class AddValueCommand : public CommandBase<IntegerState> {
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
