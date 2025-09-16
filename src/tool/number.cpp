#include "number.h"

Number number_of_tokens(string token, ErrorReporter &error_reporter) {
    long long num = -1;
    if (token.length() > 2 && token[0] == '0' && token[1] == 'x') {
        for (size_t i = 2; i < token.length(); i++) {
            if (token[i] == '_')
                continue;

            if (!(token[i] >= '0' && token[i] <= '9') || !(token[i] >= 'a' && token[i] <= 'f') ||
                !(token[i] >= 'A' && token[i] <= 'F')) {
                if ((token[i] == 'i' || token[i] == 'u') &&
                    ((token.length() == i + 2 && token[i + 1] == '3' && token[i + 2] == '2') ||
                     (token.length() == i + 4 && token[i + 1] == 's' && token[i + 2] == 'i' &&
                      token[i + 3] == 'z' && token[i + 4] == 'e'))) {
                    if (token[i] == 'i') {
                        if (num > 2147483647) {
                            error_reporter.report_error("Integer overflow");
                            return {-1, "unknown"};
                        }
                        if (token[i + 1] == 's')
                            return {num, "isize"};
                        else
                            return {num, "i32"};
                    } else if (token[i] == 'u') {
                        if (num > 4294967295) {
                            error_reporter.report_error("Unsigned integer overflow");
                            return {-1, "unknown"};
                        }
                        if (token[i + 1] == 's')
                            return {num, "usize"};
                        else
                            return {num, "u32"};
                    }

                } else {
                    error_reporter.report_error("Invalid number format");
                    return {-1, "unknown"};
                }
            }
            if (num == -1)
                num = 0;
            num = num * 16 +
                  (token[i] >= '0' && token[i] <= '9'
                       ? token[i] - '0'
                       : (token[i] >= 'a' && token[i] <= 'f'
                              ? token[i] - 'a' + 10
                              : (token[i] >= 'A' && token[i] <= 'F' ? token[i] - 'A' + 10 : -1)));
        }
    } else if (token.length() > 2 && token[0] == '0' && token[1] == 'b') {
        for (size_t i = 2; i < token.length(); i++) {
            if (token[i] == '_')
                continue;

            if (token[i] != '0' && token[i] != '1') {
                if ((token[i] == 'i' || token[i] == 'u') &&
                    ((token.length() == i + 2 && token[i + 1] == '3' && token[i + 2] == '2') ||
                     (token.length() == i + 4 && token[i + 1] == 's' && token[i + 2] == 'i' &&
                      token[i + 3] == 'z' && token[i + 4] == 'e'))) {
                    if (token[i] == 'i') {
                        if (num > 2147483647) {
                            error_reporter.report_error("Integer overflow");
                            return {-1, "unknown"};
                        }
                        if (token[i + 1] == 's')
                            return {num, "isize"};
                        else
                            return {num, "i32"};

                    } else if (token[i] == 'u') {
                        if (num > 4294967295) {
                            error_reporter.report_error("Unsigned integer overflow");
                            return {-1, "unknown"};
                        }
                        if (token[i + 1] == 's')
                            return {num, "usize"};
                        else
                            return {num, "u32"};
                    }

                } else {
                    error_reporter.report_error("Invalid number format");
                    return {-1, "unknown"};
                }
            }
            if (num == -1)
                num = 0;
            num = num * 2 + (token[i] == '0' ? 0 : 1);
        }

    } else if (token.length() > 1 && token[0] == '0' && token[1] == 'o') {
        for (size_t i = 2; i < token.length(); i++) {
            if (token[i] == '_')
                continue;

            if (!(token[i] >= '0' && token[i] <= '7')) {
                if ((token[i] == 'i' || token[i] == 'u') &&
                    ((token.length() == i + 2 && token[i + 1] == '3' && token[i + 2] == '2') ||
                     (token.length() == i + 4 && token[i + 1] == 's' && token[i + 2] == 'i' &&
                      token[i + 3] == 'z' && token[i + 4] == 'e'))) {
                    if (token[i] == 'i') {
                        if (num > 2147483647) {
                            error_reporter.report_error("Integer overflow");
                            return {-1, "unknown"};
                        }
                        if (token[i + 1] == 's')
                            return {num, "isize"};
                        else
                            return {num, "i32"};
                    } else if (token[i] == 'u') {
                        if (num > 4294967295) {
                            error_reporter.report_error("Unsigned integer overflow");
                            return {-1, "unknown"};
                        }
                        if (token[i + 1] == 's')
                            return {num, "isize"};
                        else
                            return {num, "i32"};
                    }

                } else {
                    error_reporter.report_error("Invalid number format");
                    return {-1, "unknown"};
                }
            }
            if (num == -1)
                num = 0;
            num = num * 8 + (token[i] - '0');
        }

    } else {
        if (token[0] == '_') {
            error_reporter.report_error("Invalid number format");
            return {-1, "unknown"};
        }
        for (size_t i = 0; i < token.length(); i++) {
            if (token[i] == '_')
                continue;

            if (!(token[i] >= '0' && token[i] <= '9')) {
                if ((token[i] == 'i' || token[i] == 'u') &&
                    ((token.length() == i + 2 && token[i + 1] == '3' && token[i + 2] == '2') ||
                     (token.length() == i + 4 && token[i + 1] == 's' && token[i + 2] == 'i' &&
                      token[i + 3] == 'z' && token[i + 4] == 'e'))) {
                    if (token[i] == 'i') {
                        if (num > 2147483647) {
                            error_reporter.report_error("Integer overflow");
                            return {-1, "unknown"};
                        }
                        if (token[i + 1] == 's')
                            return {num, "isize"};
                        else
                            return {num, "i32"};
                    } else if (token[i] == 'u') {
                        if (num > 4294967295) {
                            error_reporter.report_error("Unsigned integer overflow");
                            return {-1, "unknown"};
                        }
                        if (token[i + 1] == 's')
                            return {num, "usize"};
                        else
                            return {num, "u32"};
                    }

                } else {
                    error_reporter.report_error("Invalid number format");
                    return {-1, "unknown"};
                }
            }
            if (num == -1)
                num = 0;
            num = num * 10 + (token[i] - '0');
        }
    }

    if (num < 0) {
        error_reporter.report_error("Invalid number format");
        return {-1, "unknown"};
    }
    return {num, "anyint"};
}
