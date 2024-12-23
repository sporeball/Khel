#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

double current_time() {
  auto now = chrono::system_clock::now();
  double t = now.time_since_epoch().count();
  return t;
}

string read_file(string filename) {
  filesystem::path path(filename);
  ifstream f(path, ios::in | ios::binary);
  const auto sz = filesystem::file_size(path);
  string result(sz, '\0');
  f.read(result.data(), sz);
  return result;
}

vector<string> split(string s, string delimiter) {
  vector<string> tokens;
  int pos = 0;
  string token;
  while ((pos = s.find(delimiter)) != string::npos) {
    token = s.substr(0, pos);
    tokens.push_back(token);
    s.erase(0, pos + delimiter.length());
  }
  tokens.push_back(s);
  return tokens;
}