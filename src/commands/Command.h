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
    CommandBase(bool isSlow = false) : isSlow(isSlow){};
    virtual ~CommandBase() = default;

    /// Get description of the command
    virtual std::string_view getDescription() const = 0;

    /// Is command slow and thus snapshot will be created before next command
    bool isSlowCommand() const {
        return isSlow;
    }

   protected:
    /// Run the command, applying the modifications to the target
    virtual void run(Target& target) const = 0;

   private:
    const bool isSlow = false;
};

}  // namespace pepr3d
