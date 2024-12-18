#ifndef KHEL_CHART_H
#define KHEL_CHART_H

#include <string>
#include <variant>
#include <vector>
#include <SDL_mixer.h>

struct BpmList;
struct Beat {
  float value;
  float to_exact_time(BpmList* bpms);
  void print();
};

struct Bpm {
  float value;
  Beat* start_beat;
  Bpm(std::string s);
  float length(Bpm* next_bpm);
  void print();
};

struct BpmList {
  std::vector<Bpm*> vec;
  BpmList(std::string s);
  ~BpmList();
  Bpm* at_exact_time(float exact_time);
  Bpm* max();
  void print();
};

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

struct SyncedStruct {
  Beat* beat;
  SyncedStructType t;
  std::vector<char> keys;
  ~SyncedStruct();
  int lane();
  float lane_x();
  std::string color();
  std::string asset();
  void print();
};

struct SyncedStructList {
  std::vector<SyncedStruct*> vec;
  SyncedStructList(std::string s);
  ~SyncedStructList();
  void print();
};

struct Chart {
  Metadata* metadata;
  Mix_Chunk* audio;
  SyncedStructList* synced_structs;
  Chart(std::string filename);
  ~Chart();
  // void write_to_disk(std::string filename);
  void set_ratemod(float ratemod);
  void play();
  void print();
};

enum ChartStatus {
  NONE,
  PAUSED,
  PLAYING,
};

struct ChartInfo {
  Chart* chart;
  ChartStatus chart_status;
  float start_time;
  ~ChartInfo();
};

std::vector<std::string> deserialize_kv(std::string raw);

#endif