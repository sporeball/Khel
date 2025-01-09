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
#define KHEL_MIN_FPS 120
#define KHEL_MAX_FPS 1000
#define KHEL_UPDATE_INTERVAL (1.0 / KHEL_MAX_FPS)
#define KHEL_MAX_CYCLES (KHEL_MAX_FPS / KHEL_MIN_FPS)

using namespace std;

// Constructor method.
KhelState::KhelState(SDL_Window* w, SDL_Renderer* r)
  : window(w), renderer(r)
{
  objects = new Objects;
  groups = new Groups;
  sounds = new Sounds;
  chart_wrapper = new ChartWrapper;
  chart_wrapper->chart_status = ChartStatus::PREVIEWING;
  av = new AutoVelocity;
  av->value = 300;
  keypresses = new KeyPressList;
  performance_counter_value_at_game_start = SDL_GetPerformanceCounter();
  performance_frequency = SDL_GetPerformanceFrequency();
  offset = 0;
  visual_offset = 0;
  countdown_ticks = 0;
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
// Reset the state.
void KhelState::reset() {
  objects->clear_all();
  groups->clear_all();
  judgements.clear();
  countdown_ticks = 0;
  marvelous_count = 0;
  perfect_count = 0;
  great_count = 0;
  good_count = 0;
  miss_count = 0;
  combo = 0;
  lowest_judgement_in_combo = new Judgement;
  score = 0.0;
  max_score_per_object = 0.0;
}

void poll_event(SDL_Event* e, int* quit, KhelState* state, UiState* ui_state) {
  while (SDL_PollEvent(e)) {
    ImGui_ImplSDL2_ProcessEvent(e);
    switch (e->type) {
      case SDL_QUIT:
        *quit = 1;
        break;
      case SDL_KEYDOWN:
        if (state->chart_wrapper->chart_status == ChartStatus::PLAYING && e->key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
          state->reset();
          ui_state->reset();
          state->chart_wrapper->chart->audio->fade_out();
          state->chart_wrapper->chart_status = ChartStatus::PREVIEWING;
        }
        if (state->chart_wrapper->chart_status == ChartStatus::DONE) {
          state->reset();
          ui_state->reset();
          state->chart_wrapper->chart_status = ChartStatus::PREVIEWING;
        }
        switch (e->key.keysym.scancode) {
          case SDL_SCANCODE_1:
            state->chart_wrapper->chart->print();
            printf("\n");
            break;
          default:
            if (!map_keys.contains(e->key.keysym.scancode)) break;
            state->keypresses->add(map_keys.at(e->key.keysym.scancode), state->now());
            try_hit(state, ui_state);
            break;
        }
        break;
      case SDL_KEYUP:
        switch (e->key.keysym.scancode) {
          default:
            if (!map_keys.contains(e->key.keysym.scancode)) break;
            state->keypresses->remove(map_keys.at(e->key.keysym.scancode));
            break;
        }
        break;
    }
  }
}

void try_count(KhelState* state) {
  Bpm* bpm_at_zero = state->chart_wrapper->chart->metadata->bpms->at_exact_time(0.0);
  double one_beat_at_zero = 60.0 / bpm_at_zero->value; // seconds
  double chart_time = state->chart_time();
  // count 3
  if (chart_time + (one_beat_at_zero * 4.0) >= 0.0 && state->countdown_ticks == 0) {
    state->sounds->play_sound("assets/count.wav");
    state->countdown_ticks += 1;
  }
  // count 2
  if (chart_time + (one_beat_at_zero * 3.0) >= 0.0 && state->countdown_ticks == 1) {
    state->sounds->play_sound("assets/count.wav");
    state->countdown_ticks += 1;
  }
  // count 1
  if (chart_time + (one_beat_at_zero * 2.0) >= 0.0 && state->countdown_ticks == 2) {
    state->sounds->play_sound("assets/count.wav");
    state->countdown_ticks += 1;
  }
  // count 0
  if (chart_time + one_beat_at_zero >= 0.0 && state->countdown_ticks == 3) {
    state->sounds->play_sound("assets/count.wav");
    state->countdown_ticks += 1;
  }
}

void try_play_audio(KhelState* state) {
  if (state->chart_time() >= 0.0 && Mix_PlayingMusic() == 0) {
    state->chart_wrapper->chart->audio->play();
  }
}

void try_update_synced_structs(KhelState* state, UiState* ui_state) {
  double chart_time = state->chart_time();
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
}

void try_end_chart(KhelState* state) {
  if (state->chart_wrapper->synced_structs->vec.size() == 0) {
    state->chart_wrapper->chart->audio->fade_out();
    state->chart_wrapper->chart_status = ChartStatus::DONE;
  }
}

void update_object_instances(KhelState* state) {
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
    y += state->visual_offset; // negative = sooner
    if (t == SyncedStructType::SS_TIMING_LINE) {
      y += 16.0;
    }
    Instance* ptr = state->objects->get_instance(id);
    ptr->move(ptr->x, y);
  }
}

void draw(KhelState* state, UiState* ui_state) {
  ui_state->draw_ui(state);
  ImGui::Render();
  ImGuiIO& io = ImGui::GetIO();
  SDL_RenderSetScale(state->renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
  SDL_SetRenderDrawColor(state->renderer, (Uint8) 0, (Uint8) 0, (Uint8) 0, (Uint8) 255);
  SDL_RenderClear(state->renderer);
  ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), state->renderer);
  state->objects->draw_all_objects(state->renderer);
  SDL_RenderPresent(state->renderer);
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
  int renderer_flags_accelerated = SDL_RENDERER_ACCELERATED;
  int renderer_flags_software = SDL_RENDERER_SOFTWARE;
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
  ui_state->folder_names = foldernames("charts");

  printf("all done\n");

  double last_frame_time = 0.0;
  double cycles_left_over = 0.0;
  double now;
  double update_iterations;

  SDL_Event e;
  int quit = 0;

  while (quit == 0) {
    poll_event(&e, &quit, state, ui_state);
    now = (double) state->now();
    update_iterations = (now - last_frame_time) + cycles_left_over;
    if (update_iterations > KHEL_MAX_CYCLES * KHEL_UPDATE_INTERVAL) {
      update_iterations = KHEL_MAX_CYCLES * KHEL_UPDATE_INTERVAL;
    }
    while (update_iterations > KHEL_UPDATE_INTERVAL) {
      update_iterations -= KHEL_UPDATE_INTERVAL;
      if (state->chart_wrapper->chart_status == ChartStatus::PLAYING) {
        try_count(state);
        try_play_audio(state);
        try_hold(state, ui_state);
        try_update_synced_structs(state, ui_state);
        try_end_chart(state);
        update_object_instances(state);
      }
    }
    cycles_left_over = update_iterations;
    last_frame_time = now;
    draw(state, ui_state);
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