#ifndef GLOG_PARSER_DRIVER_H
#define GLOG_PARSER_DRIVER_H

#include <string>
#include <vector>
#include <map>

namespace GLOG_PARSER {
class Parser;
};

class ParserDriver {
  friend class GLOG_PARSER::Parser;
 public:
  std::string GLogParserGetErrorMessage() { return glog_parser_error_message_; };

  std::vector<std::vector<std::pair<std::string, std::string>>> data;

 private:
  std::string glog_parser_error_message_;

 protected:
  void GLogParserSetError(std::string msg) {glog_parser_error_message_ = msg;}

};


#endif //GLOG_PARSER_DRIVER_H
