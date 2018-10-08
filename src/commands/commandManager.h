#pragma once
#include <cassert>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <vector>
#include "commands/command.h"

/**
CommandManager handles all undoable operations on target in the form of commands See @see
ICommandBase. All commands must be executed via the CommandManager Requirements for Target: Target
must have a saveState() and loadState(State) methods
*/
template <typename Target>
class CommandManager {
   public:
    using CommandBase = ICommandBase<Target>;
    using StateType = decltype(std::declval<const Target>().saveState());

    /// How ofthen snapshots of the target should be saved (every N commands)
    static const int SNAPSHOT_FREQUENCY = 5;

    /// Create a command manager that will be operating around a snapshottable target
    CommandManager(Target& target) : mTarget(target) {}

    /// Execute a command, saving it into a history and removing all currently redoable commands.
    void execute(std::unique_ptr<CommandBase>&& command);

    /// Undo a single command operation
    void undo();

    /// Redo a single command (if possible)
    void redo();

    /// Is there a command that can be undoed? @see getLastCommand()
    bool canUndo() const;

    /// Is there a command that can be redoed? @see getNextCommand()
    bool canRedo() const;

    /// Get last command that was executed (ie the command to be undoed) @see canUndo()
    CommandBase& getLastCommand() const;

    /// Get next command to be redoed if it exists @see canRedo()
    CommandBase& getNextCommand() const;

   private:
    Target& mTarget;
    /// Executed and possibly future commands
    std::vector<std::unique_ptr<CommandBase>> mCommandHistory;

    /// Saved states of the target
    std::vector<StateType> mTargetSnapshots;

    /// Position from end of the stack
    size_t mPosFromEnd = 0;

    void clearFutureState();

    /// Get snapshot before current state
    size_t getPrevSnapshotIdx();
};

template <typename Target>
void CommandManager<Target>::execute(std::unique_ptr<CommandBase>&& command) {
    clearFutureState();

    // Save target's state every few commands
    if((mCommandHistory.size() + 1) % SNAPSHOT_FREQUENCY == 1) {
        mTargetSnapshots.emplace_back(mTarget.saveState());
    }

    command->run(mTarget);
    mCommandHistory.emplace_back(std::move(command));
}

template <typename Target>
void CommandManager<Target>::undo() {
    if(!canUndo())
        return;

    const size_t prevSnapshotIdx = getPrevSnapshotIdx();
    mTarget.loadState(mTargetSnapshots[prevSnapshotIdx]);

    mPosFromEnd++;

    // Execute all commands between last snapshot and desired state
    for(size_t i = prevSnapshotIdx * SNAPSHOT_FREQUENCY; i < mCommandHistory.size() - mPosFromEnd; i++) {
        mCommandHistory[i]->run(mTarget);
    }
}

template <typename Target>
void CommandManager<Target>::redo() {
    if(!canRedo())
        return;

    const size_t nextCommandIdx = mCommandHistory.size() - mPosFromEnd;
    mCommandHistory[nextCommandIdx]->run(mTarget);
    mPosFromEnd--;
}

template <typename Target>
bool CommandManager<Target>::canUndo() const {
    return mCommandHistory.size() > mPosFromEnd;
}

template <typename Target>
bool CommandManager<Target>::canRedo() const {
    return mPosFromEnd != 0;
}

template <typename Target>
typename CommandManager<Target>::CommandBase& CommandManager<Target>::getLastCommand() const {
    if(!canUndo()) {
        throw std::logic_error("No last command exists");
    }

    return mCommandHistory[mCommandHistory.size() - mPosFromEnd];
}

template <typename Target>
typename CommandManager<Target>::CommandBase& CommandManager<Target>::getNextCommand() const {
    if(!canRedo()) {
        throw std::logic_error("No next command exists");
    }

    return mCommandHistory[mCommandHistory.size() - mPosFromEnd + 1];
}

template <typename Target>
void CommandManager<Target>::clearFutureState() {
    if(mPosFromEnd > 0) {
        // Clear all future commands
        mCommandHistory.erase(std::prev(mCommandHistory.end(), mPosFromEnd), mCommandHistory.end());

        // Clear all future snapshots
        const size_t maxSnapshots = (mCommandHistory.size() + SNAPSHOT_FREQUENCY - 1) / SNAPSHOT_FREQUENCY;
        mTargetSnapshots.erase(std::next(mTargetSnapshots.begin(), maxSnapshots), mTargetSnapshots.end());

        mPosFromEnd = 0;
    }
}

template <typename Target>
size_t CommandManager<Target>::getPrevSnapshotIdx() {
    assert(canUndo());

    const size_t prevCommandIdx = mCommandHistory.size() - mPosFromEnd - 1;
    return prevCommandIdx / SNAPSHOT_FREQUENCY;
}
