#ifndef KHEL_SOUND_H
#define KHEL_SOUND_H

#include <string>
#include <unordered_map>
#include <SDL_mixer.h>

struct Sound {
  Mix_Chunk* chunk;
  int channel;
  Sound(std::string s);
  ~Sound();
  void play();
  void stop();
  int playing();
};

// A collection of all sounds created by Khel.
struct Sounds {
  std::unordered_map<std::string, Sound*> sounds;
  ~Sounds();
  Sound* create_sound(std::string filename);
  void play_sound(std::string filename);
};

struct Music {
  Mix_Music* music;
  int fading_out;
  Music(std::string s);
  ~Music();
  void play();
  void stop();
  double position();
  void seek(double position);
  void fade_in(double position);
  void fade_out();
};

#endif