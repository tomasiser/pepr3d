#pragma once

#include "peprimgui.h"

namespace pepr3d {

enum class DialogType : std::size_t { Information = 0, Warning, Error, FatalError };

class Dialog {
    DialogType mType;
    std::string mCaption;
    std::string mMessage;
    std::string mAcceptButton;

   public:
    Dialog(DialogType type, const std::string& caption, const std::string& message = "",
           const std::string& acceptButton = "Continue")
        : mType(type), mCaption(caption), mMessage(message), mAcceptButton(acceptButton) {}

    bool draw() const;

    DialogType getType() const {
        return mType;
    }

    std::string getCaption() const {
        return mCaption;
    }

    std::string getMessage() const {
        return mMessage;
    }

    bool operator<(const Dialog& dialog) const {
        return static_cast<std::size_t>(mType) < static_cast<std::size_t>(dialog.mType);
    }

    bool isFatalError() const {
        return mType == DialogType::FatalError;
    }
};

}  // namespace pepr3d
