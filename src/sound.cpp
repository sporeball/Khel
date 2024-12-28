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

// Constructor method.
Music::Music(string s) {
  music = Mix_LoadMUS(s.c_str());
  if (music == NULL) {
    printf("could not load .wav file!: %s\n", Mix_GetError());
  }
}
// Destructor method.
Music::~Music() {
  Mix_FreeMusic(music);
}
// Start playing this music.
void Music::play() {
  if (Mix_PlayMusic(music, 0) != 0) {
    printf("could not play .wav file!: %s\n", Mix_GetError());
  }
}
// Stop playing this music.
void Music::stop() {
  Mix_HaltMusic();
}
// Set the playback position of this music.
void Music::seek(double position) {
  Mix_SetMusicPosition(position);
}
// Fade in this music starting from the given position.
void Music::fade_in(double position) {
  Mix_FadeInMusicPos(music, 0, 500, position);
}