// macos: `g++ khel.cpp -o khel $(pkg-config --cflags --libs sdl2 sdl2_image sdl2_mixer) -std=c++17 -Wall && ./khel`
// (install SDL2, SDL2_image, and SDL2_mixer using MacPorts)

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <stdio.h>
#include <iostream>
#include "chart.h"
#include "object.h"
#include "util.h"

using namespace std;

int main(int argc, char* argv[]) {
  // setbuf(stdout, NULL);

  SDL_Window* window = NULL;
  SDL_Renderer* renderer = NULL;
  SDL_Surface* screenSurface = NULL;

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
    printf("Could not initialize SDL!: %s\n", SDL_GetError());
    return 1;
  }

  window = SDL_CreateWindow("Khel", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_SHOWN);
  if (window == NULL) {
    printf("Could not create window!: %s\n", SDL_GetError());
    return 1;
  }

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE | SDL_RENDERER_PRESENTVSYNC);
  if (renderer == NULL) {
    printf("Could not create renderer!: %s\n", SDL_GetError());
    return 1;
  }

  int imgFlags = IMG_INIT_PNG;
  if (!(IMG_Init(imgFlags) & imgFlags)) {
    printf("Could not initialize SDL_image!: %s\n", SDL_GetError());
    return 1;
  }

  int audio_rate = 44100;
  Uint16 audio_format = AUDIO_S16SYS;
  int audio_channels = 2;
  int audio_buffers = 4096;
  int channel;
  if (Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers) != 0) {
    printf("Could not initialize SDL_mixer!: %s\n", Mix_GetError());
    return 1;
  }

  screenSurface = SDL_GetWindowSurface(window);

  Uint64 performance_counter_value_at_game_start = SDL_GetPerformanceCounter();
  Uint64 now;

  Uint64 performance_frequency = SDL_GetPerformanceFrequency();
  printf("performance frequency: %llu\n", performance_frequency);
  int un_60 = performance_frequency / 60;
  int un_240 = performance_frequency / 240;
  int un_1k = performance_frequency / 1000;
  Uint64 last_tick_60 = performance_counter_value_at_game_start;
  Uint64 last_tick_240 = performance_counter_value_at_game_start;
  Uint64 last_tick_1k = performance_counter_value_at_game_start;

  AutoVelocity* av = new AutoVelocity;
  av->value = 300.0;

  Groups* groups = new Groups;
  groups->create_group("pure_calculation");
  groups->create_group("hit_objects");
  groups->create_group("hits_and_holds");
  groups->create_group("timing_lines");

  Objects* objects = new Objects;
  objects->create_instance("assets/line_white.png", 0.0, 300.0, 100, 1, renderer);

  ChartWrapper* chart_wrapper = new ChartWrapper;
  chart_wrapper->load_chart("charts/++.khel");
  chart_wrapper->chart->print();
  printf("\n");

  chart_wrapper->play_chart(renderer, objects, groups);
  now = SDL_GetPerformanceCounter() - performance_counter_value_at_game_start;
  chart_wrapper->start_time = now;

  SDL_Event e;
  int quit = 0;
  while (quit == 0) {
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) {
        quit = 1;
      }
    }
    now = SDL_GetPerformanceCounter() - performance_counter_value_at_game_start;
    double now_seconds = (double) now / performance_frequency;
    // 1000 tps
    if (now - last_tick_1k >= un_1k) {
      if (chart_wrapper->chart_status == ChartStatus::PLAYING) {
        double one_minute = 60.0;
        Bpm* bpm_at_zero = chart_wrapper->chart->metadata->bpms->at_exact_time(0.0);
        double one_beat_at_zero = one_minute / bpm_at_zero->value; // seconds
        double start_time_seconds = (double) chart_wrapper->start_time / (double) performance_frequency;
        double exact_time_seconds = now_seconds - start_time_seconds - (one_beat_at_zero * 8.0);
        if (exact_time_seconds > 0.0 && Mix_Playing(channel) == 0) {
          // play audio
          chart_wrapper->chart->audio = Mix_LoadWAV("assets/Cardboard Box - ++.wav");
          if (chart_wrapper->chart->audio == NULL) {
            printf("Could not load .wav file!: %s\n", Mix_GetError());
          }
          channel = Mix_PlayChannel(-1, chart_wrapper->chart->audio, 0);
          if (channel == -1) {
            printf("Could not play .wav file!: %s\n", Mix_GetError());
          }
        }
      }
      last_tick_1k = now;
    }
    // 60 tps
    if (now - last_tick_240 >= un_240) {
      if (chart_wrapper->chart_status == ChartStatus::PLAYING) {
        Group* hits_and_holds = groups->get_group("hits_and_holds");
        Group* pure_calculation = groups->get_group("pure_calculation");
        double one_minute = 60.0;
        Bpm* bpm_at_zero = chart_wrapper->chart->metadata->bpms->at_exact_time(0.0);
        double one_beat_at_zero = one_minute / bpm_at_zero->value;
        double start_time_seconds = (double) chart_wrapper->start_time / (double) performance_frequency;
        double exact_time_seconds = now_seconds - start_time_seconds - (one_beat_at_zero * 8.0);
        for (int i = 0; i < pure_calculation->size(); i++) {
          // all hit objects and all timing lines are subject to pure calculation
          double y = 0.0;
          Beat* beat = chart_wrapper->chart->synced_structs->vec[i]->beat;
          SyncedStructType t = chart_wrapper->chart->synced_structs->vec[i]->t;
          // we are essentially getting the synced object's position at exact time zero...
          double exact_time_from_beat = beat->to_exact_time(chart_wrapper->chart->metadata->bpms);
          double position_at_exact_time_zero = av->over_time(exact_time_from_beat, chart_wrapper->chart->metadata->bpms);
          y -= position_at_exact_time_zero;
          // and translating it by the distance that it travels from zero to now
          double distance = av->over_time(exact_time_seconds, chart_wrapper->chart->metadata->bpms);
          y += distance;
          y -= 300.0; // half the screen
          y *= -1.0; // coordinates are flipped
          int id = pure_calculation->instances[i];
          Instance* ptr = objects->get_instance(id);
          ptr->move(ptr->x, y);
        }
        for (auto id : pure_calculation->instances) {
          Object* o = objects->get_object(id);
          Instance* i = objects->get_instance(id);
          if (i->x < 0.0 || i->y < 0.0) {
            o->instances.erase(id);
            erase(pure_calculation->instances, id);
          }
        }
      }
      last_tick_240 = now;
    }
    SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 0x00, 0x00, 0x00));
    objects->draw_all_objects(screenSurface);
    SDL_UpdateWindowSurface(window);
  }

  SDL_FreeSurface(screenSurface);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  Mix_CloseAudio();
  IMG_Quit();
  SDL_Quit();
  
  return 0;
}