#include "peprassert.h"

#include <stdexcept>
#include <string>

namespace pepr3d::debug {
void peprAssert(const char* msg, const char* file, unsigned line) {
    std::string errorStr(msg);
    errorStr += " File:";
    errorStr += file;
    errorStr += " Line:";
    errorStr += std::to_string(line);

#if !defined(NDEBUG) && defined _MSC_VER
    __debugbreak();  // Breakpoint in MSVC
#endif

    throw std::logic_error(errorStr);
}
}  // namespace pepr3d::debug