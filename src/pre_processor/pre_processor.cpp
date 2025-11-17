#include "pre_processor.h"

Prog read_program() {
    string line;
    Prog program;
    program.content = "";
    int multiline_comment_num = 0;
    bool in_string = 0, in_string2 = 0;
    bool trans = 0;
    int line_num = 0;
    while (std::getline(std::cin, line)) {
        string processed_line = "";
        size_t i = 0;
        line_num++;

        // std::cout << line << " " << std::endl;
        while (i < line.length()) {
            if (in_string) {
                processed_line += line[i];
                if (trans) {
                    trans = 0;
                } else if (line[i] == '\\') {
                    trans = 1;

                } else if (line[i] == '"') {
                    in_string = 0;
                }
                i++;
            } else if (in_string2) {
                processed_line += line[i];
                if (trans) {
                    trans = 0;
                } else if (line[i] == '\\') {
                    trans = 1;

                } else if (line[i] == '\'') {
                    in_string2 = 0;
                }
                i++;
            } else if (!multiline_comment_num) {

                if (line[i] == '"') {
                    in_string = 1;
                    processed_line += line[i];
                    i++;
                } else if (line[i] == '\'') {
                    in_string2 = 1;
                    processed_line += line[i];
                    i++;
                } else if (i < line.length() - 1 && line[i] == '/' && line[i + 1] == '/') {
                    break;
                }

                else if (i < line.length() - 1 && line[i] == '/' && line[i + 1] == '*') {
                    processed_line += " ";
                    multiline_comment_num++;
                    i += 2;
                    continue;
                } else {
                    processed_line += line[i];
                    i++;
                }
            } else {

                if (i < line.length() - 1 && line[i] == '*' && line[i + 1] == '/') {
                    multiline_comment_num--;
                    i += 2;
                    continue;
                } else if (i < line.length() - 1 && line[i] == '/' && line[i + 1] == '*') {
                    multiline_comment_num++;
                    i += 2;
                    continue;
                } else {
                    i++;
                }
            }
        }

        processed_line.erase(0, processed_line.find_first_not_of(" \t\r\n"));
        processed_line.erase(processed_line.find_last_not_of(" \t\r\n") + 1);
        for (int i = 0; i < processed_line.length(); i++) {
            program.positions.push_back({line_num, i});
        }
        program.positions.push_back({line_num, processed_line.length()});
        program.content += processed_line + "\n";
    }

    return program;
}

void print_program(const string &program) {
    std::cerr << "Zeroth Step pre_preocessor:" << std::endl;
    std::cerr << program << std::endl;
    std::cerr << std::endl;
    std::cerr << std::endl;
}
