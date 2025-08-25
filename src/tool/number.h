#pragma once
#include "../error/error.h"

#include <string>

using std::string;

struct Number {
    long long value;
    bool is_signed;
};

Number number_of_tokens(string token, ErrorReporter &error_reporter);