#include <iostream>

#if defined(_WIN32) || defined(_WIN64)
#include <process.h>
#else
#include <unistd.h>
#endif

class SystemUtils {
  public:
    static void quick_exit_program(int code = 0) {
#if defined(_WIN32) || defined(_WIN64)
        _exit(code);
#elif defined(__linux__) || defined(__APPLE__)
        _exit(code);
#else
        exit(code);
#endif
    }
};