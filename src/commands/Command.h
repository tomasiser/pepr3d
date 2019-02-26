#pragma once
#include <string>

namespace pepr3d {

/// CommandBase is a base class for commands representing a single undoable operation.
/// The command itself should be an immutable entity, containing only the necessary data to perform the
/// action.
template <typename Target>
class CommandBase {
    template <typename>
    friend class CommandManager;  // allow only CM to run the commands
   public:
    /**
     * @param isSlow Mark command as slow. CommandManager will create a snapshot after a slow command.
     * attempt will be made.
     * @param canBeJoined can this command be joined together with the command of the same type
     */
    CommandBase(bool isSlow = false, bool canBeJoined = false) : mIsSlow(isSlow), mCanBeJoined(canBeJoined) {}
    virtual ~CommandBase() = default;

    /// Get description of the command
    virtual std::string_view getDescription() const = 0;

    /// Is command slow and thus snapshot will be created before next command
    bool isSlowCommand() const {
        return mIsSlow;
    }

    /// Can this command be joined with other commands of the same type
    bool canBeJoined() const {
        return mCanBeJoined;
    }

   protected:
    /// Run the command, applying the modifications to the target
    virtual void run(Target& target) const = 0;

    /**
     * Join this command with other command of the same commandType, uniting both commands into this command's contents.
     * @return true on successful join
     */
    virtual bool joinCommand(const CommandBase&) {
        return false;
    }

   private:
    const bool mIsSlow;
    const bool mCanBeJoined;
};

}  // namespace pepr3d
