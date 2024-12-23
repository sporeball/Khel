#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <variant>
#include <vector>
#include <SDL.h>
#include <SDL_mixer.h>
#include "chart.h"
#include "object.h"
#include "util.h"

using namespace std;

static const map<char, int> map_columns = {
  {'q', 0}, {'a', 0}, {'z', 0},
  {'w', 1}, {'s', 1}, {'x', 1},
  {'e', 2}, {'d', 2}, {'c', 2},
  {'r', 3}, {'f', 3}, {'v', 3},
  {'t', 4}, {'g', 4}, {'b', 4},
  {'y', 5}, {'h', 5}, {'n', 5},
  {'u', 6}, {'j', 6}, {'m', 6},
  {'i', 7}, {'k', 7}, {',', 7},
  {'o', 8}, {'l', 8}, {'.', 8},
  {'p', 9}, {';', 9}, {'/', 9},
};

static const map<char, int> map_rows = {
  {'q', 1}, {'w', 1}, {'e', 1}, {'r', 1}, {'t', 1}, {'y', 1}, {'u', 1}, {'i', 1}, {'o', 1}, {'p', 1},
  {'a', 2}, {'s', 2}, {'d', 2}, {'f', 2}, {'g', 2}, {'h', 2}, {'j', 2}, {'k', 2}, {'l', 2}, {';', 2},
  {'z', 4}, {'x', 4}, {'c', 4}, {'v', 4}, {'b', 4}, {'n', 4}, {'m', 4}, {',', 4}, {'.', 4}, {'/', 4},
};

double AutoVelocity::at_exact_time(double exact_time, BpmList* bpms) {
  Bpm* bpm = bpms->at_exact_time(exact_time);
  Bpm* max = bpms->max();
  // printf("return_value: %f\n", return_value);
  return value * (bpm->value / max->value);
}
double AutoVelocity::over_time(double time, BpmList* bpms) {
  double time_elapsed = 0.0;
  double time_remaining = time;
  double cumulative = 0.0;
  for (int i = 0; i < bpms->vec.size(); i++) {
    double av = at_exact_time(time_elapsed, bpms);
    if (i == bpms->vec.size() - 1) {
      cumulative += av * time_remaining;
      break;
    } else {
      Bpm* next_bpm = bpms->vec[i + 1];
      double length = bpms->vec[i]->length(next_bpm);
      if (time_remaining < length) {
        cumulative += av * time_remaining;
        break;
      } else {
        cumulative += av * length;
        time_elapsed += length;
        time_remaining -= length;
      }
    }
  }
  return cumulative;
}

// Convert this Beat to an exact time in seconds.
double Beat::to_exact_time(BpmList* bpms) {
  vector<Bpm*> vec = bpms->vec;
  double beats_remaining = value;
  double exact_time = 0.0;
  double one_minute = 60.0;
  // for (Bpm* ptr : vec) {
  // for (auto it = vec.begin(); it != vec.end(); ++it) {
  for (int i = 0; i < vec.size(); i++) {
    // printf("value: %f\n", vec[i]->value);
    double one_beat = one_minute / vec[i]->value;
    // printf("one_beat: %f\n", one_beat);
    if (i == vec.size() - 1) {
      exact_time += one_beat * beats_remaining;
      break;
    } else {
      Bpm* next_bpm = vec[i + 1];
      double length = vec[i]->length(next_bpm);
      if (beats_remaining < length / one_beat) {
        exact_time += one_beat * beats_remaining;
        break;
      } else {
        exact_time += length;
        beats_remaining -= length / one_beat;
      }
    }
  }
  return exact_time;
}
void Beat::print() {
  printf("Beat { ");
  printf("value: %f", value);
  printf(" }");
}

// Create a Bpm from a String.
// When possible, prefer creating a BpmList instead.
Bpm::Bpm(string s) {
  // printf("creating Bpm from string %s...\n", s.c_str());
  vector<string> tokens = split(s, "@");
  // if (tokens.size() > 2) {}
  string token_value = tokens[0];
  double d_value;
  stringstream ss(token_value);
  ss >> d_value;
  value = d_value;
  string token_start_beat = tokens[1];
  // printf("token_start_beat = %s\n", token_start_beat.c_str());
  double d_start_beat;
  stringstream ss2(token_start_beat);
  ss2 >> d_start_beat;
  Beat* ptr_start_beat = new Beat;
  ptr_start_beat->value = d_start_beat;
  start_beat = ptr_start_beat;
}
// Return the length of this Bpm in seconds.
double Bpm::length(Bpm* next_bpm) {
  if (next_bpm == NULL) {
    return numeric_limits<double>::max();
  }
  double one_minute = 60.0;
  double beats = next_bpm->start_beat->value - start_beat->value;
  double one_beat = one_minute / value;
  return beats * one_beat;
}
void Bpm::print() {
  printf("Bpm { ");
  printf("value: %f", value);
  printf(", start_beat: ");
  start_beat->print();
  printf(" }");
}

