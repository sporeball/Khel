#ifndef KHEL_UTIL_H
#define KHEL_UTIL_H

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

double current_time();
std::string read_file(std::string filename);
// std::string slurp(std::ifstream& in);
std::vector<std::string> split(std::string s, std::string delimiter);

#endif