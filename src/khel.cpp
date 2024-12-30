// macos: `g++ -g src/*.cpp src/imgui/*.cpp -o khel $(pkg-config --cflags --libs sdl2 sdl2_image sdl2_mixer) -std=c++20 -Wall && ./khel`
// (install SDL2, SDL2_image, and SDL2_mixer using MacPorts)

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <stdio.h>
#include <iostream>
#include "chart.h"
#include "input.h"
#include "khel.h"
#include "object.h"
#include "ui.h"
#include "util.h"

#define KHEL_VERSION_MAJOR 0
#define KHEL_VERSION_MINOR 0
#define KHEL_VERSION_PATCH 0

using namespace std;

// Constructor method.
KhelState::KhelState(SDL_Window* w, SDL_Renderer* r)
  : window(w), renderer(r)
{
  objects = new Objects;
  groups = new Groups;
  printf("loading charts...\n");
  chart_names = crawl("charts");
  charts = load_all_charts(chart_names);
  printf("loaded %lu charts\n", charts.size());
  chart_wrapper = new ChartWrapper;
  chart_wrapper->chart_status = ChartStatus::PREVIEWING;
  av = new AutoVelocity;
  av->value = 300;
  performance_counter_value_at_game_start = SDL_GetPerformanceCounter();
  performance_frequency = SDL_GetPerformanceFrequency();
  printf("performance frequency: %llu\n", performance_frequency);
}
// Return the number of ticks of SDL's high resolution counter elapsed since Khel started.
Uint64 KhelState::now() {
  return SDL_GetPerformanceCounter() - performance_counter_value_at_game_start;
}

