#pragma once

#include "peprimgui.h"

namespace pepr3d {

enum class DialogType : std::size_t { Information = 0, Warning, Error, FatalError };

/// Represents a modal dialog shown to a user in the user interface in front of everything else
class Dialog {
    DialogType mType;
    std::string mCaption;
    std::string mMessage;
    std::string mAcceptButton;

   public:
    Dialog(DialogType type, const std::string& caption, const std::string& message = "",
           const std::string& acceptButton = "Continue")
        : mType(type), mCaption(caption), mMessage(message), mAcceptButton(acceptButton) {}

    /// Draws the dialog and returns true if the confirmation button was pressed.
    bool draw() const;

    /// Returns DialogType of this dialog.
    DialogType getType() const {
        return mType;
    }

    /// Returns the caption (title) of this dialog.
    std::string getCaption() const {
        return mCaption;
    }

    /// Returns the message (description) of this dialog.
    std::string getMessage() const {
        return mMessage;
    }

    /// Compares another Dialog to this one based on their DialogType.
    bool operator<(const Dialog& dialog) const {
        return static_cast<std::size_t>(mType) < static_cast<std::size_t>(dialog.mType);
    }

    /// Returns true if this is a DialogType::FatalError dialog.
    bool isFatalError() const {
        return mType == DialogType::FatalError;
    }
};

}  // namespace pepr3d
