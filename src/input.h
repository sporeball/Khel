#ifndef KHEL_INPUT_H
#define KHEL_INPUT_H

#include <map>
#include <SDL.h>
#include "khel.h"
#include "ui.h"

extern const std::map<int, char> map_keys;

struct KhelState;
struct UiState;

struct KeyPress {
  char key;
  Uint64 press_time;
};

struct KeyPressList {
  std::vector<KeyPress*> vec;
  void add(char c, Uint64 t);
  void remove(char c);
  KeyPress* get(char c);
  int contains(char c);
};

enum Judgement {
  J_MARVELOUS,
  J_PERFECT,
  J_GREAT,
  J_GOOD,
  J_MISS,
  J_NONE,
};

void try_hit(KhelState* state, UiState* ui_state);

#endif