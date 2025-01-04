#ifndef KHEL_UTIL_H
#define KHEL_UTIL_H

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <SDL.h>

struct CaseInsensitive;

double as_seconds(Uint64 t);
std::vector<std::string> filenames(std::string path);
std::vector<std::string> foldernames(std::string path);
std::string read_file(std::string filename);
std::vector<std::string> split(std::string s, std::string delimiter, int max_times = -1);

#endif