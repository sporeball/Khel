// #include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <SDL.h>

using namespace std;

// Case insensitive comparator.
struct CaseInsensitive {
  bool operator()(const filesystem::path& lhs, const filesystem::path& rhs) const {
    string lhs_lower = lhs.filename().string();
    string rhs_lower = rhs.filename().string();
    transform(
      lhs_lower.begin(),
      lhs_lower.end(),
      lhs_lower.begin(),
      [](unsigned char c) { return tolower(c); }
    );
    transform(
      rhs_lower.begin(),
      rhs_lower.end(),
      rhs_lower.begin(),
      [](unsigned char c) { return tolower(c); }
    );
    return lhs_lower < rhs_lower;
  }
};

// Convert a Uint64 from SDL's high resolution counter to a time in seconds.
double as_seconds(Uint64 t) {
  return (double) t / (double) SDL_GetPerformanceFrequency();
}

// Get the names of all files in a directory as a `vector<string>`.
vector<string> filenames(string path) {
  vector<string> vec;
  set<filesystem::path, CaseInsensitive> sorted;
  for (const auto& entry : filesystem::directory_iterator(path)) {
    if (entry.is_regular_file()) {
      sorted.insert(entry.path());
    }
  }
  for (const auto& filename : sorted) {
    vec.push_back(filename.stem().string());
  }
  return vec;
}

// Get the names of all folders in a directory as a `vector<string>`.
vector<string> foldernames(string path) {
  vector<string> vec;
  set<filesystem::path, CaseInsensitive> sorted;
  for (const auto& entry : filesystem::directory_iterator(path)) {
    if (entry.is_directory()) {
      sorted.insert(entry.path());
    }
  }
  for (const auto& foldername : sorted) {
    vec.push_back(foldername.stem().string());
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
vector<string> split(string s, string delimiter, int max_times = -1) {
  vector<string> tokens;
  unsigned long pos = 0;
  string token;
  int i = 0;
  while ((pos = s.find(delimiter)) != string::npos) {
    if (i == max_times) break;
    token = s.substr(0, pos);
    tokens.push_back(token);
    s.erase(0, pos + delimiter.length());
    i++;
  }
  tokens.push_back(s);
  return tokens;
}