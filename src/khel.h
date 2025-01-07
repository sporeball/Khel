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
struct SyncedStruct;

struct KhelState {
  SDL_Window* window;
  SDL_Renderer* renderer;
  Objects* objects;
  Groups* groups;
  std::vector<Chart*> charts;
  ChartWrapper* chart_wrapper;
  AutoVelocity* av;
  KeyPressList* keypresses;
  Uint64 performance_counter_value_at_game_start;
  Uint64 performance_frequency;
  int offset;
  int visual_offset;
  std::vector<Judgement*> judgements;
  int marvelous_count;
  int perfect_count;
  int great_count;
  int good_count;
  int miss_count;
  int combo;
  Judgement* lowest_judgement_in_combo;
  double score;
  double max_score_per_object;
  KhelState(SDL_Window* w, SDL_Renderer* r);
  Uint64 now();
  double chart_time();
  void remove_synced_struct(SyncedStruct* synced);
};

#endif