// Create a BpmList from a string.
BpmList::BpmList(string s) {
  vector<string> tokens = split(s, ",");
  for (string token : tokens) {
    Bpm* bpm = new Bpm(token);
    vec.push_back(bpm);
  }
  // printf("created BpmList from string %s\n", s.c_str());
}
// Destructor method.
BpmList::~BpmList() {
  for (Bpm* ptr : vec) {
    delete ptr;
  }
}
// Return the Bpm from this BpmList that should be used at a given exact time.
Bpm* BpmList::at_exact_time(double exact_time) {
  if (vec.size() == 1) {
    return vec[0];
  }
  if (exact_time <= 0.0) {
    return vec[0];
  }
  int i = 0;
  double time = exact_time;
  while (1) {
    Bpm* next_bpm = (i + 1 < vec.size()) ? vec[i + 1] : NULL;
    double length = vec[i]->length(next_bpm);
    if (time >= length) {
      time -= length;
      i += 1;
    } else {
      return vec[i];
    }
  }
}
// Return the maximum Bpm from this BpmList.
Bpm* BpmList::max() {
  vector<double> dvec;
  for (Bpm* ptr : vec) {
    dvec.push_back(ptr->value);
  }
  int index = distance(dvec.begin(), max_element(dvec.begin(), dvec.end()));
  return vec[index];
}
void BpmList::print() {
  printf("BpmList { ");
  printf("vec: [ ");
  for (Bpm* ptr : vec) {
    ptr->print();
    printf(", ");
  }
  printf("] ");
  printf("} ");
}

void Metadata::print() {
  printf("Metadata { ");
  printf("version: %d", version);
  printf(", title: %s", title.c_str());
  printf(", subtitle: %s", subtitle.c_str());
  printf(", artist: %s", artist.c_str());
  printf(", credit: %s", credit.c_str());
  printf(", bpms: ");
  bpms->print();
  printf(" }");
}

SyncedStruct::~SyncedStruct() {
  keys.erase(keys.begin(), keys.end());
}
int SyncedStruct::lane() {
  if (t == SyncedStructType::TIMING_LINE) return -1;
  map<char, int>::const_iterator pos = map_columns.find(keys[0]);
  if (pos == map_columns.end()) {
    return -1;
  }
  return pos->second;
}
int SyncedStruct::lane_x() {
  int lane = this->lane();
  return ((40 * lane) - 180) + 400;
}
string SyncedStruct::color() {
  if (t == SyncedStructType::TIMING_LINE) {
    double e = 1.0 / 2147483648.0;
    double i;
    double f = modf(beat->value, &i);
    if (f == 0.0) return "red";
    else if (f == 0.5) return "blue";
    else if (f == 0.25 || f == 0.75) return "yellow";
    else if (f == 0.125 || f == 0.375 || f == 0.625 || f == 0.875) return "green";
    else if (abs(f - 0.333333) < e || abs(f - 0.666666) < e) return "magenta";
    else if (abs(f - 0.166666) < e || abs(f - 0.833333) < e) return "cyan";
    else return "white";
  } else {
    int rows = 0;
    for (char c : keys) {
      map<char, int>::const_iterator pos = map_rows.find(c);
      rows += pos->second;
    }
    if (rows == 1) return "red";
    else if (rows == 2) return "green";
    else if (rows == 3) return "yellow";
    else if (rows == 4) return "blue";
    else if (rows == 5) return "magenta";
    else if (rows == 6) return "cyan";
    else return "white";
  }
}
string SyncedStruct::asset() {
  string color = this->color();
  if (t == SyncedStructType::HIT || t == SyncedStructType::HOLD) {
    return "assets/circle_" + color + ".png";
  } else if (t == SyncedStructType::HOLD_TICK) {
    return "assets/hold_tick_" + color + ".png";
  } else { // timing line
    return "assets/line_" + color + ".png";
  }
}
void SyncedStruct::print() {
  printf("SyncedStruct { ");
  printf("beat: ");
  beat->print();
  printf(", t: ");
  switch (t) {
    case SyncedStructType::HIT:
      printf("HIT");
      break;
    case SyncedStructType::HOLD:
      printf("HOLD");
      break;
    case SyncedStructType::HOLD_TICK:
      printf("HOLD_TICK");
      break;
    case SyncedStructType::TIMING_LINE:
      printf("TIMING_LINE");
      break;
  }
  printf(", keys: ");
  for (char c : keys) {
    printf("%c", c);
  }
  printf(" }");
}

