#pragma once

/**
 *  Custom assert definition that throws an exception on assertion.
 *
 *  The original assert header sometimes straight up terminated program without any information on what happened.
 */

#undef P_ASSERT

#if defined(NDEBUG) && !defined(_TEST_)

#define P_ASSERT(expression) ((void)0)

#else

namespace pepr::debug {
/// Custom assert for Pepr3d
void peprAssert(const char* msg, const char* file, unsigned line);
}  // namespace pepr::debug

#define P_ASSERT(expression) \
    (void)((!!(expression)) || (pepr::debug::peprAssert((#expression), (__FILE__), (unsigned)(__LINE__)), 0))

#endif