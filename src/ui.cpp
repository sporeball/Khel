#include <string>
#include <SDL.h>
#include "ui.h"

using namespace std;

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

  // ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
}