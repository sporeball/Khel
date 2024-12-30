#ifndef KHEL_INPUT_H
#define KHEL_INPUT_H

#include <map>

extern const std::map<char, int> map_scancodes;

// struct KeyPress {
//   int scancode;
//   double press_time;
// };

void try_hit();

#endif