SyncedStructList::SyncedStructList(string s) {
  // printf("creating SyncedStructList from string %s...\n", s.c_str());
  // split on comma, yielding all the hit objects in the chart grouped by beat
  vector<string> groupings = split(s, ",");
  for (string grouping : groupings) {
    // printf("parsing grouping %s...\n", grouping.c_str());
    vector<string> hit_objects_and_beat = split(grouping, "@");
    // if hit_objects_and_beat.size() > 2 {}
    string s_hit_objects = hit_objects_and_beat[0];
    string s_beat = hit_objects_and_beat[1];
    // printf("converting s_beat %s to f_beat...\n", s_beat.c_str());
    double d_beat;
    stringstream ss(s_beat);
    ss >> d_beat;
    Beat* ptr_beat = new Beat;
    ptr_beat->value = d_beat;
    // create a single timing line synced to the whole grouping
    SyncedStruct* timing_line = new SyncedStruct;
    timing_line->beat = ptr_beat;
    timing_line->t = SyncedStructType::TIMING_LINE;
    vec.push_back(timing_line);
    // split the hit objects on plus sign, separating the hits from the holds and their info
    vector<string> hits_and_holds = split(s_hit_objects, "+");
    // if hits_and_holds.size() > 2 {}
    string s_hits = hits_and_holds[0];
    string s_holds_and_info = hits_and_holds[1];
    // split the hits on dash, yielding a vector<string>
    vector<string> hits = split(s_hits, "-");
    for (auto hit : hits) {
      // TODO: safety
      if (empty(hit)) {
        continue;
      }
      // if hit chars are all unique {}
      // if hit chars columns are all equal {}
      vector<char> keys(hit.begin(), hit.end());
      SyncedStruct* ptr_hit_object = new SyncedStruct;
      ptr_hit_object->beat = ptr_beat;
      ptr_hit_object->t = SyncedStructType::HIT;
      ptr_hit_object->keys = keys;
      vec.push_back(ptr_hit_object);
    }
    // split the holds on colon, yielding the holds and their required information
    vector<string> holds_and_info = split(s_holds_and_info, ":");
    // if holds_and_info.size() > 2 {}
    string s_holds = holds_and_info[0];
    string s_info = holds_and_info[1];
    // split the holds on dash, yielding a vector<string>
    vector<string> holds = split(s_holds, "-");
    // split the hold information on equals, separating the length from the tick count
    vector<string> length_and_tick_count = split(s_info, "=");
    // if length_and_tick_count.size() > 2 {}
    string s_length = length_and_tick_count[0];
    string s_tick_count = length_and_tick_count[1];
    double d_length;
    stringstream ss2(s_length);
    ss2 >> d_length;
    int i_tick_count;
    stringstream ss3(s_tick_count);
    ss3 >> i_tick_count;
    for (auto hold : holds) {
      if (empty(hold)) {
        continue;
      }
      // if hit chars are all unique {}
      // if hit chars columns are all equal {}
      vector<char> keys(hold.begin(), hold.end());
      SyncedStruct* ptr_hit_object = new SyncedStruct;
      ptr_hit_object->beat = ptr_beat;
      ptr_hit_object->t = SyncedStructType::HOLD;
      ptr_hit_object->keys = keys;
      vec.push_back(ptr_hit_object);
    }
    // hold ticks
    for (auto hold : holds) {
      int i = 1;
      double delta = d_length / (double) i_tick_count;
      double tick_beat_value = ptr_beat->value + delta;
      while (i < i_tick_count) {
        Beat* ptr_beat = new Beat;
        ptr_beat->value = tick_beat_value;
        vector<char> keys(hold.begin(), hold.end());
        SyncedStruct* ptr_hit_object = new SyncedStruct;
        ptr_hit_object->beat = ptr_beat;
        ptr_hit_object->t = SyncedStructType::HOLD_TICK;
        ptr_hit_object->keys = keys;
        vec.push_back(ptr_hit_object);
        i += 1;
        tick_beat_value += delta;
      }
    }
  }
  // reverse the whole vector, meaning that SyncedStructs which are timed later appear first
  // if we do not do this, destroying one pair of SyncedStructs in the pure calculation group
  // will quickly destroy them all
  reverse(vec.begin(), vec.end());
  // printf("created SyncedStructList from string %s\n", s.c_str());
}
SyncedStructList::~SyncedStructList() {
  for (SyncedStruct* ptr : vec) {
    delete ptr;
  }
}
void SyncedStructList::print() {
  printf("SyncedStructList { ");
  for (SyncedStruct* ptr : vec) {
    ptr->print();
    printf(", ");
  }
  printf("}");
}

