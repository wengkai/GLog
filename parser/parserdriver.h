#ifndef GLOG_PARSER_DRIVER_H
#define GLOG_PARSER_DRIVER_H

#include <map>
#include <string>
#include <vector>

namespace GLOG_PARSER {
class Parser;
};

class ParserDriver {
    friend class GLOG_PARSER::Parser;

  public:
    auto GetErrorMessages() { return glog_parser_error_messages_; };

    std::vector<std::vector<std::pair<std::string, std::string>>> data;

    void ClearErrors() {
        glog_parser_error_messages_.clear();
        look_for_eor_ = false;
    }

  private:
    std::vector<std::string> glog_parser_error_messages_;

  protected:
    void SetError(std::string msg) { glog_parser_error_messages_.push_back(msg); }
    bool look_for_eor_ = false;
};

#endif // GLOG_PARSER_DRIVER_H
