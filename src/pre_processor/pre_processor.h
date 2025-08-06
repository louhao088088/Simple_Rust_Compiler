#pragma once

#include <iostream>
#include <string>
#include <vector>
using std::string;
using std::vector;

// Read Program, delete the empty line and delete comments in the program.
string read_program();

// Print Program in the pre-processor.
void print_program(const string &program);