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
struct Sounds;
struct SyncedStruct;
struct UiState;

struct KhelState {
  SDL_Window* window;
  SDL_Renderer* renderer;
  Objects* objects;
  Groups* groups;
  Sounds* sounds;
  std::vector<Chart*> charts;
  ChartWrapper* chart_wrapper;
  AutoVelocity* av;
  KeyPressList* keypresses;
  Uint64 performance_counter_value_at_game_start;
  Uint64 performance_frequency;
  int offset;
  int visual_offset;
  int countdown_ticks;
  std::vector<Judgement*> judgements;
  std::vector<int> judgement_counts;
  int combo;
  Judgement* lowest_judgement_in_combo;
  double score;
  double max_score_per_object;
  KhelState(SDL_Window* w, SDL_Renderer* r);
  Uint64 now();
  double chart_time();
  void remove_synced_struct(SyncedStruct* synced);
  void reset();
};

void poll_event(SDL_Event* e, int* quit, KhelState* state, UiState* ui_state);
void try_count(KhelState* state);
void try_play_audio(KhelState* state);
void try_update_synced_structs(KhelState* state, UiState* ui_state);
void try_end_chart(KhelState* state);
void update_object_instances(KhelState* state);
void draw(KhelState* state, UiState* ui_state);

#endif