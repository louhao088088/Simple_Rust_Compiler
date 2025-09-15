#pragma once
#include "../error/error.h"

#include <string>

using std::string;

struct Number {
    long long value;
    string Type; // "i32", "u32", "isize", "usize", "anyint"
};

Number number_of_tokens(string token, ErrorReporter &error_reporter);