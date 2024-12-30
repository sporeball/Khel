#ifndef KHEL_H
#define KHEL_H

#include <map>
#include <vector>
#include <SDL.h>
#include "chart.h"
#include "object.h"

struct KhelState {
  SDL_Window* window;
  SDL_Renderer* renderer;
  Objects* objects;
  Groups* groups;
  std::vector<std::string> chart_names;
  std::vector<Chart*> charts;
  ChartWrapper* chart_wrapper;
  AutoVelocity* av;
  Uint64 performance_counter_value_at_game_start;
  Uint64 performance_frequency;
  int offset;
  int score;
  KhelState(SDL_Window* w, SDL_Renderer* r);
  Uint64 now();
};

#endif