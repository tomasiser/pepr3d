#pragma once
#include <string>

/*
ICommandBase is a base class for commands representing a single undoable operation.
The command itself should be an immutable entity, containing only the necessary data to perform the
action.
*/
template <typename Target>
class ICommandBase {
    template <typename>
    friend class CommandManager;  // allow only CM to run the commands
   public:
    ICommandBase() = default;
    virtual ~ICommandBase() = default;

    /// Get description of the command
    virtual std::string_view getDescription() const = 0;

   protected:
    /// Run the command, applying the modifications to the target
    virtual void run(Target& target) const = 0;
};
