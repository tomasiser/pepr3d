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

    explicit CmdAddValue(int addedValue = 1) : mAddedValue(addedValue) {}

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

    explicit CmdAddValueSlow(int addedValue = 1) : CommandBase(true), mAddedValue(addedValue) {}

   protected:
    virtual void run(MockTarget& target) const override {
        target.mInnerValue += mAddedValue;
    }

    int mAddedValue;
};

class CmdAddValueJoinable : public CommandBase<MockTarget> {
   public:
    virtual std::string_view getDescription() const override {
        return "IncreaseVal";
    }

    explicit CmdAddValueJoinable(int addedValue = 1) : CommandBase(false, true), mAddedValue(addedValue) {}

   protected:
    virtual void run(MockTarget& target) const override {
        target.mInnerValue += mAddedValue;
    }

    // Join this command with other of the same type
    virtual bool joinCommand(const CommandBase<MockTarget>& otherBase) override {
        // Dynamic casting a reference may throw exception
        // If you want to avoid exception handling get pointer from the reference

        try {
            const CmdAddValueJoinable& other = dynamic_cast<const CmdAddValueJoinable&>(otherBase);
            mAddedValue += other.mAddedValue;
            return true;
        } catch(std::bad_cast&) {
            return false;
        }
    }

    int mAddedValue;
};

class CmdAddValueJoinableSlow : public CommandBase<MockTarget> {
   public:
    virtual std::string_view getDescription() const override {
        return "IncreaseVal";
    }

    explicit CmdAddValueJoinableSlow(int addedValue = 1) : CommandBase(true, true), mAddedValue(addedValue) {}

   protected:
    virtual void run(MockTarget& target) const override {
        target.mInnerValue += mAddedValue;
    }

    // Join this command with other of the same type
    virtual bool joinCommand(const CommandBase<MockTarget>& otherBase) override {
        // Dynamic casting a reference may throw exception
        // If you want to avoid exception handling get pointer from the reference

        try {
            const CmdAddValueJoinableSlow& other = dynamic_cast<const CmdAddValueJoinableSlow&>(otherBase);
            mAddedValue += other.mAddedValue;
            return true;
        } catch(std::bad_cast&) {
            return false;
        }
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
    /**
     * Test canRedo, canUndo and actually re-doing
     */

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
    /**
     * Test canRedo, canUndo and actually re-doing on slow commands
     */

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
    /**
     * Test the overwrite future funcionality - if you undo several times, and perform a new stroke, you cannot redo
     * anymore.
     */

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

TEST(CommandManager, Joinable) {
    /**
     * Test that joinable commands are combined into one command that is undoable whole
     */

    MockTarget target{};
    CommandManager<MockTarget> cm(target);

    EXPECT_FALSE(cm.canUndo());

    int counter = target.mInnerValue;

    const auto maxSteps = 5 * CommandManager<MockTarget>::SNAPSHOT_FREQUENCY + 1;

    // run a few commands, making sure to do more than one snapshot
    for(int i = 0; i < maxSteps; i++) {
        cm.execute(make_unique<CmdAddValueJoinable>(i), true);
        counter += i;

        EXPECT_EQ(target.mInnerValue, counter);
        EXPECT_TRUE(cm.canUndo());

        cm.undo();
        EXPECT_EQ(target.mInnerValue, 0);

        EXPECT_FALSE(cm.canUndo());
        EXPECT_TRUE(cm.canRedo());

        cm.redo();
        EXPECT_EQ(target.mInnerValue, counter);
    }
}

TEST(CommandManager, JoinRemovesSnapshot) {
    /*
     *Test that joining into command that is snapshoted correctly removes the now invalid snapshot
     */

    MockTarget target{};
    CommandManager<MockTarget> cm(target);

    int counter = 0;
    cm.execute(make_unique<CmdAddValueJoinableSlow>(1));  // this will create snapshot at next command
    counter++;

    cm.execute(make_unique<CmdAddValue>(50000));  // Snapshot created before executing this
    cm.undo();

    // This command will be joined to the first one.
    cm.execute(make_unique<CmdAddValueJoinableSlow>(10), true);

    // Make sure that the old snapshot got deleted by stepping forward and returning back
    cm.execute(make_unique<CmdAddValue>(50000));
    cm.undo();

    // The value restored from the snapshot should be equal to the combined value of both commands
    EXPECT_EQ(target.mInnerValue, 11);
}

}  // namespace pepr3d
#endif
