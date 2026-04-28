#include "StdinReader.h"

#include <cstdio>
#include <iostream>
#include <thread>

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#define IS_PIPED (!_isatty(_fileno(stdin)))
#else
#include <unistd.h>
#define IS_PIPED (!isatty(fileno(stdin)))
#endif

int StdinReaderWorker::work() {
    const int STDIN_WAIT_TIME = 100;
    std::this_thread::sleep_for(std::chrono::milliseconds(STDIN_WAIT_TIME));

    bool valid = IS_PIPED && (std::cin.rdbuf()->in_avail() > 0 ||
                              std::cin.peek() != std::char_traits<char>::eof());
    if (!valid) {
        return ParseError::PipeUnavailable;
    }
    int parse_ret = parser.parse(); // block until eof
    // try {
    //     parse_ret = parser.parse(); // block until eof
    // } catch (const std::exception &e) {
    //     if (m_stop.load()) {
    //         return ParseError::Interrupt;
    //     }
    //     errors.write([&](auto &errors) { errors.emplace_back(e.what()); });
    // } catch (...) {
    //     if (m_stop.load()) {
    //         return ParseError::Interrupt;
    //     }
    //     errors.write([&](auto &errors) { errors.emplace_back("Unknown Error"); });
    // }
    if (m_stop.load()) {
        return ParseError::Interrupt;
    }
    return parse_ret;
}

int StdinReaderWorker::run() {
    struct StopGuard {
        std::atomic<bool> &m_stop;
        StopGuard(std::atomic<bool> &m_stop) : m_stop(m_stop) { m_stop.store(false); }
        ~StopGuard() { m_stop.store(true); }
    };
    StopGuard stopGuard(m_stop);
    int parse_ret = work();
    return parse_ret;
}

StdinReaderWorker::StdinReaderWorker() { driver.switch_streams(std::cin, std::cerr); }

StdinReaderWorker::~StdinReaderWorker() {
    if (!m_stop.load()) {
        interrupt();
    }
}

void StdinReaderWorker::interrupt() {
    m_stop.store(true);
#ifdef _WIN32
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    if (hStdin != INVALID_HANDLE_VALUE) {
        CloseHandle(hStdin);
    }
#else
    close(0);
#endif
}
