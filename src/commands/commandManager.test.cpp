#include "commands/commandManager.h"
#ifdef _TEST_
#include <gtest/gtest.h>
#include <vector>

using std::make_unique;
struct MockTarget {
    int mInnerValue = 0;

    int saveState() const {
        return mInnerValue;
    };

    void loadState(int newState) {
        mInnerValue = newState;
    }
};

class CmdAddValue : public ICommandBase<MockTarget> {
   public:
    virtual std::string_view getDescription() const override {
        return "IncreaseVal";
    }

    CmdAddValue(int addedValue = 1) : mAddedValue(addedValue) {}

   protected:
    virtual void run(MockTarget& target) const override {
        target.mInnerValue += mAddedValue;
    }

    int mAddedValue;
};

TEST(CommandManager, Undo) {
    /**
     * Test that undo is available and undoes the correct command
     */

    MockTarget target;
    CommandManager<MockTarget> cm(target);

    EXPECT_FALSE(cm.canUndo());

    int counter = target.mInnerValue;
    std::vector<int> counterHistory = {counter};
    const auto maxSteps = 5 * CommandManager<MockTarget>::SNAPSHOT_FREQUENCY + 1;

    // run a few commands, making sure to do more than one snapshot
    for(int i = 0; i < maxSteps; i++) {
        cm.execute(make_unique<CmdAddValue>(i));
        counter += i;
        counterHistory.push_back(counter);

        EXPECT_EQ(target.mInnerValue, counter);
        EXPECT_TRUE(cm.canUndo());
    }

    // undo all commands
    for(int i = 0; i < maxSteps; i++) {
        ASSERT_TRUE(cm.canUndo());
        cm.undo();
        counterHistory.pop_back();

        EXPECT_EQ(target.mInnerValue, counterHistory.back());
    }

    EXPECT_FALSE(cm.canUndo());
}

TEST(CommandManager, Redo) {
    MockTarget target;
    CommandManager<MockTarget> cm(target);

    EXPECT_FALSE(cm.canRedo());

    std::vector<int> valueHistory = {target.mInnerValue};
    const auto maxSteps = 5 * CommandManager<MockTarget>::SNAPSHOT_FREQUENCY + 1;

    // run a few commands, making sure to do more than one snapshot
    for(int i = 0; i < maxSteps; i++) {
        cm.execute(make_unique<CmdAddValue>(i));
        valueHistory.push_back(target.mInnerValue);
    }

    EXPECT_FALSE(cm.canRedo());
    ASSERT_TRUE(cm.canUndo());

    // Undo half the operations
    std::vector<int>::iterator currentValueIt = valueHistory.end() - 1;
    for(int i = 0; i < maxSteps / 2; i++) {
        cm.undo();
        currentValueIt--;
        EXPECT_EQ(target.mInnerValue, *currentValueIt);
    }

    // Redo all the operations
    while(currentValueIt != valueHistory.end() - 1) {
        ASSERT_TRUE(cm.canRedo());
        cm.redo();
        currentValueIt++;
        EXPECT_EQ(target.mInnerValue, *currentValueIt);
    }

    EXPECT_FALSE(cm.canRedo());
}

#endif
