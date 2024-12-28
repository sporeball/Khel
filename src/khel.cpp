// macos: `g++ -g src/*.cpp src/imgui/*.cpp -o khel $(pkg-config --cflags --libs sdl2 sdl2_image sdl2_mixer) -std=c++20 -Wall && ./khel`
// (install SDL2, SDL2_image, and SDL2_mixer using MacPorts)

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <stdio.h>
#include <iostream>
#include "chart.h"
#include "object.h"
#include "ui.h"
#include "util.h"

#define KHEL_VERSION_MAJOR 0
#define KHEL_VERSION_MINOR 0
#define KHEL_VERSION_PATCH 0

using namespace std;

int main() {
  // setbuf(stdout, NULL);

  SDL_Window* window = NULL;
  SDL_Renderer* renderer = NULL;
  SDL_Surface* screenSurface = NULL;

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
  int audio_buffers = 4096;
  if (Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers) != 0) {
    printf("could not initialize SDL_mixer!: %s\n", Mix_GetError());
    return 1;
  }

  printf("all done\n");

  screenSurface = SDL_GetWindowSurface(window);

  Uint64 performance_counter_value_at_game_start = SDL_GetPerformanceCounter();
  Uint64 now;

  Uint64 performance_frequency = SDL_GetPerformanceFrequency();
  printf("performance frequency: %llu\n", performance_frequency);
  Uint64 un_240 = performance_frequency / 240;
  Uint64 un_1k = performance_frequency / 1000;
  Uint64 last_tick_240 = performance_counter_value_at_game_start;
  Uint64 last_tick_1k = performance_counter_value_at_game_start;

  AutoVelocity* av = new AutoVelocity;
  av->value = 300;

  Groups* groups = new Groups;

  Objects* objects = new Objects;
  objects->create_instance("assets/line_white.png", 0.0, 300.0, 100, 1, renderer);

  init_imgui(window, renderer);
  set_imgui_style();

  ChartWrapper* chart_wrapper = new ChartWrapper;

  vector<string> charts = crawl("charts");
  int charts_listbox_index = 0;

  SDL_Event e;
  int quit = 0;
  while (quit == 0) {
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
              chart_wrapper->chart->print();
              printf("\n");
              break;
            default:
              break;
          }
          break;
      }
    }
    now = SDL_GetPerformanceCounter() - performance_counter_value_at_game_start;
    double now_seconds = (double) now / performance_frequency;
    // start imgui frame
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    {
      auto window_width = 800.0;
      ImGui::Begin("Khel", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
      ImGui::SetWindowPos(ImVec2(0.0, 0.0));
      ImGui::SetWindowSize(ImVec2(800.0, 600.0));
      ImGui::PushItemWidth(200.0);
      ImGui::ListBox(
        "##Chart",
        &charts_listbox_index,
        string_vector_getter,
        charts.data(),
        (int) charts.size()
      );
      if (ImGui::Button("Play")) {
        string chart_path = "charts/" + charts[charts_listbox_index] + ".khel";
        chart_wrapper->load_chart(chart_path);
        chart_wrapper->play_chart(renderer, objects, groups);
        chart_wrapper->start_time = now;
      }
      ImGui::SliderInt("AV", &av->value, 100.0f, 500.0f);
      ImGui::PopItemWidth();
      if (chart_wrapper->chart_status == ChartStatus::PLAYING) {
        double one_minute = 60.0;
        Bpm* bpm_at_zero = chart_wrapper->chart->metadata->bpms->at_exact_time(0.0);
        double one_beat_at_zero = one_minute / bpm_at_zero->value;
        double start_time_seconds = (double) chart_wrapper->start_time / (double) performance_frequency;
        double exact_time_seconds = now_seconds - start_time_seconds - (one_beat_at_zero * 8.0);
        Bpm* bpm_now = chart_wrapper->chart->metadata->bpms->at_exact_time(exact_time_seconds);
        string bpm_text = format("{:.2f}", bpm_now->value);
        auto bpm_text_width = ImGui::CalcTextSize(bpm_text.c_str()).x;
        ImGui::SetCursorPosX((window_width - bpm_text_width) * 0.5f);
        ImGui::Text("%s", bpm_text.c_str());
      }
      ImGui::End();
    }
    // updates
    // 1000 tps
    if (now - last_tick_1k >= un_1k) {
      if (chart_wrapper->chart_status == ChartStatus::PLAYING) {
        double one_minute = 60.0;
        Bpm* bpm_at_zero = chart_wrapper->chart->metadata->bpms->at_exact_time(0.0);
        double one_beat_at_zero = one_minute / bpm_at_zero->value; // seconds
        double start_time_seconds = (double) chart_wrapper->start_time / (double) performance_frequency;
        double exact_time_seconds = now_seconds - start_time_seconds - (one_beat_at_zero * 8.0);
        if (exact_time_seconds > 0.0 && chart_wrapper->chart->audio->playing() == 0) {
          chart_wrapper->chart->audio->play();
        }
      }
      last_tick_1k = now;
    }
    // 240 tps
    if (now - last_tick_240 >= un_240) {
      if (chart_wrapper->chart_status == ChartStatus::PLAYING) {
        // Group* hits_and_holds = groups->get_group("hits_and_holds");
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
          if (t == SyncedStructType::TIMING_LINE) {
            y += 16.0;
          }
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
    ImGui::Render();
    ImGuiIO& io = ImGui::GetIO();
    SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
    SDL_SetRenderDrawColor(renderer, (Uint8) 0, (Uint8) 0, (Uint8) 0, (Uint8) 255);
    SDL_RenderClear(renderer);
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
    objects->draw_all_objects(renderer);
    SDL_RenderPresent(renderer);
  }

  ImGui_ImplSDLRenderer2_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
  SDL_FreeSurface(screenSurface);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  Mix_CloseAudio();
  IMG_Quit();
  SDL_Quit();
  
  return 0;
}