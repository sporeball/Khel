// #include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <SDL.h>

using namespace std;

// Convert a Uint64 from SDL's high resolution counter to a time in seconds.
double as_seconds(Uint64 t) {
  return (double) t / (double) SDL_GetPerformanceFrequency();
}

// Get the names of all files in a directory as a `vector<string>`.
vector<string> crawl(string path) {
  vector<string> vec;
  for (const auto& entry : filesystem::directory_iterator(path)) {
    vec.push_back(entry.path().stem().c_str());
  }
  return vec;
}

// Read the contents of a file into a string.
string read_file(string filename) {
  filesystem::path path(filename);
  ifstream f(path, ios::in | ios::binary);
  const auto sz = filesystem::file_size(path);
  string result(sz, '\0');
  f.read(result.data(), sz);
  return result;
}

// Split a string on some delimiter.
vector<string> split(string s, string delimiter) {
  vector<string> tokens;
  unsigned long pos = 0;
  string token;
  while ((pos = s.find(delimiter)) != string::npos) {
    token = s.substr(0, pos);
    tokens.push_back(token);
    s.erase(0, pos + delimiter.length());
  }
  tokens.push_back(s);
  return tokens;
}