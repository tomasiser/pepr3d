#include "commands/CommandManager.h"
#ifdef _TEST_
#include <gtest/gtest.h>
#include <vector>

namespace pepr3d {

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

class CmdAddValue : public CommandBase<MockTarget> {
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

class CmdAddValueSlow : public CommandBase<MockTarget> {
   public:
    virtual std::string_view getDescription() const override {
        return "IncreaseVal";
    }

    CmdAddValueSlow(int addedValue = 1) : CommandBase(true), mAddedValue(addedValue) {}

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
        --currentValueIt;
        EXPECT_EQ(target.mInnerValue, *currentValueIt);
    }

    // Redo all the operations
    while(currentValueIt != valueHistory.end() - 1) {
        ASSERT_TRUE(cm.canRedo());
        cm.redo();
        ++currentValueIt;
        EXPECT_EQ(target.mInnerValue, *currentValueIt);
    }

    EXPECT_FALSE(cm.canRedo());
}

TEST(CommandManager, SlowCommandsRedo) {
    MockTarget target;
    CommandManager<MockTarget> cm(target);

    EXPECT_FALSE(cm.canRedo());

    std::vector<int> valueHistory = {target.mInnerValue};
    const auto maxSteps = 10 * CommandManager<MockTarget>::SNAPSHOT_FREQUENCY + 1;

    // run a few commands, making sure to do more than one snapshot
    for(int i = 0; i < maxSteps; i++) {
        if(i % 7 == 0) {
            cm.execute(make_unique<CmdAddValueSlow>(i));
        } else {
            cm.execute(make_unique<CmdAddValue>(i));
        }

        valueHistory.push_back(target.mInnerValue);
    }

    EXPECT_FALSE(cm.canRedo());
    ASSERT_TRUE(cm.canUndo());

    // Undo half the operations
    std::vector<int>::iterator currentValueIt = valueHistory.end() - 1;
    for(int i = 0; i < maxSteps / 2; i++) {
        cm.undo();
        --currentValueIt;
        EXPECT_EQ(target.mInnerValue, *currentValueIt);
    }

    // Redo all the operations
    while(currentValueIt != valueHistory.end() - 1) {
        ASSERT_TRUE(cm.canRedo());
        cm.redo();
        ++currentValueIt;
        EXPECT_EQ(target.mInnerValue, *currentValueIt);
    }

    EXPECT_FALSE(cm.canRedo());
}

TEST(CommandManager, SlowCommandsFutureClear) {
    MockTarget target;
    CommandManager<MockTarget> cm(target);

    EXPECT_FALSE(cm.canRedo());

    std::vector<int> valueHistory = {target.mInnerValue};
    const auto maxSteps = 10 * CommandManager<MockTarget>::SNAPSHOT_FREQUENCY + 1;

    // run a few commands, making sure to do more than one snapshot
    for(int i = 0; i < maxSteps; i++) {
        if(i % 7 == 0) {
            cm.execute(make_unique<CmdAddValueSlow>(i));
        } else {
            cm.execute(make_unique<CmdAddValue>(i));
        }

        valueHistory.push_back(target.mInnerValue);
    }

    EXPECT_FALSE(cm.canRedo());
    ASSERT_TRUE(cm.canUndo());

    // Undo half the operations
    std::vector<int>::iterator currentValueIt = valueHistory.end() - 1;
    for(int i = 0; i < maxSteps / 2; i++) {
        cm.undo();
        --currentValueIt;
        EXPECT_EQ(target.mInnerValue, *currentValueIt);
    }

    EXPECT_TRUE(cm.canRedo());

    // Execute additional command to clear the future
    cm.execute(make_unique<CmdAddValue>(1));
    EXPECT_FALSE(cm.canRedo());
    EXPECT_TRUE(cm.canUndo());

    cm.undo();
    EXPECT_TRUE(cm.canRedo());

    // Undo all remaining ops
    for(int i = 0; i < maxSteps - (maxSteps / 2); i++) {
        ASSERT_TRUE(cm.canUndo());
        cm.undo();
        --currentValueIt;
        EXPECT_EQ(target.mInnerValue, *currentValueIt);
    }

    EXPECT_FALSE(cm.canUndo());
}

}  // namespace pepr3d
#endif
