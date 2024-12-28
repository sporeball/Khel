#ifndef KHEL_CHART_H
#define KHEL_CHART_H

#include <string>
#include <variant>
#include <vector>
#include <SDL.h>
#include <SDL_mixer.h>
#include "object.h"
#include "sound.h"

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
  BpmList* bpms;
  void print();
};

enum SyncedStructType {
  HIT,
  HOLD,
  HOLD_TICK,
  TIMING_LINE,
};

// Some kind of struct that's synced to a beat.
struct SyncedStruct {
  Beat* beat;
  SyncedStructType t;
  std::vector<char> keys;
  ~SyncedStruct();
  int lane();
  int lane_x();
  std::string color();
  std::string asset();
  void print();
};

// A list of structs synced to a beat.
struct SyncedStructList {
  std::vector<SyncedStruct*> vec;
  SyncedStructList(std::string s);
  ~SyncedStructList();
  void print();
};

struct Chart {
  Metadata* metadata;
  Sound* audio;
  SyncedStructList* synced_structs;
  Chart(std::string filename);
  ~Chart();
  void play();
  void print();
};

// Status of the current chart.
enum ChartStatus {
  NONE,
  PAUSED,
  PLAYING,
};

// A chart object and associated information.
struct ChartWrapper {
  Chart* chart;
  ChartStatus chart_status;
  Uint64 start_time;
  ChartWrapper();
  ~ChartWrapper();
  void load_chart(std::string filename);
  void play_chart(SDL_Renderer* renderer, Objects* objects, Groups* groups);
};

std::vector<std::string> deserialize_kv(std::string raw);

#endif