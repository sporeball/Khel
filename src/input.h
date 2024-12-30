#ifndef KHEL_INPUT_H
#define KHEL_INPUT_H

#include <map>
#include "khel.h"
#include "ui.h"

extern const std::map<char, int> map_scancodes;

// struct KeyPress {
//   int scancode;
//   double press_time;
// };

void try_hit(KhelState* state, UiState* ui_state);

#endif