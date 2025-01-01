#ifndef KHEL_H
#define KHEL_H

#include <map>
#include <vector>
#include <SDL.h>
#include "chart.h"
#include "input.h"
#include "object.h"

struct AutoVelocity;
struct Chart;
struct ChartWrapper;
struct Groups;
struct Judgement;
struct KeyPressList;
struct Objects;

struct KhelState {
  SDL_Window* window;
  SDL_Renderer* renderer;
  Objects* objects;
  Groups* groups;
  std::vector<std::string> chart_names;
  std::vector<Chart*> charts;
  ChartWrapper* chart_wrapper;
  AutoVelocity* av;
  KeyPressList* keypresses;
  Uint64 performance_counter_value_at_game_start;
  Uint64 performance_frequency;
  int offset;
  std::vector<Judgement*> judgements;
  int combo;
  Judgement* lowest_judgement_in_combo;
  double score;
  double max_score_per_object;
  KhelState(SDL_Window* w, SDL_Renderer* r);
  Uint64 now();
  double chart_time();
};

#endif