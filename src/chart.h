#ifndef KHEL_CHART_H
#define KHEL_CHART_H

#include <map>
#include <string>
#include <variant>
#include <vector>
#include <SDL.h>
#include <SDL_mixer.h>
#include "input.h"
#include "object.h"
#include "sound.h"

extern const std::map<char, int> map_columns;
extern const std::map<char, int> map_rows;

struct BpmList; // forward declaration

// Used to determine scroll speed.
struct AutoVelocity {
  int value;
  double at_exact_time(double exact_time, BpmList* bpms);
  double over_time(double time, BpmList* bpms);
};

// The beat that an object should be placed on.
struct Beat {
  double value;
  double to_exact_time(BpmList* bpms);
  void print();
};

// A tempo value in beats per minute.
struct Bpm {
  double value;
  Beat* start_beat;
  Bpm(std::string s);
  double length(Bpm* next_bpm);
  void print();
};

// A list of tempo values in beats per minute.
struct BpmList {
  std::vector<Bpm*> vec;
  BpmList(std::string s);
  ~BpmList();
  Bpm* at_exact_time(double exact_time);
  Bpm* min();
  Bpm* max();
  void print();
};

// The metadata associated with a chart.
struct Metadata {
  int version;
  std::string title;
  std::string subtitle;
  std::string artist;
  std::string credit;
  Beat* preview;
  BpmList* bpms;
  void print();
};

enum SyncedStructType {
  SS_HIT,
  SS_HOLD,
  SS_HOLD_TICK,
  SS_TIMING_LINE,
};

// Some kind of struct that's synced to a beat.
struct SyncedStruct {
  // The ID of a linked object instance.
  int id;
  Beat* beat;
  SyncedStructType t;
  std::vector<char> keys;
  Judgement* judgement;
  ~SyncedStruct();
  SyncedStruct* clone() const;
  int lane();
  int lane_x();
  std::string color();
  std::string asset();
  void print();
};

// A list of structs synced to a beat.
struct SyncedStructList {
  std::vector<SyncedStruct*> vec;
  SyncedStructList(SyncedStructList* l);
  SyncedStructList(std::string s);
  ~SyncedStructList();
  void print();
};

// A specific difficulty of a chart.
struct Difficulty {
  std::string name;
  SyncedStructList* synced_struct_list;
};

// A list of all difficulties of a chart.
struct DifficultyList {
  std::vector<Difficulty*> vec;
  ~DifficultyList();
  Difficulty* get(std::string name);
  std::vector<std::string> names();
};

// A chart object.
struct Chart {
  Metadata* metadata;
  Music* audio;
  DifficultyList* difficulties;
  Chart(std::string filename);
  ~Chart();
  Difficulty* get_difficulty(std::string name);
  void print();
};

// Status of the current chart.
enum ChartStatus {
  DONE,
  NONE,
  PAUSED,
  PLAYING,
  PREVIEWING,
};

// A chart object and associated information.
struct ChartWrapper {
  Chart* chart;
  ChartStatus chart_status;
  // Active synced structs.
  // This is copied from one of the wrapped chart's difficulties when `play_chart()` is called.
  SyncedStructList* synced_structs;
  Uint64 start_time;
  ChartWrapper();
  ~ChartWrapper();
  void load_chart(Chart* c);
  void play_chart(std::string difficulty, SDL_Renderer* renderer, Objects* objects, Groups* groups);
};

std::string deserialize_group(std::string raw);
std::vector<std::string> deserialize_kv(std::string raw);
std::vector<Chart*> load_all_charts(std::vector<std::string> chart_names);

#endif