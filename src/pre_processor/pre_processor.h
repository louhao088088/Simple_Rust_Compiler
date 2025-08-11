#pragma once

#include <iostream>
#include <string>
#include <vector>

using std::pair;
using std::string;
using std::vector;

// Read Program, delete the empty line and delete comments in the program.

struct Prog {
    string content;
    vector<pair<int, int>> positions; // Store the positions of the comments
};

Prog read_program();

// Print Program in the pre-processor.
void print_program(const string &program);