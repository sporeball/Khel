#ifndef KHEL_SOUND_H
#define KHEL_SOUND_H

#include <string>
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