Chart::Chart(string filename) {
  unordered_map<string, string> map;
  string contents = read_file(filename);
  vector<string> lines = split(contents, "\n");
  for (auto line : lines) {
    vector<string> key_and_value = deserialize_kv(line);
    string key = key_and_value[0];
    string value = key_and_value[1];
    pair<string, string> p(key, value);
    map.insert(p);
  }
  string s_version = map["version"];
  string s_title = map["title"];
  string s_subtitle = map["subtitle"];
  string s_artist = map["artist"];
  string s_credit = map["credit"];
  string s_hit_objects = map["hit_objects"];
  string s_bpms = map["bpms"];
  metadata = new Metadata;
  metadata->version = 0;
  metadata->title = s_title;
  metadata->subtitle = s_subtitle;
  metadata->artist = s_artist;
  metadata->credit = s_credit;
  metadata->bpms = new BpmList(s_bpms);
  synced_structs = new SyncedStructList(s_hit_objects);
}
Chart::~Chart() {
  Mix_FreeChunk(audio);
  synced_structs->~SyncedStructList();
}
void Chart::set_ratemod(double ratemod) {
  // TODO
}
// void Chart::play() {
// }
void Chart::print() {
  printf("Chart { ");
  printf("metadata: ");
  metadata->print();
  printf(", audio: [private]");
  printf(", synced_structs: ");
  synced_structs->print();
  printf(" }");
}

ChartWrapper::ChartWrapper() {
  chart_status = ChartStatus::NONE;
}
ChartWrapper::~ChartWrapper() {
  chart->~Chart();
}
void ChartWrapper::load_chart(string filename) {
  chart = new Chart(filename);
}
void ChartWrapper::play_chart(SDL_Renderer* renderer, Objects* objects, Groups* groups) {
  string title = chart->metadata->title;
  string subtitle = chart->metadata->subtitle;
  string artist = chart->metadata->artist;
  string credit = chart->metadata->credit;
  if (empty(subtitle)) {
    printf("playing chart \"%s - %s\" (mapped by %s)...\n", artist.c_str(), title.c_str(), credit.c_str());
  } else {
    printf("playing chart \"%s - %s (%s)\" (mapped by %s)...\n", artist.c_str(), title.c_str(), subtitle.c_str(), credit.c_str());
  }
  for (SyncedStruct* synced : chart->synced_structs->vec) {
    if (synced->t == SyncedStructType::TIMING_LINE) {
      int id = objects->create_instance(synced->asset(), 0.0, 1000.0, 100, 1, renderer);
      groups->insert_into_group("pure_calculation", id);
      groups->insert_into_group("timing_lines", id);
    } else {
      int id = objects->create_instance(synced->asset(), synced->lane_x(), 1000.0, 32, 32, renderer);
      groups->insert_into_group("pure_calculation", id);
      groups->insert_into_group("hit_objects", id);
      if (synced->t != SyncedStructType::HOLD_TICK) {
        groups->insert_into_group("hits_and_holds", id);
      }
    }
  }
  // start_time = SDL_GetPerformanceCounter();
  chart_status = ChartStatus::PLAYING;
}

vector<string> deserialize_kv(string raw) {
  // if (!raw.ends_with("")) {}
  vector<string> vec;
  vector<string> key_and_value = split(raw, "=");
  string key = key_and_value[0];
  string value = key_and_value[1];
  value.pop_back();
  vec.push_back(key);
  vec.push_back(value);
  return vec;
}