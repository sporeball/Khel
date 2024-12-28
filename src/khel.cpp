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
  int audio_buffers = 1024;
  if (Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers) != 0) {
    printf("could not initialize SDL_mixer!: %s\n", Mix_GetError());
    return 1;
  }

  printf("all done\n");

  screenSurface = SDL_GetWindowSurface(window);

  Uint64 performance_counter_value_at_game_start = SDL_GetPerformanceCounter();
  // Uint64 then;
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

  init_imgui(window, renderer);
  set_imgui_style();
  int charts_listbox_index = 0;
  int difficulties_listbox_index = 0;

  vector<string> chart_names = crawl("charts");
  vector<Chart*> charts = load_all_charts(chart_names);
  Chart* chart;
  vector<string> difficulties;
  string difficulty;

  ChartWrapper* chart_wrapper = new ChartWrapper;
  chart_wrapper->chart_status = ChartStatus::PREVIEWING;

  float offset;
  int score;
  string judgement;

  SDL_Event e;
  int quit = 0;
  while (quit == 0) {
    // then = now;
    now = SDL_GetPerformanceCounter() - performance_counter_value_at_game_start;
    double now_seconds = (double) now / (double) performance_frequency;
    // double frame_time = (double) (now - then) / (double) performance_frequency;
    while (SDL_PollEvent(&e)) {
      ImGui_ImplSDL2_ProcessEvent(&e);
      switch (e.type) {
        case SDL_QUIT:
          quit = 1;
          break;
        case SDL_KEYDOWN:
          now = SDL_GetPerformanceCounter() - performance_counter_value_at_game_start;
          now_seconds = (double) now / (double) performance_frequency;
          SDL_KeyboardEvent* key = &e.key;
          switch (key->keysym.scancode) {
            case SDL_SCANCODE_1:
              chart_wrapper->chart->print();
              printf("\n");
              break;
            case SDL_SCANCODE_Q:
            case SDL_SCANCODE_A:
            case SDL_SCANCODE_Z:
            case SDL_SCANCODE_W:
            case SDL_SCANCODE_S:
            case SDL_SCANCODE_X:
            case SDL_SCANCODE_E:
            case SDL_SCANCODE_D:
            case SDL_SCANCODE_C:
            case SDL_SCANCODE_R:
            case SDL_SCANCODE_F:
            case SDL_SCANCODE_V:
            case SDL_SCANCODE_T:
            case SDL_SCANCODE_G:
            case SDL_SCANCODE_B:
            case SDL_SCANCODE_Y:
            case SDL_SCANCODE_H:
            case SDL_SCANCODE_N:
            case SDL_SCANCODE_U:
            case SDL_SCANCODE_J:
            case SDL_SCANCODE_M:
            case SDL_SCANCODE_I:
            case SDL_SCANCODE_K:
            case SDL_SCANCODE_COMMA:
            case SDL_SCANCODE_O:
            case SDL_SCANCODE_L:
            case SDL_SCANCODE_PERIOD:
            case SDL_SCANCODE_P:
            case SDL_SCANCODE_SEMICOLON:
            case SDL_SCANCODE_SLASH:
              chart_wrapper->try_hit(key->keysym.sym, difficulty, offset, now_seconds, performance_frequency, &score, &judgement);
              break;
            default:
              break;
          }
          break;
      }
    }
    // start imgui frame
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    {
      auto window_width = 800.0;
      ImGui::Begin("Khel", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
      ImGui::SetWindowPos(ImVec2(0.0, 0.0));
      ImGui::SetWindowSize(ImVec2(800.0, 600.0));
      if (chart_wrapper->chart_status == ChartStatus::PREVIEWING) {
        // detect change to chart
        if (charts[charts_listbox_index] != chart) {
          if (chart != nullptr) {
            chart->audio->fade_out();
          }
          chart = charts[charts_listbox_index];
          difficulties = chart->difficulties;
          difficulties_listbox_index = 0;
        }
        // fade in
        Beat* preview = chart->metadata->preview;
        double preview_seconds = preview->to_exact_time(chart->metadata->bpms);
        if (Mix_PlayingMusic() == 0) {
          chart->audio->fade_in(preview_seconds);
        }
        // fade out
        Beat* preview_end = new Beat;
        preview_end->value = preview->value + 32.0;
        double preview_end_seconds = preview_end->to_exact_time(chart->metadata->bpms);
        if (Mix_GetMusicPosition(chart->audio->music) >= preview_end_seconds - 0.5 && chart->audio->fading_out == 0) {
          chart->audio->fade_out();
        }
        // detect change to difficulty
        if (difficulties[difficulties_listbox_index] != difficulty) {
          difficulty = difficulties[difficulties_listbox_index];
        }
        ImGui::PushItemWidth(200.0);
        ImGui::ListBox(
          "##Chart",
          &charts_listbox_index,
          string_vector_getter,
          chart_names.data(),
          (int) chart_names.size()
        );
        ImGui::ListBox(
          "##Difficulty",
          &difficulties_listbox_index,
          string_vector_getter,
          difficulties.data(),
          (int) difficulties.size()
        );
        ImGui::SliderInt("AV", &av->value, 100.0f, 500.0f);
        ImGui::SliderFloat("Offset", &offset, -100.0f, 100.0f);
        ImGui::PopItemWidth();
        if (ImGui::Button("Play")) {
          chart->audio->stop();
          chart_wrapper->load_chart(chart);
          chart_wrapper->play_chart(difficulty, renderer, objects, groups);
          now = SDL_GetPerformanceCounter() - performance_counter_value_at_game_start;
          chart_wrapper->start_time = now;
          // create objects
          objects->create_instance("assets/line_white.png", 0.0, 120.0, 100, 1, renderer);
          for (int i = 0; i < 10; i++) {
            objects->create_instance("assets/circle_gray.png", ((40 * i) - 180) + 400, 104.0, 32, 32, renderer);
          }
        }
      }
      if (chart_wrapper->chart_status == ChartStatus::PLAYING) {
        double one_minute = 60.0;
        Bpm* bpm_at_zero = chart_wrapper->chart->metadata->bpms->at_exact_time(0.0);
        double one_beat_at_zero = one_minute / bpm_at_zero->value;
        double start_time_seconds = (double) chart_wrapper->start_time / (double) performance_frequency;
        now = SDL_GetPerformanceCounter() - performance_counter_value_at_game_start;
        now_seconds = (double) now / (double) performance_frequency;
        double exact_time_seconds = now_seconds - start_time_seconds - (one_beat_at_zero * 8.0);
        Bpm* bpm_now = chart_wrapper->chart->metadata->bpms->at_exact_time(exact_time_seconds);
        string bpm_text = format("{:.2f}", bpm_now->value);
        auto bpm_text_width = ImGui::CalcTextSize(bpm_text.c_str()).x;
        string score_text = format("{}", score);
        auto score_text_width = ImGui::CalcTextSize(score_text.c_str()).x;
        auto judgement_text_width = ImGui::CalcTextSize(judgement.c_str()).x;
        ImGui::Dummy(ImVec2(0.0f, 20.0f));
        ImGui::SetCursorPosX((window_width - score_text_width) * 0.95f);
        ImGui::Text("%s", score_text.c_str());
        ImGui::SetCursorPosX((window_width - bpm_text_width) * 0.5f);
        ImGui::Text("%s", bpm_text.c_str());
        ImGui::SetCursorPosX((window_width - judgement_text_width) * 0.5f);
        ImGui::Text("%s", judgement.c_str());
      }
      ImGui::End();
    }
    // updates
    // 1000 tps
    now = SDL_GetPerformanceCounter() - performance_counter_value_at_game_start;
    if (now - last_tick_1k >= un_1k) {
      if (chart_wrapper->chart_status == ChartStatus::PLAYING) {
        double one_minute = 60.0;
        Bpm* bpm_at_zero = chart_wrapper->chart->metadata->bpms->at_exact_time(0.0);
        double one_beat_at_zero = one_minute / bpm_at_zero->value; // seconds
        double start_time_seconds = (double) chart_wrapper->start_time / (double) performance_frequency;
        now = SDL_GetPerformanceCounter() - performance_counter_value_at_game_start;
        now_seconds = (double) now / (double) performance_frequency;
        double exact_time_seconds = now_seconds - start_time_seconds - (one_beat_at_zero * 8.0);
        if (exact_time_seconds > 0.0 && Mix_PlayingMusic() == 0) {
          printf("before: %f\n", (double) (SDL_GetPerformanceCounter() - performance_counter_value_at_game_start) / (double) performance_frequency);
          chart_wrapper->chart->audio->play();
          while (1) {
            if (Mix_PlayingMusic() != 0) break;
          }
          printf("after: %f\n", (double) (SDL_GetPerformanceCounter() - performance_counter_value_at_game_start) / (double) performance_frequency);
        }
      }
      last_tick_1k = now;
    }
    // 240 tps
    now = SDL_GetPerformanceCounter() - performance_counter_value_at_game_start;
    if (now - last_tick_240 >= un_240) {
      if (chart_wrapper->chart_status == ChartStatus::PLAYING) {
        // Group* hits_and_holds = groups->get_group("hits_and_holds");
        Group* pure_calculation = groups->get_group("pure_calculation");
        double one_minute = 60.0;
        Bpm* bpm_at_zero = chart_wrapper->chart->metadata->bpms->at_exact_time(0.0);
        double one_beat_at_zero = one_minute / bpm_at_zero->value;
        double start_time_seconds = (double) chart_wrapper->start_time / (double) performance_frequency;
        now = SDL_GetPerformanceCounter() - performance_counter_value_at_game_start;
        now_seconds = (double) now / (double) performance_frequency;
        double exact_time_seconds = now_seconds - start_time_seconds - (one_beat_at_zero * 8.0);
        for (int i = 0; i < pure_calculation->size(); i++) {
          // all hit objects and all timing lines are subject to pure calculation
          double y = 0.0;
          Beat* beat = chart_wrapper->chart->synced_structs[difficulty]->vec[i]->beat;
          SyncedStructType t = chart_wrapper->chart->synced_structs[difficulty]->vec[i]->t;
          // we are essentially getting the synced object's position at exact time zero...
          double exact_time_from_beat = beat->to_exact_time(chart_wrapper->chart->metadata->bpms);
          double position_at_exact_time_zero = av->over_time(exact_time_from_beat, chart_wrapper->chart->metadata->bpms);
          y -= position_at_exact_time_zero;
          // and translating it by the distance that it travels from zero to now
          double distance = av->over_time(exact_time_seconds, chart_wrapper->chart->metadata->bpms);
          y += distance;
          y -= 120.0; // correct
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