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
    using CommandBaseType = CommandBase<Target>;
    using StateType = decltype(std::declval<const Target>().saveState());

    /// How often snapshots of the target should be saved (at minimum)
    static const int SNAPSHOT_FREQUENCY = 10;

    /// Create a command manager that will be operating around a snapshottable target
    explicit CommandManager(Target& target) : mTarget(target) {}

    /// Execute a command, saving it into a history and removing all currently redoable commands.
    /// @param join Try to join this command into the last one
    void execute(std::unique_ptr<CommandBaseType>&& command, bool join = false);

    /// Undo a single command operation
    void undo();

    /// Redo a single command (if possible)
    void redo();

    /// Is there a command that can be undoed? @see getLastCommand()
    bool canUndo() const;

    /// Is there a command that can be redoed? @see getNextCommand()
    bool canRedo() const;

    /// Get last command that was executed (ie the command to be undoed) @see canUndo()
    const CommandBaseType& getLastCommand() const;

    /// Get next command to be redoed if it exists @see canRedo()
    const CommandBaseType& getNextCommand() const;

   private:
    Target& mTarget;
    /// Executed and possibly future commands
    std::vector<std::unique_ptr<CommandBaseType>> mCommandHistory;

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
    auto getPrevSnapshotIterator() const -> decltype(std::declval<const std::vector<SnapshotPair>>().begin());
    
    /// Should we save state before next command
    bool shouldSaveState() const;

    /// Join this command with the last one
    bool joinWithLastCommand(CommandBaseType& command);

    size_t getNumOfCommandsSinceSnapshot() const;

    CommandBaseType& getLastCommand() {
        // allow non const access for command manager
        const auto* constThis = static_cast<const decltype(this)>(this);
        return const_cast<CommandBaseType&>(constThis->getLastCommand());
    }
};

template <typename Target>
auto CommandManager<Target>::getPrevSnapshotIterator() const
    -> decltype(std::declval<const std::vector<SnapshotPair>>().begin()) {
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
void CommandManager<Target>::execute(std::unique_ptr<CommandBaseType>&& command, bool join) {
    clearFutureState();

    // Command is either stored in history or joined to the last one

    if(!join || !joinWithLastCommand(*command)) {
        // Save target's state every few commands
        if(shouldSaveState()) {
            const size_t nextCommandIdx = mCommandHistory.size() - mPosFromEnd;
            mTargetSnapshots.push_back({mTarget.saveState(), nextCommandIdx});
        }

        command->run(mTarget);
        mCommandHistory.emplace_back(std::move(command));
    } else {
        command->run(mTarget);
    }
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
const typename CommandManager<Target>::CommandBaseType& CommandManager<Target>::getLastCommand() const {
    if(!canUndo()) {
        throw std::logic_error("No last command exists");
    }

    return *mCommandHistory[mCommandHistory.size() - mPosFromEnd - 1];
}

template <typename Target>
const typename CommandManager<Target>::CommandBaseType& CommandManager<Target>::getNextCommand() const {
    if(!canRedo()) {
        throw std::logic_error("No next command exists");
    }

    return *mCommandHistory[mCommandHistory.size() - mPosFromEnd];
}

template <typename Target>
void CommandManager<Target>::clearFutureState() {
    if(mPosFromEnd > 0) {
        // Clear all future snapshots
        mTargetSnapshots.erase(std::next(getPrevSnapshotIterator()), mTargetSnapshots.end());
        
        // Clear all future commands
        mCommandHistory.erase(std::prev(mCommandHistory.end(), mPosFromEnd), mCommandHistory.end());

        mPosFromEnd = 0;
    }
}

template <typename Target>
bool CommandManager<Target>::shouldSaveState() const {
    if(mTargetSnapshots.empty()) {
        return true;
    }

    const size_t commandsSinceSnapshot = getNumOfCommandsSinceSnapshot();

    return (canUndo() && getLastCommand().isSlowCommand() && commandsSinceSnapshot != 0) ||
           commandsSinceSnapshot >= SNAPSHOT_FREQUENCY;
}

template <typename Target>
bool CommandManager<Target>::joinWithLastCommand(CommandBaseType& command) {
    if(!canUndo() || getLastCommand().getCommandType() != command.getCommandType() || command.getCommandType() == 0)
        return false;

    if(getLastCommand().joinCommand(command)) {
        // If the command that got modified has a valid snapshot in front of it destroy it
        if(getNumOfCommandsSinceSnapshot() == 0) {
            mTargetSnapshots.erase(std::prev(mTargetSnapshots.end()), mTargetSnapshots.end());
        }
        return true;
    } else {
        return false;
    }
}

template <typename Target>
size_t CommandManager<Target>::getNumOfCommandsSinceSnapshot() const {
    const size_t nextCommandIdx = mCommandHistory.size() - mPosFromEnd;
    return nextCommandIdx - (getPrevSnapshotIterator()->nextCommandIdx - 1);
}

}  // namespace pepr3d
