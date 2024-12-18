#ifndef KHEL_CHART_H
#define KHEL_CHART_H

#include <string>
#include <variant>
#include <vector>
#include <SDL_mixer.h>

const int CHART_VERSION = 0;

struct Beat {
  float value;
  // float to_exact_time(BpmList* bpms);
};

struct Bpm {
  float value;
  Beat* start_beat;
  Bpm(std::string s);
  float length(Bpm* next_bpm);
};

struct BpmList {
  std::vector<Bpm*> vec;
  BpmList(std::string s);
  ~BpmList();
  Bpm* at_exact_time(float exact_time);
  Bpm* max();
};

struct Metadata {
  int version;
  std::string title;
  std::string subtitle;
  std::string artist;
  std::string credit;
  BpmList* bpms;
};

enum HitObjectType {
  HIT,
  HOLD,
  HOLD_TICK,
};

struct HitObject {
  Beat* beat;
  HitObjectType t;
  std::vector<char> keys;
  ~HitObject();
  int lane();
  float lane_x();
  std::string color();
  std::string asset();
};

struct TimingLine {
  Beat* beat;
  std::string asset();
};

using SyncedStruct = std::variant<HitObject, TimingLine>;

struct SyncedStructList {
  std::vector<SyncedStruct*> vec;
  SyncedStructList(std::string s);
  ~SyncedStructList();
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

#endif