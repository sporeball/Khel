#ifndef KHEL_UTIL_H
#define KHEL_UTIL_H

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <SDL.h>

double as_seconds(Uint64 t);
std::vector<std::string> crawl(std::string path);
std::string read_file(std::string filename);
std::vector<std::string> split(std::string s, std::string delimiter);

#endif