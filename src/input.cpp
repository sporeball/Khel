#include <map>
#include <vector>
#include <SDL.h>
#include "chart.h"
#include "input.h"
#include "ui.h"
#include "util.h"

using namespace std;

const map<char, int> map_scancodes = {
  {'q', SDL_SCANCODE_Q}, {'a', SDL_SCANCODE_A}, {'z', SDL_SCANCODE_Z},
  {'w', SDL_SCANCODE_W}, {'s', SDL_SCANCODE_S}, {'x', SDL_SCANCODE_X},
  {'e', SDL_SCANCODE_E}, {'d', SDL_SCANCODE_D}, {'c', SDL_SCANCODE_C},
  {'r', SDL_SCANCODE_R}, {'f', SDL_SCANCODE_F}, {'v', SDL_SCANCODE_V},
  {'t', SDL_SCANCODE_T}, {'g', SDL_SCANCODE_G}, {'b', SDL_SCANCODE_B},
  {'y', SDL_SCANCODE_Y}, {'h', SDL_SCANCODE_H}, {'n', SDL_SCANCODE_N},
  {'u', SDL_SCANCODE_U}, {'j', SDL_SCANCODE_J}, {'m', SDL_SCANCODE_M},
  {'i', SDL_SCANCODE_I}, {'k', SDL_SCANCODE_K}, {',', SDL_SCANCODE_COMMA},
  {'o', SDL_SCANCODE_O}, {'l', SDL_SCANCODE_L}, {'.', SDL_SCANCODE_PERIOD},
  {'p', SDL_SCANCODE_P}, {';', SDL_SCANCODE_SEMICOLON}, {'/', SDL_SCANCODE_SLASH},
};

void try_hit(KhelState* state, UiState* ui_state) {
  if (state->chart_wrapper->chart_status != ChartStatus::PLAYING) return;
  double chart_time = state->chart_time();
  const Uint8* keyboard_state = SDL_GetKeyboardState(NULL);
  SyncedStructList* synced_struct_list = state->chart_wrapper->chart->get_difficulty(ui_state->difficulty)->synced_struct_list;
  vector<SyncedStruct*> hits_and_holds;
  vector<SyncedStruct*> hits_and_holds_within_window;
  // determine which hits and holds are within the timing window
  for (auto synced : synced_struct_list->vec) {
    if (synced->t != SyncedStructType::HIT && synced->t != SyncedStructType::HOLD) continue;
    hits_and_holds.push_back(synced);
    double synced_exact_time = synced->beat->to_exact_time(state->chart_wrapper->chart->metadata->bpms);
    double early_limit = synced_exact_time - 0.135;
    double late_limit = synced_exact_time + 0.135;
    if (chart_time >= early_limit && chart_time <= late_limit) {
      hits_and_holds_within_window.push_back(synced);
    } else if (chart_time > late_limit) {
      synced_struct_list->vec.erase(remove(synced_struct_list->vec.begin(), synced_struct_list->vec.end(), synced), synced_struct_list->vec.end());
    }
  }
  // determine which hits and holds are having their keys pressed right now
  vector<SyncedStruct*> matches;
  for (auto synced : hits_and_holds_within_window) {
    if (all_of(synced->keys.begin(), synced->keys.end(), [keyboard_state](char c) {
      return keyboard_state[map_scancodes.at(c)] == 1;
    })) {
      matches.push_back(synced);
    }
  }
  // figure out how accurate each one is
  for (auto match : matches) {
    string s_keys(match->keys.begin(), match->keys.end());
    double match_exact_time = match->beat->to_exact_time(state->chart_wrapper->chart->metadata->bpms);
    double hit_time_ms = (chart_time - match_exact_time) * 1000.0;
    if (hit_time_ms < 0.0) {
      printf("hit %s %f ms early\n", s_keys.c_str(), std::abs(hit_time_ms));
    } else if (hit_time_ms > 0.0) {
      printf("hit %s %f ms late\n", s_keys.c_str(), hit_time_ms);
    } else {
      printf("hit %s exactly on time!\n", s_keys.c_str());
    }
    if (std::abs(hit_time_ms) <= 23.0) { // marvelous
      // state->score += max_score_per_object;
      ui_state->judgement = "marvelous!";
    } else if (std::abs(hit_time_ms) <= 45.0) { // perfect
      // state->score += ceil(max_score_per_object * 0.75);
      ui_state->judgement = "perfect";
    } else if (std::abs(hit_time_ms) <= 90.0) { // great
      // state->score += ceil(max_score_per_object * 0.5);
      ui_state->judgement = "great";
    } else { // good
      // state->score += ceil(max_score_per_object * 0.25);
      ui_state->judgement = "good";
    }
    synced_struct_list->vec.erase(remove(synced_struct_list->vec.begin(), synced_struct_list->vec.end(), match), synced_struct_list->vec.end());
  }
}