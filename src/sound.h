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

#endif