int main() {
  SDL_Window* window = NULL;
  SDL_Renderer* renderer = NULL;

  printf("Khel (C++ ver.) v%d.%d.%d\n", KHEL_VERSION_MAJOR, KHEL_VERSION_MINOR, KHEL_VERSION_PATCH);

  printf("initializing SDL...\n");
  int sdl_flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO;
  if (SDL_Init(sdl_flags) != 0) {
    printf("could not initialize SDL!: %s\n", SDL_GetError());
    return 1;
  }

  printf("creating window...\n");
  int window_flags = SDL_WINDOW_ALLOW_HIGHDPI;
  window = SDL_CreateWindow("Khel", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, window_flags);
  if (window == NULL) {
    printf("could not create window!: %s\n", SDL_GetError());
    return 1;
  }

  printf("creating renderer...\n");
  int renderer_flags_accelerated = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
  int renderer_flags_software = SDL_RENDERER_SOFTWARE | SDL_RENDERER_PRESENTVSYNC;
  renderer = SDL_CreateRenderer(window, -1, renderer_flags_accelerated);
  if (renderer == NULL) {
    printf("could not create accelerated renderer!: %s\n", SDL_GetError());
    printf("falling back to software renderer\n");
    renderer = SDL_CreateRenderer(window, -1, renderer_flags_software);
    if (renderer == NULL) {
      printf("could not create software renderer!: %s\n", SDL_GetError());
      return 1;
    }
  }

  printf("initializing SDL_image...\n");
  int img_flags = IMG_INIT_PNG;
  if (IMG_Init(img_flags) != img_flags) {
    printf("could not initialize SDL_image!: %s\n", SDL_GetError());
    return 1;
  }

  printf("initializing SDL_mixer...\n");
  int audio_rate = 44100;
  Uint16 audio_format = AUDIO_S16SYS;
  int audio_channels = 2;
  int audio_buffers = 1024;
  if (Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers) != 0) {
    printf("could not initialize SDL_mixer!: %s\n", Mix_GetError());
    return 1;
  }

  printf("initializing imgui...\n");
  init_imgui(window, renderer);
  set_imgui_style();

  KhelState* state = new KhelState(window, renderer);
  UiState* ui_state = new UiState;

  printf("all done\n");

  Uint64 un_240 = state->performance_frequency / 240;
  Uint64 un_1k = state->performance_frequency / 1000;
  Uint64 last_tick_240 = state->performance_counter_value_at_game_start;
  Uint64 last_tick_1k = state->performance_counter_value_at_game_start;

  // int max_score_per_object;

  SDL_Event e;
  int quit = 0;
  while (quit == 0) {
    // then = now;
    // double frame_time = (double) (now - then) / (double) performance_frequency;
    while (SDL_PollEvent(&e)) {
      ImGui_ImplSDL2_ProcessEvent(&e);
      switch (e.type) {
        case SDL_QUIT:
          quit = 1;
          break;
        case SDL_KEYDOWN:
          SDL_KeyboardEvent* key = &e.key;
          switch (key->keysym.scancode) {
            case SDL_SCANCODE_1:
              state->chart_wrapper->chart->print();
              printf("\n");
              break;
            default:
              break;
          }
          break;
      }
    }
    ui_state->draw_ui(state);
    // updates
    // 1000 tps
    if (state->now() - last_tick_1k >= un_1k) {
      if (state->chart_wrapper->chart_status == ChartStatus::PLAYING) {
        double one_minute = 60.0;
        Bpm* bpm_at_zero = state->chart_wrapper->chart->metadata->bpms->at_exact_time(0.0);
        double one_beat_at_zero = one_minute / bpm_at_zero->value; // seconds
        double start_time_seconds = as_seconds(state->chart_wrapper->start_time);
        double now_seconds = as_seconds(state->now());
        double exact_time_seconds = now_seconds - start_time_seconds - (one_beat_at_zero * 8.0);
        // play audio
        if (exact_time_seconds > 0.0 && Mix_PlayingMusic() == 0) {
          state->chart_wrapper->chart->audio->play();
        }
        // handle key presses
        const Uint8* keyboard_state = SDL_GetKeyboardState(NULL);
        SyncedStructList* synced_struct_list = state->chart_wrapper->chart->get_difficulty(ui_state->difficulty)->synced_struct_list;
        vector<SyncedStruct*> hits_and_holds;
        vector<SyncedStruct*> hits_and_holds_within_window;
        // determine which hits and holds are within the timing window
        for (auto synced : synced_struct_list->vec) {
          if (synced->t != SyncedStructType::HIT && synced->t != SyncedStructType::HOLD) continue;
          hits_and_holds.push_back(synced);
          double synced_exact_time = synced->beat->to_exact_time(state->chart_wrapper->chart->metadata->bpms);
          double early_limit = synced_exact_time - 0.135;
          double late_limit = synced_exact_time + 0.135;
          if (exact_time_seconds >= early_limit && exact_time_seconds <= late_limit) {
            hits_and_holds_within_window.push_back(synced);
          } else if (exact_time_seconds > late_limit) {
            synced_struct_list->vec.erase(remove(synced_struct_list->vec.begin(), synced_struct_list->vec.end(), synced), synced_struct_list->vec.end());
          }
        }
        // determine which hits and holds are having their keys pressed right now
        vector<SyncedStruct*> matches;
        for (auto synced : hits_and_holds_within_window) {
          if (all_of(synced->keys.begin(), synced->keys.end(), [keyboard_state](char c) {
            return keyboard_state[map_scancodes.at(c)] == 1;
          })) {
            matches.push_back(synced);
          }
        }
        // figure out how accurate each one is
        for (auto match : matches) {
          string s_keys(match->keys.begin(), match->keys.end());
          double match_exact_time = match->beat->to_exact_time(state->chart_wrapper->chart->metadata->bpms);
          double hit_time_ms = (exact_time_seconds - match_exact_time) * 1000.0;
          if (hit_time_ms < 0.0) {
            printf("hit %s %f ms early\n", s_keys.c_str(), std::abs(hit_time_ms));
          } else if (hit_time_ms > 0.0) {
            printf("hit %s %f ms late\n", s_keys.c_str(), hit_time_ms);
          } else {
            printf("hit %s exactly on time!\n", s_keys.c_str());
          }
          if (std::abs(hit_time_ms) <= 23.0) { // marvelous
            // state->score += max_score_per_object;
            ui_state->judgement = "marvelous!";
          } else if (std::abs(hit_time_ms) <= 45.0) { // perfect
            // state->score += ceil(max_score_per_object * 0.75);
            ui_state->judgement = "perfect";
          } else if (std::abs(hit_time_ms) <= 90.0) { // great
            // state->score += ceil(max_score_per_object * 0.5);
            ui_state->judgement = "great";
          } else { // good
            // state->score += ceil(max_score_per_object * 0.25);
            ui_state->judgement = "good";
          }
          synced_struct_list->vec.erase(remove(synced_struct_list->vec.begin(), synced_struct_list->vec.end(), match), synced_struct_list->vec.end());
        }
      }
      last_tick_1k = state->now();
    }
    // 240 tps
    if (state->now() - last_tick_240 >= un_240) {
      if (state->chart_wrapper->chart_status == ChartStatus::PLAYING) {
        // Group* hits_and_holds = groups->get_group("hits_and_holds");
        Group* pure_calculation = state->groups->get_group("pure_calculation");
        double one_minute = 60.0;
        Bpm* bpm_at_zero = state->chart_wrapper->chart->metadata->bpms->at_exact_time(0.0);
        double one_beat_at_zero = one_minute / bpm_at_zero->value;
        double start_time_seconds = as_seconds(state->chart_wrapper->start_time);
        double now_seconds = as_seconds(state->now());
        double exact_time_seconds = now_seconds - start_time_seconds - (one_beat_at_zero * 8.0);
        SyncedStructList* synced_struct_list = state->chart_wrapper->chart->get_difficulty(ui_state->difficulty)->synced_struct_list;
        for (int i = 0; i < pure_calculation->size(); i++) {
          // all hit objects and all timing lines are subject to pure calculation
          double y = 0.0;
          Beat* beat = synced_struct_list->vec[i]->beat;
          SyncedStructType t = synced_struct_list->vec[i]->t;
          // we are essentially getting the synced object's position at exact time zero...
          double exact_time_from_beat = beat->to_exact_time(state->chart_wrapper->chart->metadata->bpms);
          double position_at_exact_time_zero = state->av->over_time(exact_time_from_beat, state->chart_wrapper->chart->metadata->bpms);
          y -= position_at_exact_time_zero;
          // and translating it by the distance that it travels from zero to now
          double distance = state->av->over_time(exact_time_seconds, state->chart_wrapper->chart->metadata->bpms);
          y += distance;
          y -= 120.0; // correct
          y *= -1.0; // coordinates are flipped
          if (t == SyncedStructType::TIMING_LINE) {
            y += 16.0;
          }
          int id = pure_calculation->instances[i];
          Instance* ptr = state->objects->get_instance(id);
          ptr->move(ptr->x, y);
        }
        for (auto id : pure_calculation->instances) {
          Object* o = state->objects->get_object(id);
          Instance* i = state->objects->get_instance(id);
          if (i->x < 0.0 || i->y < 0.0) {
            o->instances.erase(id);
            erase(pure_calculation->instances, id);
          }
        }
      }
      last_tick_240 = state->now();
    }
    ImGui::Render();
    ImGuiIO& io = ImGui::GetIO();
    SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
    SDL_SetRenderDrawColor(renderer, (Uint8) 0, (Uint8) 0, (Uint8) 0, (Uint8) 255);
    SDL_RenderClear(renderer);
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
    state->objects->draw_all_objects(renderer);
    SDL_RenderPresent(renderer);
  }

  ImGui_ImplSDLRenderer2_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  Mix_CloseAudio();
  IMG_Quit();
  SDL_Quit();
  
  return 0;
}