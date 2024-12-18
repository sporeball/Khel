#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <variant>
#include <vector>
#include <SDL_mixer.h>
#include "chart.h"
#include "util.h"

using namespace std;

// Convert this Beat to an exact time in seconds.
float Beat::to_exact_time(BpmList* bpms) {
  vector<Bpm*> vec = bpms->vec;
  float beats_remaining = value;
  float exact_time = 0.0f;
  float one_minute = 60.0f;
  // for (Bpm* ptr : vec) {
  // for (auto it = vec.begin(); it != vec.end(); ++it) {
  for (int i = 0; i < vec.size(); i++) {
    float one_beat = one_minute / value;
    if (i == vec.size() - 1) {
      exact_time += one_beat * beats_remaining;
      break;
    } else {
      Bpm* next_bpm = vec[i + 1];
      float length = vec[i]->length(next_bpm);
      if (beats_remaining < length / one_beat) {
        exact_time += one_beat * beats_remaining;
        break;
      } else {
        exact_time += length;
        beats_remaining - length / one_beat;
      }
    }
    return exact_time;
  }
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
  float f_value;
  stringstream ss(token_value);
  ss >> f_value;
  value = f_value;
  string token_start_beat = tokens[1];
  // printf("token_start_beat = %s\n", token_start_beat.c_str());
  float f_start_beat;
  stringstream ss2(token_start_beat);
  ss2 >> f_start_beat;
  Beat* ptr_start_beat = new Beat;
  ptr_start_beat->value = f_start_beat;
  start_beat = ptr_start_beat;
}
// Return the length of this Bpm in seconds.
float Bpm::length(Bpm* next_bpm) {
  if (next_bpm == NULL) {
    return numeric_limits<float>::max();
  }
  float one_minute = 60.0f;
  float beats = next_bpm->start_beat->value - this->start_beat->value;
  float one_beat = one_minute / this->start_beat->value;
  return beats * one_beat;
}
void Bpm::print() {
  printf("Bpm { ");
  printf("value: %f", value);
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
Bpm* BpmList::at_exact_time(float exact_time) {
  if (vec.size() == 1) {
    return vec[0];
  }
  if (exact_time <= 0.0) {
    return vec[0];
  }
  int i = 0;
  float time = exact_time;
  while (1) {
    Bpm* next_bpm = (i + 1 < vec.size()) ? vec[i + 1] : NULL;
    float length = vec[i]->length(next_bpm);
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
  vector<float> fvec;
  for (Bpm* ptr : vec) {
    fvec.push_back(ptr->value);
  }
  int index = distance(fvec.begin(), max_element(fvec.begin(), fvec.end()));
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
  // TODO
}
float SyncedStruct::lane_x() {
  // TODO
}
string SyncedStruct::color() {
  // TODO
}
string SyncedStruct::asset() {
  // TODO
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
    float f_beat;
    stringstream ss(s_beat);
    ss >> f_beat;
    Beat* ptr_beat = new Beat;
    ptr_beat->value = f_beat;
    // create a single timing line synced to the whole grouping
    SyncedStruct* timing_line = new SyncedStruct;
    timing_line->beat = ptr_beat;
    timing_line->t = SyncedStructType::TIMING_LINE;
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
    float f_length;
    stringstream ss2(s_length);
    ss2 >> f_length;
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
      float delta = f_length / (float) i_tick_count;
      float tick_beat_value = ptr_beat->value + delta;
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
void Chart::set_ratemod(float ratemod) {
  // TODO
}
void Chart::play() {
  // TODO
}
void Chart::print() {
  printf("Chart { ");
  printf("metadata: ");
  metadata->print();
  printf(", audio: [private]");
  printf(", synced_structs: ");
  synced_structs->print();
  printf(" }");
}

ChartInfo::~ChartInfo() {
  chart->~Chart();
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