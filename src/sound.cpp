#include <iostream>
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

// Destructor method.
Sounds::~Sounds() {
  for (auto it = sounds.begin(); it != sounds.end(); ) {
    it->second->~Sound();
    it = sounds.erase(it);
  }
}
// Create a new sound and return a pointer to it.
Sound* Sounds::create_sound(string filename) {
  Sound* sound = new Sound(filename);
  pair<string, Sound*> p(filename, sound);
  sounds.insert(p);
  return sound;
}
// Play the sound with the given filename.
// Creates the sound if it does not exist.
void Sounds::play_sound(string filename) {
  unordered_map<string, Sound*>::const_iterator got = sounds.find(filename);
  Sound* sound;
  if (got == sounds.end()) {
    sound = create_sound(filename);
  } else {
    sound = got->second;
  }
  sound->play();
}

// Constructor method.
Music::Music(string s) {
  music = Mix_LoadMUS(s.c_str());
  if (music == NULL) {
    printf("could not load .wav file!: %s\n", Mix_GetError());
  }
  fading_out = 0;
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
  fading_out = 0;
}
// Stop playing this music.
void Music::stop() {
  Mix_HaltMusic();
}
double Music::position() {
  return Mix_GetMusicPosition(music);
}
// Set the playback position of this music.
void Music::seek(double position) {
  Mix_SetMusicPosition(position);
}
// Fade in this music starting from the given position.
void Music::fade_in(double position) {
  Mix_FadeInMusicPos(music, 0, 500, position);
  fading_out = 0;
}
// Fade out this music.
void Music::fade_out() {
  Mix_FadeOutMusic(500);
  fading_out = 1;
}