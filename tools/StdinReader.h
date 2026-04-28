#ifndef STDIN_READER_H
#define STDIN_READER_H

#include <QThread>

#include "glogparser.h"

#include <atomic>
#include <memory>

class StdinReaderWorker {
  private:
    GLOG_PARSER::DriverSynchronized driver{std::make_unique<GLOG_PARSER::LexerInteractive>()};

  public:
    decltype(driver.data) &data{driver.data};
    decltype(driver.errors) &errors{driver.errors};

  private:
    GLOG_PARSER::Parser parser{&driver};
    int work();
    std::atomic<bool> m_stop = false;

  public:
    // 定义错误码枚举
    enum ParseError : int {
        Success = 0,
        // 管道不可用
        PipeUnavailable = 0x43564,
        // 中断解析
        Interrupt = 0x56456,
        // parser返回的其他非零结果
    };
    int run();

  public:
    StdinReaderWorker();
    ~StdinReaderWorker();
    void interrupt();
};

#endif