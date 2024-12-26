#ifndef KHEL_UI_H
#define KHEL_UI_H

#include <string>
#include <unordered_map>
#include <vector>
#include <SDL.h>
#include <SDL_ttf.h>

struct Text {
  std::string text;
  TTF_Font* font;
  SDL_Color color;
  int x;
  int y;
  int w;
  int h;
  SDL_Rect* rect;
  SDL_Surface* surface;
  SDL_Texture* texture;
  Text(std::string text, TTF_Font* font, SDL_Color color, SDL_Renderer* renderer);
  ~Text();
  void set_position(int new_x, int new_y);
  void set_text(std::string new_text, SDL_Renderer* renderer);
  void center_x();
  // void draw(SDL_Renderer* renderer);
  void draw(SDL_Surface* screenSurface);
};

struct Ui {
  std::unordered_map<int, Text*> texts;
  int min_available_text_id;
  int add_text(std::string text, TTF_Font* font, SDL_Color color, SDL_Renderer* renderer);
  Text* get_text_instance(int id);
  // void draw_all_items(SDL_Renderer* renderer);
  void draw_all_items(SDL_Surface* screenSurface);
};

#endif