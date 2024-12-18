// macos: `g++ khel.cpp -o khel $(pkg-config --cflags --libs sdl2 sdl2_image) -std=c++11 -Wall && ./khel`
// (install SDL2 and SDL2_image using MacPorts)

#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>
#include <iostream>
#include "object.h"

using namespace std;

int main(int argc, char* argv[]) {
  SDL_Window* window = NULL;
  SDL_Renderer* renderer = NULL;
  SDL_Surface* screenSurface = NULL;

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("Could not initialize SDL!: %s\n", SDL_GetError());
  }

  window = SDL_CreateWindow("Khel", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_SHOWN);
  if (window == NULL) {
    printf("Could not create window!: %s\n", SDL_GetError());
  }

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE | SDL_RENDERER_PRESENTVSYNC);
  if (renderer == NULL) {
    printf("Could not create renderer!: %s\n", SDL_GetError());
  }

  int imgFlags = IMG_INIT_PNG;
  if (!(IMG_Init(imgFlags) & imgFlags)) {
    printf("Could not initialize SDL_image!: %s\n", SDL_GetError());
  }

  screenSurface = SDL_GetWindowSurface(window);

  Groups groups;
  groups.create_group("pure_calculation");
  groups.create_group("hit_objects");
  groups.create_group("hits_and_holds");
  groups.create_group("timing_lines");

  Objects objects;
  // Object* circle_red = objects.create_object("assets/circle_red.png", renderer);
  // Instance* i0 = circle_red->instantiate(100, 100, 32, 32);
  // Instance* i1 = circle_red->instantiate(150, 100, 32, 32);
  // Instance* i2 = circle_red->instantiate(200, 100, 32, 32);

  SDL_Event e;
  int quit = 0;
  while (quit == 0) {
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) {
        quit = 1;
      }
    }
    SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 0x00, 0x00, 0x00));
    objects.draw_all_objects(screenSurface);
    SDL_UpdateWindowSurface(window);
  }

  // delete objects;

  SDL_FreeSurface(screenSurface);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  IMG_Quit();
  SDL_Quit();
  
  return 0;
}