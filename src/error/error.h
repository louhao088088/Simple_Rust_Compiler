#pragma once
#include <iostream>
#include <string>

class ErrorReporter {
  public:
    void report_error(const std::string &message, int line = -1, int column = -1);
    void report_warning(const std::string &message, int line = -1, int column = -1);
    bool has_errors() const { return has_errors_; }

  private:
    bool has_errors_ = false;
};