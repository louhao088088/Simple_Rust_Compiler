// error.cpp
#include "error.h"
void ErrorReporter::report_error(const std::string &message, int line, int column) {
    has_errors_ = true;
    std::cerr << "Error";
    if (line >= 0)
        std::cerr << " at line " << line;
    if (column >= 0)
        std::cerr << ", column " << column;
    std::cerr << ": " << message << std::endl;
}

void ErrorReporter::report_warning(const std::string &message, int line, int column) {
    std::cerr << "Warning";
    if (line >= 0)
        std::cerr << " at line " << line;
    if (column >= 0)
        std::cerr << ", column " << column;
    std::cerr << ": " << message << std::endl;
}