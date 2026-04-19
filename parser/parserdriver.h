#ifndef GLOG_PARSER_DRIVER_H
#define GLOG_PARSER_DRIVER_H

#include <exception>
#include <map>
#include <string>
#include <vector>

namespace GLOG_PARSER {
class Parser;
};

class ParserDriver {
    friend class GLOG_PARSER::Parser;

  public:
    ParserDriver() : parser_error_messages_(), ref_parser_error_messages_(parser_error_messages_) {}
    // critical: errors must live longer than me
    explicit ParserDriver(std::vector<std::string> &errors) : ref_parser_error_messages_(errors) {}

    // I can not leave my scope
    // Do NOT wrap me with stl pointers
    ParserDriver(const ParserDriver &) = delete;
    ParserDriver(ParserDriver &&) = delete;
    ParserDriver &operator=(const ParserDriver &) = delete;
    ParserDriver &operator=(ParserDriver &&) = delete;

    const std::vector<std::string> &GetErrorMessages() const { return ref_parser_error_messages_; };

    std::vector<std::vector<std::pair<std::string, std::string>>> data;

  private:
    std::vector<std::string> parser_error_messages_;
    std::vector<std::string> &ref_parser_error_messages_;
    size_t m_errors = 0;

  protected:
    void SetError(const std::string &msg) {
        ref_parser_error_messages_.push_back(msg);

        static constexpr size_t MAX_ERROR_COUNT = 100;

        if (++m_errors >= MAX_ERROR_COUNT) {
            throw std::runtime_error("Parser halted: reached limit of " +
                                     std::to_string(MAX_ERROR_COUNT) + " errors. " +
                                     "Last error: " + msg);
        }
    }
    bool look_for_eor_ = false;

  public:
    void ClearErrors() {
        ref_parser_error_messages_.clear();
        look_for_eor_ = false;
    }
};

#endif // GLOG_PARSER_DRIVER_H
