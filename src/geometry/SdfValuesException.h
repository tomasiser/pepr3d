#pragma once

#include <exception>
#include <stdexcept>

namespace pepr3d {

class SdfValuesException : public std::runtime_error {
   public:
    using std::runtime_error::runtime_error;
};

}  // namespace pepr3d