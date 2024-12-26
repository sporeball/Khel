#ifndef KHEL_UI_H
#define KHEL_UI_H

#include <string>
#include <unordered_map>
#include <vector>
#include <SDL.h>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"
#include "imgui/imgui_impl_sdlrenderer2.h"
#include "imgui/imgui_stdlib.h"

void init_imgui(SDL_Window* window, SDL_Renderer* renderer);

#endif