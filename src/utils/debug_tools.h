//
//  debug_tools.h
//
//  Created by Antonin on 18/09/2022.
//

#ifndef debug_tools_h
#define debug_tools_h

// Ignore the diagnostic security as we can pass
// variadic arguments to build_log (and to fprintf as well)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"

#include <cassert>
#include <stdio.h>
#include <string>
#include <time.h>

/// @brief Build and prints a log statement.
/// The prefix argument is optional.
template <typename... Args>
void build_log(FILE* stream, const char* prefix, Args... message)
{
    // Get timestamp
    time_t ltime;
    ltime = time(NULL);
    char* time = asctime(localtime(&ltime));
    assert(time != nullptr);
    // Remove the newline character
    if (time != nullptr)
        time[strlen(time) - 1] = '\0';
    // Print the date and the prefix
    if (prefix == nullptr)
        fprintf(stream, "[%s] ", time);
    else
        fprintf(stream, "[%s] %s: ", time, prefix);
    // Print the full message
    fprintf(stream, message...);
    fprintf(stream, "\n");
}

#pragma GCC diagnostic pop

#ifdef DEBUG
// DEBUG

namespace utils
{
/// @brief Debug log statement
#define Log(...) build_log(stdout, nullptr, __VA_ARGS__)
/// @brief Warning log statement
#define LogW(...) build_log(stderr, (char*)"Warning", __VA_ARGS__)
/// @brief Warn the developer, at runtime, that the function has not been implemented
#define WARN_RT_UNIMPLEMENTED                                                                       \
    build_log(stderr, (char*)"Error", (char*)"Line %d of %s: not implemented", __LINE__, __FILE__); \
    assert(0)
/// @brief Warn the developer, at compile time, that the function has not been implemented
#define WARN_CT_UNIMPLEMENTED static_assert(0)
/// @brief Warn the developer of a possible bug
#define WARN assert(0)
} // namespace utils

#else
// PROFILE & RELEASE

namespace utils
{
/// @brief Debug log statement
#define Log(...)
/// @brief Warning log statement
#define LogW(...)
/// @brief Warn the developer, at runtime, that the function has not been implemented yet
#define WARN_RT_UNIMPLEMENTED
/// @brief Warn the developer, at compile time, that the function has not been implemented yet
#define WARN_CT_UNIMPLEMENTED
/// @brief Warn the developer of a possible bug
#define WARN
} // namespace utils

#endif

/// @brief Error log statement
#define LogE(...) build_log(stderr, (char*)"Error", __VA_ARGS__)

#endif /* debug_tools_h */
