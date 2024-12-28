#include <string>
#include <SDL_mixer.h>
#include "sound.h"

using namespace std;

// Constructor method.
Sound::Sound(string s) {
  chunk = Mix_LoadWAV(s.c_str());
  if (chunk == NULL) {
    printf("could not load .wav file!: %s\n", Mix_GetError());
  }
}
// Destructor method.
Sound::~Sound() {
  Mix_FreeChunk(chunk);
}
// Start playing this sound.
void Sound::play() {
  channel = Mix_PlayChannel(-1, chunk, 0);
  if (channel == -1) {
    printf("could not play .wav file!: %s\n", Mix_GetError());
  }
}
// Stop playing this sound.
void Sound::stop() {
  Mix_HaltChannel(channel);
}
// Return whether this sound is currently playing.
int Sound::playing() {
  return Mix_Playing(channel);
}