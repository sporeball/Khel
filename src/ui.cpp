#include <iostream>
#include <string>
#include <SDL.h>
#include "ui.h"
#include "util.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"
#include "imgui/imgui_impl_sdlrenderer2.h"
#include "imgui/imgui_stdlib.h"

using namespace std;

bool string_vector_getter(void* data, int n, const char** out_text);

// Constructor method.
UiState::UiState() {
  chart = nullptr;
  charts_listbox_index = 0;
  difficulties_listbox_index = 0;
}

// Initialize imgui.
void init_imgui(SDL_Window* window, SDL_Renderer* renderer) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.IniFilename = NULL;
  io.LogFilename = NULL;
  io.ConfigFlags = ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
  io.Fonts->AddFontFromFileTTF("assets/rainyhearts.ttf", 16.0f);

  ImGui::StyleColorsDark();

  ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer2_Init(renderer);
}

// Set imgui's style.
void set_imgui_style() {
  ImGuiStyle& style = ImGui::GetStyle();
  ImVec4* colors = style.Colors;

  colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
  colors[ImGuiCol_TextDisabled] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
  colors[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
  colors[ImGuiCol_ChildBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
  colors[ImGuiCol_PopupBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
  colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
  colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  colors[ImGuiCol_FrameBg] = ImVec4(0.1f, 0.1f, 0.1f, 1.00f);
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
  colors[ImGuiCol_TitleBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
  colors[ImGuiCol_TitleBgActive] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
  colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
  colors[ImGuiCol_MenuBarBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
  colors[ImGuiCol_ScrollbarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
  colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
  colors[ImGuiCol_CheckMark] = ImVec4(0.86f, 0.93f, 0.89f, 1.00f);
  colors[ImGuiCol_SliderGrab] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
  colors[ImGuiCol_SliderGrabActive] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
  colors[ImGuiCol_Button] = ImVec4(0.1f, 0.1f, 0.1f, 1.00f);
  colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
  colors[ImGuiCol_ButtonActive] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
  colors[ImGuiCol_Header] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
  colors[ImGuiCol_HeaderHovered] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
  colors[ImGuiCol_HeaderActive] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
  colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
  colors[ImGuiCol_SeparatorHovered] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
  colors[ImGuiCol_SeparatorActive] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
  colors[ImGuiCol_ResizeGrip] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
  colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
  colors[ImGuiCol_ResizeGripActive] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
  colors[ImGuiCol_Tab] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
  colors[ImGuiCol_TabHovered] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
  colors[ImGuiCol_TabActive] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
  colors[ImGuiCol_TabUnfocused] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
  colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
  colors[ImGuiCol_PlotLines] = ImVec4(0.86f, 0.93f, 0.89f, 1.00f);
  colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.86f, 0.93f, 0.89f, 1.00f);
  colors[ImGuiCol_PlotHistogram] = ImVec4(0.86f, 0.93f, 0.89f, 1.00f);
  colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.86f, 0.93f, 0.89f, 1.00f);
  colors[ImGuiCol_TextSelectedBg] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
  colors[ImGuiCol_DragDropTarget] = ImVec4(0.86f, 0.93f, 0.89f, 1.00f);
  colors[ImGuiCol_NavHighlight] = ImVec4(0.86f, 0.93f, 0.89f, 1.00f);
  colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.86f, 0.93f, 0.89f, 1.00f);
  colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
  colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);

  style.WindowRounding = 0.0f;
  style.FrameRounding = 0.0f;
  style.GrabRounding = 0.0f;
  style.ScrollbarRounding = 0.0f;
  style.TabRounding = 0.0f;
}

// Draw the UI.
void UiState::draw_ui(KhelState* state) {
  ImGui_ImplSDLRenderer2_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();
  {
    int imgui_window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    ImGui::Begin("Khel", NULL, imgui_window_flags);
    ImGui::SetWindowPos(ImVec2(0.0, 0.0));
    ImGui::SetWindowSize(ImVec2(800.0, 600.0));
    if (state->chart_wrapper->chart_status == ChartStatus::PREVIEWING) {
      draw_ui_previewing(state);
    }
    if (state->chart_wrapper->chart_status == ChartStatus::PLAYING) {
      draw_ui_playing(state);
    }
    ImGui::End();
  }
}
// Draw the UI for when the current chart is previewing.
void UiState::draw_ui_previewing(KhelState* state) {
  // detect change to chart
  if (state->charts[charts_listbox_index] != chart) {
    if (chart != nullptr) {
      chart->audio->fade_out();
    }
    chart = state->charts[charts_listbox_index];
    difficulties = chart->difficulties->names();
    difficulties_listbox_index = 0;
  }
  // detect change to difficulty
  if (difficulties[difficulties_listbox_index] != difficulty) {
    difficulty = difficulties[difficulties_listbox_index];
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
  ImGui::PushItemWidth(200.0);
  ImGui::ListBox(
    "##Chart",
    &charts_listbox_index,
    string_vector_getter,
    state->chart_names.data(),
    (int) state->chart_names.size()
  );
  ImGui::ListBox(
    "##Difficulty",
    &difficulties_listbox_index,
    string_vector_getter,
    difficulties.data(),
    (int) difficulties.size()
  );
  ImGui::SliderInt("AV", &state->av->value, 100, 500);
  ImGui::SliderInt("Offset", &state->offset, -100, 100);
  ImGui::PopItemWidth();
  if (ImGui::Button("Play")) {
    chart->audio->stop();
    state->chart_wrapper->load_chart(chart);
    state->chart_wrapper->play_chart(difficulty, state->renderer, state->objects, state->groups);
    state->chart_wrapper->start_time = state->now();
    // TODO: include hold ticks in this calculation
    // max_score_per_object = ceil(1000000.0 / (double) state->groups->get_group("hits_and_holds")->size());
    // create objects
    state->objects->create_instance("assets/line_white.png", 0.0, 120.0, 100, 1, state->renderer);
    for (int i = 0; i < 10; i++) {
      state->objects->create_instance("assets/circle_gray.png", ((40 * i) - 180) + 400, 104.0, 32, 32, state->renderer);
    }
  }
}
// Draw the UI for when the current chart is playing.
void UiState::draw_ui_playing(KhelState* state) {
  auto window_width = 800.0;
  double one_minute = 60.0;
  Bpm* bpm_at_zero = state->chart_wrapper->chart->metadata->bpms->at_exact_time(0.0);
  double one_beat_at_zero = one_minute / bpm_at_zero->value;
  double start_time_seconds = as_seconds(state->chart_wrapper->start_time);
  double now_seconds = as_seconds(state->now());
  double exact_time_seconds = now_seconds - start_time_seconds - (one_beat_at_zero * 8.0);
  Bpm* bpm_now = state->chart_wrapper->chart->metadata->bpms->at_exact_time(exact_time_seconds);
  string bpm_text = format("{:.2f}", bpm_now->value);
  auto bpm_text_width = ImGui::CalcTextSize(bpm_text.c_str()).x;
  string score_text = format("{}", state->score);
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

// This function can be pased to ImGui::ListBox in order to use a vector<string> with it.
// See https://github.com/ocornut/imgui/issues/911.
bool string_vector_getter(void* data, int n, const char** out_text) {
  string* strings = (string*) data;
  string& current_string = strings[n];
  *out_text = current_string.c_str();
  return true;
}