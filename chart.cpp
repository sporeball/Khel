#include <iostream>
#include <limits>
#include <string>
#include <variant>
#include <vector>
#include <SDL_mixer.h>
#include "chart.h"

using namespace std;
using SyncedStruct = variant<HitObject, TimingLine>;

// Convert this Beat to an exact time in seconds.
// float Beat::to_exact_time(BpmList* bpms) {
//   vector<Bpm*> vec = bpms->vec;
//   float beats_remaining = value;
//   float exact_time = 0.0f;
//   float one_minute = 60.0f;
//   // for (Bpm* ptr : vec) {
//   // for (auto it = vec.begin(); it != vec.end(); ++it) {
//   for (int i = 0; i < vec.size(); i++) {
//     float one_beat = one_minute / value;
//     if (i == vec.size() - 1) {
//       exact_time += one_beat * beats_remaining;
//       break;
//     } else {
//       Bpm* next_bpm = vec[i + 1];
//       float length = vec[i]->length(next_bpm);
//       if (beats_remaining < length / one_beat) {
//         exact_time += one_beat * beats_remaining;
//         break;
//       } else {
//         exact_time += length;
//         beats_remaining - length / one_beat;
//       }
//     }
//     return exact_time;
//   }
// }

// Create a Bpm from a String.
// When possible, prefer creating a BpmList instead.
Bpm::Bpm(string s) {
  // TODO
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

// Create a BpmList from a string.
BpmList::BpmList(string s) {
  // TODO
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

HitObject::~HitObject() {
  keys.erase(keys.begin(), keys.end());
}
int HitObject::lane() {
  // TODO
}
float HitObject::lane_x() {
  // TODO
}
string HitObject::color() {
  // TODO
}
string HitObject::asset() {
  // TODO
}

string TimingLine::asset() {
  // TODO
}

SyncedStructList::SyncedStructList(string s) {
  // TODO
}
SyncedStructList::~SyncedStructList() {
  for (SyncedStruct* ptr : vec) {
    delete ptr;
  }
}

Chart::Chart(string filename) {
  // TODO
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

ChartInfo::~ChartInfo() {
  chart->~Chart();
}