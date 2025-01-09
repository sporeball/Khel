#include <algorithm>
#include <map>
#include <set>
#include <string>
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

// Construct a judgement with type `JudgementType::J_NONE`.
Judgement::Judgement() {
  ms = -100000.0;
}
// Construct a judgement with type `JudgementType::J_GOOD` or better.
Judgement::Judgement(double m) : ms(m) {}
// Construct a judgement with type `JudgementType::J_MISS`.
Judgement* Judgement::miss() {
  Judgement* j = new Judgement();
  j->ms = -10000.0;
  return j;
}
// Return the type of this judgement.
JudgementType Judgement::t() {
  if (std::abs(ms) <= 23.0) {
    return JudgementType::J_MARVELOUS;
  } else if (std::abs(ms) <= 45.0) {
    return JudgementType::J_PERFECT;
  } else if (std::abs(ms) <= 90.0) {
    return JudgementType::J_GREAT;
  } else if (std::abs(ms) <= 135.0) {
    return JudgementType::J_GOOD;
  } else if (std::abs(ms) < 100000.0) {
    return JudgementType::J_MISS;
  } else {
    return JudgementType::J_NONE;
  }
}
// Return the amount of score that should be added by this judgement.
double Judgement::score(double max_score_per_object) {
  switch (t()) {
    case JudgementType::J_MARVELOUS:
      return max_score_per_object;
    case JudgementType::J_PERFECT:
      return max_score_per_object - 10.0;
    case JudgementType::J_GREAT:
      return (max_score_per_object * 0.6) - 10.0;
    case JudgementType::J_GOOD:
      return (max_score_per_object * 0.2) - 10.0;
    default:
      return 0.0;
  }
}
// Return the text that should be used to represent this judgemt in the UI.
string Judgement::text() {
  switch (t()) {
    case JudgementType::J_MARVELOUS:
      return "marvelous!";
    case JudgementType::J_PERFECT:
      return "perfect";
    case JudgementType::J_GREAT:
      return "great";
    case JudgementType::J_GOOD:
      return "good";
    case JudgementType::J_MISS:
      return "miss";
    case JudgementType::J_NONE:
      return "none";
  }
}

// Try to register a hit on the currently playing chart.
void try_hit(KhelState* state, UiState* ui_state) {
  if (state->chart_wrapper->chart_status != ChartStatus::PLAYING) return;
  Uint64 now = state->now();
  double chart_time = state->chart_time();
  KeyPressList* keypresses = state->keypresses;
  SyncedStructList* synced_struct_list = state->chart_wrapper->synced_structs;
  vector<SyncedStruct*> hit_objects_within_window;
  // for each synced struct...
  for (SyncedStruct* synced : synced_struct_list->vec) {
    // it must be a hit or the start of a hold
    if (synced->t == SyncedStructType::SS_TIMING_LINE || synced->t == SyncedStructType::SS_HOLD_TICK) continue;
    // it must be within the timing window (+/-135ms)
    double synced_exact_time = synced->beat->to_exact_time(state->chart_wrapper->chart->metadata->bpms);
    double early_limit = synced_exact_time - 0.135;
    double late_limit = synced_exact_time + 0.135;
    if (chart_time < early_limit || chart_time > late_limit) continue;
    // it must have all of its keys pressed right now
    if (!all_of(synced->keys.begin(), synced->keys.end(), [now, keypresses](char c) {
      KeyPress* keypress = keypresses->get(c);
      if (keypress == nullptr) return false;
      double press_duration_seconds = as_seconds(now - keypress->press_time);
      return press_duration_seconds <= 0.135;
    })) {
      continue;
    }
    // no other synced structs in the vector should have the same keys as this one
    if (any_of(hit_objects_within_window.begin(), hit_objects_within_window.end(), [synced](SyncedStruct* s) {
      string s1(s->keys.begin(), s->keys.end());
      string s2(synced->keys.begin(), synced->keys.end());
      return s1 == s2;
    })) {
      continue;
    }
    hit_objects_within_window.push_back(synced);
  }
  // figure out how accurate each one is
  for (auto match : hit_objects_within_window) {
    double match_exact_time = match->beat->to_exact_time(state->chart_wrapper->chart->metadata->bpms);
    double ms = (chart_time - match_exact_time) * 1000.0;
    judge(ms, match, state, ui_state);
    state->remove_synced_struct(match);
  }
}

void try_hold(KhelState* state, UiState* ui_state) {
  if (state->chart_wrapper->chart_status != ChartStatus::PLAYING) return;
  double chart_time = state->chart_time();
  KeyPressList* keypresses = state->keypresses;
  SyncedStructList* synced_struct_list = state->chart_wrapper->synced_structs;
  // for each synced struct...
  for (SyncedStruct* synced : synced_struct_list->vec) {
    // it must be a hold tick
    if (synced->t != SyncedStructType::SS_HOLD_TICK) continue;
    // it must be within the second half of the timing window
    double synced_exact_time = synced->beat->to_exact_time(state->chart_wrapper->chart->metadata->bpms);
    double late_limit = synced_exact_time + 0.135;
    if (chart_time < synced_exact_time || chart_time > late_limit) continue;
    // it must have all of its keys pressed right now
    if (!all_of(synced->keys.begin(), synced->keys.end(), [keypresses](char c) {
      KeyPress* keypress = keypresses->get(c);
      if (keypress == nullptr) return false;
      return true;
    })) {
      continue;
    }
    judge(0.0, synced, state, ui_state);
    state->remove_synced_struct(synced);
  }
}

void judge(double ms, SyncedStruct* synced, KhelState* state, UiState* ui_state) {
  ms += state->offset;
  Judgement* j = new Judgement(ms);
  if (ms < 0.0) {
    printf("judge: %f ms early\n", std::abs(ms));
  } else {
    printf("judge: %f ms late\n", ms);
  }
  synced->judgement = j;
  state->judgements.push_back(j);
  state->score += j->score(state->max_score_per_object);
  ui_state->judgement = j->text();
  // counts
  switch (j->t()) {
    case JudgementType::J_MARVELOUS:
      state->judgement_counts[0] += 1;
      break;
    case JudgementType::J_PERFECT:
      state->judgement_counts[1] += 1;
      break;
    case JudgementType::J_GREAT:
      state->judgement_counts[2] += 1;
      break;
    case JudgementType::J_GOOD:
      state->judgement_counts[3] += 1;
      break;
    case JudgementType::J_MISS:
      state->judgement_counts[4] += 1;
      break;
    default:
      break;
  }
  // combo
  if (j->t() == JudgementType::J_MISS) {
    if (state->combo > 0) {
      state->combo = -1;
      state->lowest_judgement_in_combo = j;
    } else {
      state->combo -= 1;
    }
  } else {
    if (state->combo > 0) {
      state->combo += 1;
    } else {
      state->combo = 1;
    }
    if (state->lowest_judgement_in_combo->t() == JudgementType::J_MISS || state->lowest_judgement_in_combo->t() == JudgementType::J_NONE) {
      state->lowest_judgement_in_combo = j;
    } else if (j->t() > state->lowest_judgement_in_combo->t()) {
      state->lowest_judgement_in_combo = j;
    }
  }
}