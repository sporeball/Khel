#ifndef KHEL_UI_H
#define KHEL_UI_H

#include <string>
#include <unordered_map>
#include <vector>
#include <SDL.h>
#include "chart.h"
#include "khel.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"
#include "imgui/imgui_impl_sdlrenderer2.h"
#include "imgui/imgui_stdlib.h"

struct UiState {
  Chart* chart;
  std::vector<std::string> folder_names;
  std::string folder_name;
  std::vector<std::string> chart_names;
  std::string chart_name;
  std::vector<std::string> difficulty_names;
  std::string difficulty_name;
  std::vector<int> color_counts;
  std::string judgement;
  int folders_listbox_index;
  int charts_listbox_index;
  int difficulties_listbox_index;
  void draw_ui(KhelState* state);
  void draw_ui_previewing(KhelState* state);
  void draw_ui_playing(KhelState* state);
  void draw_ui_done(KhelState* state);
  void reset();
  UiState();
};

void init_imgui(SDL_Window* window, SDL_Renderer* renderer);
void set_imgui_style();
bool string_vector_getter(void* data, int n, const char** out_text);

#endif