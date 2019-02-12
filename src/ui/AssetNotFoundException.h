#pragma once

#include <exception>
#include <stdexcept>

namespace pepr3d {

/// Exception thrown when a required asset is not found in the assets/ directory
class AssetNotFoundException : public std::runtime_error {
   public:
    using std::runtime_error::runtime_error;
};

}  // namespace pepr3d