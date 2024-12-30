#include <map>
#include <vector>
#include <SDL.h>
#include "chart.h"
#include "input.h"

using namespace std;

const map<char, int> map_scancodes = {
  {'q', SDL_SCANCODE_Q}, {'a', SDL_SCANCODE_A}, {'z', SDL_SCANCODE_Z},
  {'w', SDL_SCANCODE_W}, {'s', SDL_SCANCODE_S}, {'x', SDL_SCANCODE_X},
  {'e', SDL_SCANCODE_E}, {'d', SDL_SCANCODE_D}, {'c', SDL_SCANCODE_C},
  {'r', SDL_SCANCODE_R}, {'f', SDL_SCANCODE_F}, {'v', SDL_SCANCODE_V},
  {'t', SDL_SCANCODE_T}, {'g', SDL_SCANCODE_G}, {'b', SDL_SCANCODE_B},
  {'y', SDL_SCANCODE_Y}, {'h', SDL_SCANCODE_H}, {'n', SDL_SCANCODE_N},
  {'u', SDL_SCANCODE_U}, {'j', SDL_SCANCODE_J}, {'m', SDL_SCANCODE_M},
  {'i', SDL_SCANCODE_I}, {'k', SDL_SCANCODE_K}, {',', SDL_SCANCODE_COMMA},
  {'o', SDL_SCANCODE_O}, {'l', SDL_SCANCODE_L}, {'.', SDL_SCANCODE_PERIOD},
  {'p', SDL_SCANCODE_P}, {';', SDL_SCANCODE_SEMICOLON}, {'/', SDL_SCANCODE_SLASH},
};

void try_hit(KhelState* state) {
  if (state->chart_wrapper->chart_status != ChartStatus::PLAYING) return;
  // TODO
}