#include <map>
#include <vector>
#include <SDL.h>
#include "chart.h"
#include "input.h"
#include "ui.h"
#include "util.h"

using namespace std;

const std::map<int, char> map_keys = {
  {SDL_SCANCODE_Q, 'q'}, {SDL_SCANCODE_A, 'a'}, {SDL_SCANCODE_Z, 'z'},
  {SDL_SCANCODE_W, 'w'}, {SDL_SCANCODE_S, 's'}, {SDL_SCANCODE_X, 'x'},
  {SDL_SCANCODE_E, 'e'}, {SDL_SCANCODE_D, 'd'}, {SDL_SCANCODE_C, 'c'},
  {SDL_SCANCODE_R, 'r'}, {SDL_SCANCODE_F, 'f'}, {SDL_SCANCODE_V, 'v'},
  {SDL_SCANCODE_T, 't'}, {SDL_SCANCODE_G, 'g'}, {SDL_SCANCODE_B, 'b'},
  {SDL_SCANCODE_Y, 'y'}, {SDL_SCANCODE_H, 'h'}, {SDL_SCANCODE_N, 'n'},
  {SDL_SCANCODE_U, 'u'}, {SDL_SCANCODE_J, 'j'}, {SDL_SCANCODE_M, 'm'},
  {SDL_SCANCODE_I, 'i'}, {SDL_SCANCODE_K, 'k'}, {SDL_SCANCODE_COMMA, ','},
  {SDL_SCANCODE_O, 'o'}, {SDL_SCANCODE_L, 'l'}, {SDL_SCANCODE_PERIOD, '.'},
  {SDL_SCANCODE_P, 'p'}, {SDL_SCANCODE_SEMICOLON, ';'}, {SDL_SCANCODE_SLASH, '/'},
};

// Add a key press to the key press list.
void KeyPressList::add(char c, Uint64 t) {
  KeyPress* key_press = new KeyPress;
  key_press->key = c;
  key_press->press_time = t;
  vec.push_back(key_press);
}
// Remove a key press from the key press list.
void KeyPressList::remove(char c) {
  auto it = remove_if(
    vec.begin(),
    vec.end(),
    [c](KeyPress* key_press) {
      return key_press->key == c;
    }
  );
  vec.erase(it, vec.end());
}
// Get a pointer to the given key press in the key press list.
KeyPress* KeyPressList::get(char c) {
  for (auto keypress : vec) {
    if (keypress->key == c) return keypress;
  }
  return nullptr;
}
// Return whether the key press list contains the given key.
int KeyPressList::contains(char c) {
  for (auto keypress : vec) {
    if (keypress->key == c) return 1;
  }
  return 0;
}

// Try to register a hit on the currently playing chart.
void try_hit(KhelState* state, UiState* ui_state) {
  if (state->chart_wrapper->chart_status != ChartStatus::PLAYING) return;
  Uint64 now = state->now();
  double chart_time = state->chart_time();
  KeyPressList* keypresses = state->keypresses;
  SyncedStructList* synced_struct_list = state->chart_wrapper->synced_structs;
  vector<SyncedStruct*> hit_objects_within_window;
  // determine which hit objects are within the timing window
  for (auto synced : synced_struct_list->vec) {
    switch (synced->t) {
      case SyncedStructType::SS_TIMING_LINE:
        continue;
      default:
        double synced_exact_time = synced->beat->to_exact_time(state->chart_wrapper->chart->metadata->bpms);
        double early_limit = synced_exact_time - 0.135;
        double late_limit = synced_exact_time + 0.135;
        if (chart_time >= early_limit && chart_time <= late_limit) {
          hit_objects_within_window.push_back(synced);
        } else if (chart_time > late_limit + 1.0) {
          // remove from objects and groups (stops rendering)
          state->objects->destroy_instance(synced->id);
          state->groups->remove_from_all_groups(synced->id);
          // remove from chart wrapper
          state->chart_wrapper->synced_structs->vec.erase(
            remove(
              state->chart_wrapper->synced_structs->vec.begin(),
              state->chart_wrapper->synced_structs->vec.end(),
              synced
            ),
            state->chart_wrapper->synced_structs->vec.end()
          );
        } else if (chart_time > late_limit && synced->judgement == Judgement::J_NONE) {
          string s(synced->keys.begin(), synced->keys.end());
          printf("missed %s\n", s.c_str());
          synced->judgement = Judgement::J_MISS;
          ui_state->judgement = "miss";
        }
        break;
    }
  }
  // determine which hits and holds are having their keys pressed right now
  vector<SyncedStruct*> matches;
  for (auto synced : hit_objects_within_window) {
    double synced_exact_time = synced->beat->to_exact_time(state->chart_wrapper->chart->metadata->bpms);
    SyncedStructType t = synced->t;
    if (all_of(synced->keys.begin(), synced->keys.end(), [now, keypresses, chart_time, synced_exact_time, t](char c) {
      KeyPress* keypress = keypresses->get(c);
      if (keypress == nullptr) return false;
      if (t == SyncedStructType::SS_HOLD_TICK) {
        return chart_time > synced_exact_time;
      } else {
        double press_duration_seconds = as_seconds(now - keypress->press_time);
        return press_duration_seconds <= 0.135;
      }
    })) {
      matches.push_back(synced);
    }
  }
  // figure out how accurate each one is
  for (auto match : matches) {
    string s_keys(match->keys.begin(), match->keys.end());
    double match_exact_time = match->beat->to_exact_time(state->chart_wrapper->chart->metadata->bpms);
    SyncedStructType t = match->t;
    if (t == SyncedStructType::SS_HOLD_TICK) {
      printf("held %s\n", s_keys.c_str());
      state->score += state->max_score_per_object;
      match->judgement = Judgement::J_MARVELOUS;
      ui_state->judgement = "marvelous!";
    } else {
      double hit_time_ms = (chart_time - match_exact_time) * 1000.0;
      if (hit_time_ms < 0.0) {
        printf("hit %s %f ms early\n", s_keys.c_str(), std::abs(hit_time_ms));
      } else if (hit_time_ms > 0.0) {
        printf("hit %s %f ms late\n", s_keys.c_str(), hit_time_ms);
      } else {
        printf("hit %s exactly on time!\n", s_keys.c_str());
      }
      // return a judgement
      if (std::abs(hit_time_ms) <= 23.0) { // marvelous
        state->score += state->max_score_per_object;
        match->judgement = Judgement::J_MARVELOUS;
        ui_state->judgement = "marvelous!";
      } else if (std::abs(hit_time_ms) <= 45.0) { // perfect
        state->score += ceil(state->max_score_per_object * 0.75);
        match->judgement = Judgement::J_PERFECT;
        ui_state->judgement = "perfect";
      } else if (std::abs(hit_time_ms) <= 90.0) { // great
        state->score += ceil(state->max_score_per_object * 0.5);
        match->judgement = Judgement::J_GREAT;
        ui_state->judgement = "great";
      } else { // good
        state->score += ceil(state->max_score_per_object * 0.25);
        match->judgement = Judgement::J_GOOD;
        ui_state->judgement = "good";
      }
    }
    // remove from objects and groups (stops rendering)
    state->objects->destroy_instance(match->id);
    state->groups->remove_from_all_groups(match->id);
    // remove from chart wrapper
    state->chart_wrapper->synced_structs->vec.erase(
      remove(
        state->chart_wrapper->synced_structs->vec.begin(),
        state->chart_wrapper->synced_structs->vec.end(),
        match
      ),
      state->chart_wrapper->synced_structs->vec.end()
    );
  }
}