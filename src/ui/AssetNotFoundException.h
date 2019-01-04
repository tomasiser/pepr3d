#pragma once

#include <exception>

namespace pepr3d {

class AssetNotFoundException : public std::exception {
   public:
    using std::exception::exception;
};

}  // namespace pepr3d