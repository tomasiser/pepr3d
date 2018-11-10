#pragma once
#include <cassert>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <vector>
#include "commands/Command.h"

namespace pepr3d {

/**
CommandManager handles all undoable operations on target in the form of commands See @see
CommandBase. All commands must be executed via the CommandManager Requirements for Target: Target
must have a saveState() and loadState(State) methods
*/
template <typename Target>
class CommandManager {
   public:
    using CommandBase = CommandBase<Target>;
    using StateType = decltype(std::declval<const Target>().saveState());

    /// How often snapshots of the target should be saved (at minimum)
    static const int SNAPSHOT_FREQUENCY = 10;

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
    const CommandBase& getLastCommand() const;

    /// Get next command to be redoed if it exists @see canRedo()
    const CommandBase& getNextCommand() const;

   private:
    Target& mTarget;
    /// Executed and possibly future commands
    std::vector<std::unique_ptr<CommandBase>> mCommandHistory;

    /// Saved states of the target and commandId after them
    struct SnapshotPair {
        StateType state;
        size_t nextCommandIdx;
    };

    std::vector<SnapshotPair> mTargetSnapshots;

    /// Position from end of the stack
    size_t mPosFromEnd = 0;

    void clearFutureState();

    /// Get snapshot before current state
    auto getPrevSnapshotIterator() const;

    /// Should we save state before next command
    bool shouldSaveState() const;

    CommandBase& getLastCommand() {
        // allow non const access for command manager
        const auto* constThis = static_cast<const decltype(this)>(this);
        return const_cast<CommandBase&>(constThis->getLastCommand());
    }
};

template <typename Target>
auto CommandManager<Target>::getPrevSnapshotIterator() const {
    assert(mPosFromEnd <= mCommandHistory.size());

    const size_t nextCommandIdx = mCommandHistory.size() - mPosFromEnd;

    // Find last snapshot that is saved before next command
    auto prevSnapshotIt = std::prev(mTargetSnapshots.end());
    while(prevSnapshotIt->nextCommandIdx > nextCommandIdx) {
        prevSnapshotIt = std::prev(prevSnapshotIt);
    }

    return prevSnapshotIt;
}

template <typename Target>
void CommandManager<Target>::execute(std::unique_ptr<CommandBase>&& command) {
    clearFutureState();

    // Save target's state every few commands
    if(shouldSaveState()) {
        const size_t nextCommandIdx = mCommandHistory.size() - mPosFromEnd;
        mTargetSnapshots.push_back({mTarget.saveState(), nextCommandIdx});
    }

    command->run(mTarget);
    mCommandHistory.emplace_back(std::move(command));
}

template <typename Target>
void CommandManager<Target>::undo() {
    if(!canUndo())
        return;

    mPosFromEnd++;
    auto prevSnapshotIt = getPrevSnapshotIterator();
    mTarget.loadState(prevSnapshotIt->state);

    // Execute all commands between last snapshot and desired state
    for(size_t i = prevSnapshotIt->nextCommandIdx; i < mCommandHistory.size() - mPosFromEnd; i++) {
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
const typename CommandManager<Target>::CommandBase& CommandManager<Target>::getLastCommand() const {
    if(!canUndo()) {
        throw std::logic_error("No last command exists");
    }

    return *mCommandHistory[mCommandHistory.size() - mPosFromEnd - 1];
}

template <typename Target>
const typename CommandManager<Target>::CommandBase& CommandManager<Target>::getNextCommand() const {
    if(!canRedo()) {
        throw std::logic_error("No next command exists");
    }

    return *mCommandHistory[mCommandHistory.size() - mPosFromEnd];
}

template <typename Target>
void CommandManager<Target>::clearFutureState() {
    if(mPosFromEnd > 0) {
        // Clear all future commands
        mCommandHistory.erase(std::prev(mCommandHistory.end(), mPosFromEnd), mCommandHistory.end());

        // Clear all future snapshots
        mTargetSnapshots.erase(std::next(getPrevSnapshotIterator()), mTargetSnapshots.end());
        mPosFromEnd = 0;
    }
}

template <typename Target>
bool CommandManager<Target>::shouldSaveState() const {
    if(mTargetSnapshots.empty()) {
        return true;
    }

    const size_t nextCommandIdx = mCommandHistory.size() - mPosFromEnd;
    const size_t commandsSinceSnapshot = nextCommandIdx - getPrevSnapshotIterator()->nextCommandIdx;

    return (canUndo() && getLastCommand().isSlowCommand() && commandsSinceSnapshot != 0) ||
           commandsSinceSnapshot >= SNAPSHOT_FREQUENCY;
}

}  // namespace pepr3d
