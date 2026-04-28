#ifndef SAFE_LOGGER_H
#define SAFE_LOGGER_H

#include <cstdio>
#include <utility>

class SafeLogger {
  public:
    template <typename... Args> static void log_error(const char *format, Args &&...args) noexcept {
        std::fprintf(stderr, "[ERROR]: ");
        std::fprintf(stderr, format, std::forward<Args>(args)...);
        std::fprintf(stderr, "\n");
        std::fflush(stderr);
    }
};

#endif