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
#include "input.h"
#include "object.h"
#include "sound.h"
#include "util.h"

using namespace std;

const map<char, int> map_columns = {
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

const map<char, int> map_rows = {
  {'q', 1}, {'w', 1}, {'e', 1}, {'r', 1}, {'t', 1}, {'y', 1}, {'u', 1}, {'i', 1}, {'o', 1}, {'p', 1},
  {'a', 2}, {'s', 2}, {'d', 2}, {'f', 2}, {'g', 2}, {'h', 2}, {'j', 2}, {'k', 2}, {'l', 2}, {';', 2},
  {'z', 4}, {'x', 4}, {'c', 4}, {'v', 4}, {'b', 4}, {'n', 4}, {'m', 4}, {',', 4}, {'.', 4}, {'/', 4},
};

// Return the auto velocity value that should be used at a given exact time.
// Scales around `AutoVelocity::value`.
double AutoVelocity::at_exact_time(double exact_time, BpmList* bpms) {
  Bpm* bpm = bpms->at_exact_time(exact_time);
  Bpm* max = bpms->max();
  // printf("return_value: %f\n", return_value);
  return (double) value * (bpm->value / max->value);
}
// Return the cumulative auto velocity over some amount of time in seconds, measured from exact time zero.
double AutoVelocity::over_time(double time, BpmList* bpms) {
  double time_elapsed = 0.0;
  double time_remaining = time;
  double cumulative = 0.0;
  for (unsigned long i = 0; i < bpms->vec.size(); i++) {
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
  for (unsigned long i = 0; i < vec.size(); i++) {
    double one_beat = one_minute / vec[i]->value;
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
  unsigned long i = 0;
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
// Return the minimum Bpm from this BpmList.
Bpm* BpmList::min() {
  vector<double> dvec;
  for (Bpm* ptr : vec) {
    dvec.push_back(ptr->value);
  }
  int index = distance(dvec.begin(), min_element(dvec.begin(), dvec.end()));
  return vec[index];
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
  unsigned long size = vec.size();
  printf("BpmList { ");
  printf("vec: [%lu items] ", size);
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

// Destructor method.
SyncedStruct::~SyncedStruct() {
  keys.erase(keys.begin(), keys.end());
}
// Return a pointer to a clone of this SyncedStruct.
SyncedStruct* SyncedStruct::clone() const {
  return new SyncedStruct(*this);
}
// Return the column that this SyncedStruct is in.
// Returns -1 if this SyncedStruct is a timing line.
int SyncedStruct::lane() {
  if (t == SyncedStructType::SS_TIMING_LINE) return -1;
  map<char, int>::const_iterator pos = map_columns.find(keys[0]);
  if (pos == map_columns.end()) {
    return -1;
  }
  return pos->second;
}
// Return the x coordinate of the column that this SyncedStruct is in.
int SyncedStruct::lane_x() {
  int lane = this->lane();
  return ((40 * lane) - 180) + 400;
}
// Return the color of this SyncedStruct.
string SyncedStruct::color() {
  if (t == SyncedStructType::SS_TIMING_LINE) {
    // double e = 1.0 / 2147483648.0;
    double e = 1.0 / 65536.0;
    double i;
    double f = modf(beat->value, &i);
    if (f == 0.0) return "red";
    else if (f == 0.5) return "blue";
    else if (f == 0.25 || f == 0.75) return "yellow";
    else if (f == 0.125 || f == 0.375 || f == 0.625 || f == 0.875) return "green";
    else if (std::abs(f - 0.333333) < e || std::abs(f - 0.666666) < e) return "magenta";
    else if (std::abs(f - 0.166666) < e || std::abs(f - 0.833333) < e) return "cyan";
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
// Return the asset that should be used to draw this SyncedStruct.
string SyncedStruct::asset() {
  string color = this->color();
  if (t == SyncedStructType::SS_HIT || t == SyncedStructType::SS_HOLD) {
    return "assets/circle_" + color + ".png";
  } else if (t == SyncedStructType::SS_HOLD_TICK) {
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
    case SyncedStructType::SS_HIT:
      printf("HIT");
      break;
    case SyncedStructType::SS_HOLD:
      printf("HOLD");
      break;
    case SyncedStructType::SS_HOLD_TICK:
      printf("HOLD_TICK");
      break;
    case SyncedStructType::SS_TIMING_LINE:
      printf("TIMING_LINE");
      break;
  }
  printf(", keys: ");
  for (char c : keys) {
    printf("%c", c);
  }
  printf(" }");
}

// Create a SyncedStructList from an existing one.
SyncedStructList::SyncedStructList(SyncedStructList* l) {
  for (const auto& ss : l->vec) {
    vec.push_back(ss->clone());
  }
}
// Create a SyncedStructList from a string.
SyncedStructList::SyncedStructList(string s) {
  // split on comma, yielding all the hit objects in the chart grouped by beat
  vector<string> groupings = split(s, ",");
  for (string grouping : groupings) {
    vector<string> hit_objects_and_beat = split(grouping, "@");
    // if hit_objects_and_beat.size() > 2 {}
    string s_hit_objects = hit_objects_and_beat[0];
    string s_beat = hit_objects_and_beat[1];
    double d_beat;
    stringstream ss(s_beat);
    ss >> d_beat;
    Beat* ptr_beat = new Beat;
    ptr_beat->value = d_beat;
    // create a single timing line synced to the whole grouping
    SyncedStruct* timing_line = new SyncedStruct;
    timing_line->id = -1;
    timing_line->beat = ptr_beat;
    timing_line->t = SyncedStructType::SS_TIMING_LINE;
    timing_line->judgement = new Judgement;
    vec.push_back(timing_line);
    // split the hit objects on plus sign, separating the hits from the holds and their info
    vector<string> hits_and_holds = split(s_hit_objects, "+");
    // if hits_and_holds.size() > 2 {}
    string s_hits = hits_and_holds[0];
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
      ptr_hit_object->id = -1;
      ptr_hit_object->beat = ptr_beat;
      ptr_hit_object->t = SyncedStructType::SS_HIT;
      ptr_hit_object->keys = keys;
      ptr_hit_object->judgement = new Judgement;
      vec.push_back(ptr_hit_object);
    }
    if (hits_and_holds.size() == 2) {
      string s_holds_and_info = hits_and_holds[1];
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
        ptr_hit_object->id = -1;
        ptr_hit_object->beat = ptr_beat;
        ptr_hit_object->t = SyncedStructType::SS_HOLD;
        ptr_hit_object->keys = keys;
        ptr_hit_object->judgement = new Judgement;
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
          ptr_hit_object->id = -1;
          ptr_hit_object->beat = ptr_beat;
          ptr_hit_object->t = SyncedStructType::SS_HOLD_TICK;
          ptr_hit_object->keys = keys;
          ptr_hit_object->judgement = new Judgement;
          vec.push_back(ptr_hit_object);
          i += 1;
          tick_beat_value += delta;
        }
      }
    }
  }
}
// Destructor method.
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

// Destructor method.
DifficultyList::~DifficultyList() {
  vec.clear();
}
// Get the given difficulty from the list.
Difficulty* DifficultyList::get(string name) {
  for (auto difficulty : vec) {
    if (difficulty->name == name) {
      return difficulty;
    }
  }
  return nullptr;
}
// Get the names of all difficulties in the list.
vector<string> DifficultyList::names() {
  vector<string> v;
  for (auto difficulty : vec) {
    v.push_back(difficulty->name);
  }
  return v;
}

// Constructor method.
Chart::Chart(string filename) {
  std::map<string, std::map<string, string>> map;
  string group;
  vector<string> difficulty_names;
  // read lines
  string contents = read_file(filename);
  vector<string> lines = split(contents, "\n");
  for (auto line : lines) {
    if (empty(line)) continue;
    if (line.starts_with("[") && line.ends_with("]")) {
      group = deserialize_group(line);
      std::map<string, string> empty;
      map.insert({group, empty});
      // insert into difficulty names here to preserve insertion order
      if (group != "metadata") {
        difficulty_names.push_back(group);
      }
    } else {
      vector<string> key_and_value = deserialize_kv(line);
      string key = key_and_value[0];
      string value = key_and_value[1];
      pair<string, string> p(key, value);
      map[group].insert(p);
    }
  }
  // get keys and values
  std::map<string, string> map_metadata = map["metadata"];
  string s_version = map_metadata["version"];
  string s_title = map_metadata["title"];
  string s_subtitle = map_metadata["subtitle"];
  string s_artist = map_metadata["artist"];
  string s_credit = map_metadata["credit"];
  string s_bpms = map_metadata["bpms"];
  string s_preview = map_metadata["preview"];
  // create metadata
  metadata = new Metadata;
  metadata->version = 0;
  metadata->title = s_title;
  metadata->subtitle = s_subtitle;
  metadata->artist = s_artist;
  metadata->credit = s_credit;
  metadata->bpms = new BpmList(s_bpms);
  double d_preview;
  stringstream ss(s_preview);
  ss >> d_preview;
  Beat* ptr_preview = new Beat;
  ptr_preview->value = d_preview;
  metadata->preview = ptr_preview;
  // load audio
  string s_audio;
  if (empty(s_subtitle)) {
    s_audio = "assets/" + s_artist + " - " + s_title + ".wav";
  } else {
    s_audio = "assets/" + s_artist + " - " + s_title + " (" + s_subtitle + ").wav";
  }
  audio = new Music(s_audio);
  // load difficulties
  difficulties = new DifficultyList;
  for (auto difficulty_name : difficulty_names) {
    std::map<string, string> difficulty_map = map[difficulty_name];
    string s_hit_objects = difficulty_map["hit_objects"];
    Difficulty* difficulty = new Difficulty;
    difficulty->name = difficulty_name;
    difficulty->synced_struct_list = new SyncedStructList(s_hit_objects);
    difficulties->vec.push_back(difficulty);
  }
}
// Destructor method.
Chart::~Chart() {
  audio->~Music();
  difficulties->~DifficultyList();
}
// Get a specific difficulty of this chart.
Difficulty* Chart::get_difficulty(string name) {
  return difficulties->get(name);
}
void Chart::print() {
  printf("Chart { ");
  printf("metadata: ");
  metadata->print();
  printf(", audio: [private]");
  printf(", synced_structs: [private]");
  // synced_structs->print();
  printf(" }");
}

// Constructor method.
ChartWrapper::ChartWrapper() {
  chart_status = ChartStatus::NONE;
}
// Destructor method.
ChartWrapper::~ChartWrapper() {
  chart->~Chart();
  synced_structs->~SyncedStructList();
}
// Load a chart into this ChartWrapper.
void ChartWrapper::load_chart(Chart* c) {
  chart = c;
}
// Play the chart attached to this ChartWrapper.
void ChartWrapper::play_chart(string difficulty, SDL_Renderer* renderer, Objects* objects, Groups* groups) {
  // print
  string title = chart->metadata->title;
  string subtitle = chart->metadata->subtitle;
  string artist = chart->metadata->artist;
  string credit = chart->metadata->credit;
  if (empty(subtitle)) {
    printf("playing chart \"%s - %s [%s]\" (mapped by %s)...\n", artist.c_str(), title.c_str(), difficulty.c_str(), credit.c_str());
  } else {
    printf("playing chart \"%s - %s (%s) [%s]\" (mapped by %s)...\n", artist.c_str(), title.c_str(), subtitle.c_str(), difficulty.c_str(), credit.c_str());
  }
  // create object instances
  synced_structs = new SyncedStructList(chart->get_difficulty(difficulty)->synced_struct_list);
  for (SyncedStruct* synced : synced_structs->vec) {
    int id;
    // create object instance
    if (synced->t == SyncedStructType::SS_TIMING_LINE) {
      id = objects->create_instance(synced->asset(), 0.0, 1000.0, 25, 1, renderer);
      groups->insert_into_group("pure_calculation", id);
      groups->insert_into_group("timing_lines", id);
    } else {
      if (synced->t == SyncedStructType::SS_HOLD_TICK) {
        id = objects->create_instance(synced->asset(), synced->lane_x() + 10, 1000.0, 12, 12, renderer);
      } else {
        id = objects->create_instance(synced->asset(), synced->lane_x(), 1000.0, 32, 32, renderer);
      }
      groups->insert_into_group("pure_calculation", id);
      groups->insert_into_group("hit_objects", id);
      if (synced->t != SyncedStructType::SS_HOLD_TICK) {
        groups->insert_into_group("hits_and_holds", id);
      }
    }
    // set ID
    synced->id = id;
  }
  printf("hit objects: %d\n", groups->get_group("hit_objects")->size());
  chart_status = ChartStatus::PLAYING;
}

// Deserialize a group from .khel format into a `string`.
string deserialize_group(string raw) {
  string s = raw;
  s.erase(s.begin());
  s.pop_back();
  return s;
}
// Deserialize a key-value pair from .khel format into a `vector<string>`.
vector<string> deserialize_kv(string raw) {
  // if (empty(raw)) {}
  // if (!raw.ends_with("")) {}
  vector<string> vec;
  vector<string> key_and_value = split(raw, "=", 1);
  string key = key_and_value[0];
  string value = key_and_value[1];
  value.pop_back();
  vec.push_back(key);
  vec.push_back(value);
  return vec;
}

// Load all given charts into a vector<Chart*>.
vector<Chart*> load_all_charts_in_folder(string folder_name) {
  vector<Chart*> charts;
  vector<string> chart_names = filenames("charts/" + folder_name);
  for (string chart_name : chart_names) {
    string chart_path = "charts/" + folder_name + "/" + chart_name + ".khel";
    Chart* chart = new Chart(chart_path);
    charts.push_back(chart);
  }
  return charts;
}