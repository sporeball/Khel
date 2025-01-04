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
  folder_names = foldernames("charts");
  chart_wrapper = new ChartWrapper;
  chart_wrapper->chart_status = ChartStatus::PREVIEWING;
  av = new AutoVelocity;
  av->value = 300;
  keypresses = new KeyPressList;
  performance_counter_value_at_game_start = SDL_GetPerformanceCounter();
  performance_frequency = SDL_GetPerformanceFrequency();
  offset = 0;
  marvelous_count = 0;
  perfect_count = 0;
  great_count = 0;
  good_count = 0;
  miss_count = 0;
  combo = 0;
  lowest_judgement_in_combo = new Judgement;
  score = 0.0;
  max_score_per_object = 0.0;
  printf("performance frequency: %llu\n", performance_frequency);
}
// Return the number of ticks of SDL's high resolution counter elapsed since Khel started.
Uint64 KhelState::now() {
  return SDL_GetPerformanceCounter() - performance_counter_value_at_game_start;
}
// Return the number of seconds elapsed in the currently playing chart.
// This method should only be called when the chart status is ChartStatus::PLAYING.
double KhelState::chart_time() {
  Bpm* bpm_at_zero = chart_wrapper->chart->metadata->bpms->at_exact_time(0.0);
  double one_beat_at_zero = 60.0 / bpm_at_zero->value; // seconds
  double start_time_seconds = as_seconds(chart_wrapper->start_time);
  double now_seconds = as_seconds(now());
  return now_seconds - start_time_seconds - (one_beat_at_zero * 8.0);
}
// Completely remove a synced struct from the state.
void KhelState::remove_synced_struct(SyncedStruct* synced) {
  // remove from objects and groups (stops rendering)
  objects->destroy_instance(synced->id);
  groups->remove_from_all_groups(synced->id);
  // remove from chart wrapper
  chart_wrapper->synced_structs->vec.erase(
    remove(
      chart_wrapper->synced_structs->vec.begin(),
      chart_wrapper->synced_structs->vec.end(),
      synced
    ),
    chart_wrapper->synced_structs->vec.end()
  );
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
          if (state->chart_wrapper->chart_status == ChartStatus::DONE) {
            state->chart_wrapper->chart_status = ChartStatus::PREVIEWING;
            state->objects->clear_all();
            state->groups->clear_all();
            state->judgements.clear();
            state->marvelous_count = 0;
            state->perfect_count = 0;
            state->great_count = 0;
            state->good_count = 0;
            state->miss_count = 0;
            state->combo = 0;
            state->lowest_judgement_in_combo = new Judgement;
            state->score = 0.0;
            state->max_score_per_object = 0.0;
            ui_state->judgement = "";
          }
          switch (e.key.keysym.scancode) {
            case SDL_SCANCODE_1:
              state->chart_wrapper->chart->print();
              printf("\n");
              break;
            default:
              if (!map_keys.contains(e.key.keysym.scancode)) break;
              state->keypresses->add(map_keys.at(e.key.keysym.scancode), state->now());
              try_hit(state, ui_state);
              break;
          }
          break;
        case SDL_KEYUP:
          switch (e.key.keysym.scancode) {
            default:
              if (!map_keys.contains(e.key.keysym.scancode)) break;
              state->keypresses->remove(map_keys.at(e.key.keysym.scancode));
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
        double chart_time = state->chart_time();
        // play audio
        if (chart_time >= 0.0 && Mix_PlayingMusic() == 0) {
          state->chart_wrapper->chart->audio->play();
        }
        // try hold
        try_hold(state, ui_state);
        // for each synced struct...
        for (auto synced : state->chart_wrapper->synced_structs->vec) {
          double synced_exact_time = synced->beat->to_exact_time(state->chart_wrapper->chart->metadata->bpms);
          double late_limit = synced_exact_time + 0.135;
          // remove super late synced structs
          if (chart_time > late_limit + 1.0) {
            state->remove_synced_struct(synced);
          }
          // miss late synced structs
          else if (chart_time > late_limit && synced->t != SyncedStructType::SS_TIMING_LINE && synced->judgement->t() == JudgementType::J_NONE) {
            judge(-10000.0, synced, state, ui_state);
          }
        }
        // end chart
        if (state->chart_wrapper->synced_structs->vec.size() == 0) {
          state->chart_wrapper->chart->audio->fade_out();
          state->chart_wrapper->chart_status = ChartStatus::DONE;
        }
      }
      last_tick_1k = state->now();
    }
    // 240 tps
    if (state->now() - last_tick_240 >= un_240) {
      if (state->chart_wrapper->chart_status == ChartStatus::PLAYING) {
        // Group* hits_and_holds = groups->get_group("hits_and_holds");
        Group* pure_calculation = state->groups->get_group("pure_calculation");
        SyncedStructList* synced_struct_list = state->chart_wrapper->synced_structs;
        for (int i = 0; i < pure_calculation->size(); i++) {
          int id = pure_calculation->instances[i];
          SyncedStruct* synced;
          for (auto ss : synced_struct_list->vec) {
            if (ss->id == id) {
              synced = ss;
            }
          }
          double y = 0.0;
          Beat* beat = synced->beat;
          SyncedStructType t = synced->t;
          // we are essentially getting the synced object's position at exact time zero...
          double exact_time_from_beat = beat->to_exact_time(state->chart_wrapper->chart->metadata->bpms);
          double position_at_exact_time_zero = state->av->over_time(exact_time_from_beat, state->chart_wrapper->chart->metadata->bpms);
          y -= position_at_exact_time_zero;
          // and translating it by the distance that it travels from zero to now
          double distance = state->av->over_time(state->chart_time(), state->chart_wrapper->chart->metadata->bpms);
          y += distance;
          y -= 120.0; // correct
          y *= -1.0; // coordinates are flipped
          if (t == SyncedStructType::SS_TIMING_LINE) {
            y += 16.0;
          }
          Instance* ptr = state->objects->get_instance(id);
          ptr->move(ptr->x, y);
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