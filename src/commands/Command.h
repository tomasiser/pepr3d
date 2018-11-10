#pragma once
#include <string>

namespace pepr3d {

/**
CommandBase is a base class for commands representing a single undoable operation.
The command itself should be an immutable entity, containing only the necessary data to perform the
action.
*/
template <typename Target>
class CommandBase {
    template <typename>
    friend class CommandManager;  // allow only CM to run the commands
   public:
    /**
     * @param isSlow Mark command as slow. CommandManager will create a snapshot after a slow command.
     * @param commandType Command type is used to determine if two commands can be joined. If commandType is zero no
     * attempt will be made.
     */
    CommandBase(bool isSlow = false, uint64_t commandType = 0) : mIsSlow(isSlow), mCommandType(commandType) {}
    virtual ~CommandBase() = default;

    /// Get description of the command
    virtual std::string_view getDescription() const = 0;

    /// Is command slow and thus snapshot will be created before next command
    bool isSlowCommand() const {
        return mIsSlow;
    }

    /// Get value representing the command's type. Commands of the same type other than zero are assumed to be joinable
    uint64_t getCommandType() const {
        return mCommandType;
    }

   protected:
    /// Run the command, applying the modifications to the target
    virtual void run(Target& target) const = 0;

    /**
     * Join this command with other command of the same commandType, uniting both commands into this command's contents.
     * @return true on successful join
     */
    virtual bool joinCommand(const CommandBase& other) {
        return false;
    }

   private:
    const bool mIsSlow = false;
    const uint64_t mCommandType;
};

}  // namespace pepr3d
