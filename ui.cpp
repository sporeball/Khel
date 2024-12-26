#include <string>
#include <SDL.h>
#include <SDL_ttf.h>
#include "ui.h"

using namespace std;

// // Constructor method.
// Text::Text(string text, TTF_Font* font, SDL_Color color, SDL_Renderer* renderer) :
//   text(text), font(font), color(color)
// {
//   TTF_SizeText(font, text.c_str(), &w, &h);
//   surface = TTF_RenderText_Solid(font, text.c_str(), color);
//   // texture = SDL_CreateTextureFromSurface(renderer, surface);
//   rect = new SDL_Rect;
//   // rect->x = x;
//   // rect->y = y;
//   rect->w = w;
//   rect->h = h;
// }
// // Destructor method.
// Text::~Text() {
//   SDL_FreeSurface(surface);
//   // SDL_DestroyTexture(texture);
// }
// // Update the position of this Text instance.
// void Text::set_position(int new_x, int new_y) {
//   rect->x = new_x;
//   rect->y = new_y;
// }
// // Update the x position of this Text instance.
// void Text::set_x_position(int new_x) {
//   rect->x = new_x;
// }
// // Update the y position of this Text instance.
// void Text::set_y_position(int new_y) {
//   rect->y = new_y;
// }
// // Update the text of this Text instance.
// void Text::set_text(string new_text, SDL_Renderer* renderer) {
//   text = new_text;
//   TTF_SizeText(font, text.c_str(), &w, &h);
//   surface = TTF_RenderText_Solid(font, text.c_str(), color);
//   // texture = SDL_CreateTextureFromSurface(renderer, surface);
//   rect->w = w;
//   rect->h = h;
// }
// // Center this Text instance.
// void Text::center_x() {
//   rect->x = 400 - (rect->w / 2);
// }
// // Draw this Text instance.
// // void Text::draw(SDL_Renderer* renderer) {
// void Text::draw(SDL_Surface* screenSurface) {
//   // SDL_RenderCopy(renderer, texture, NULL, rect);
//   SDL_BlitSurface(surface, NULL, screenSurface, rect);
// }

// // Create a new Text instance.
// // Returns the ID of the Text instance.
// int Ui::add_text(std::string text, TTF_Font* font, SDL_Color color, SDL_Renderer* renderer) {
//   Text* t = new Text(text, font, color, renderer);
//   pair<int, Text*> p(min_available_text_id, t);
//   texts.insert(p);
//   return min_available_text_id++;
// }
// // Get the Text instance with the given ID.
// Text* Ui::get_text_instance(int id) {
//   for (auto instance : texts) {
//     if (instance.first == id) {
//       return instance.second;
//     }
//   }
//   return nullptr;
// }
// // Draw all UI items.
// void Ui::draw_all_items(SDL_Surface* screenSurface) {
//   for (auto text : texts) {
//     text.second->draw(screenSurface);
//   }
// }

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