#include <iostream>
#include <vector>
#include <unordered_map>
#include <SDL.h>
#include <SDL_image.h>
#include "object.h"

using namespace std;

// Constructor method.
Instance::Instance(int x, int y, int w, int h) {
  rect = new SDL_Rect;
  move(x, y, w, h);
}
// Destructor method.
Instance::~Instance() {
  delete rect;
}
// Move this instance.
void Instance::move(int x, int y, int w, int h) {
  rect->x = x;
  rect->y = y;
  rect->w = w;
  rect->h = h;
}
// Draw this instance.
void Instance::draw_instance(SDL_Surface* surface, SDL_Surface* screenSurface) {
  SDL_BlitSurface(surface, NULL, screenSurface, rect);
}
// Destroy this instance.
void Instance::destroy_instance() {
  this->~Instance();
}

// Constructor method.
Object::Object(const char* filename, SDL_Renderer* renderer) {
  surface = IMG_Load(filename);
  texture = SDL_CreateTextureFromSurface(renderer, surface);
}
// Destructor method.
Object::~Object() {
  SDL_FreeSurface(surface);
  SDL_DestroyTexture(texture);
  for (Instance* ptr : instances) {
    delete ptr;
  }
}
// Instantiate this object at the given coordinates.
Instance* Object::instantiate(int x, int y, int w, int h) {
  Instance* ptr = new Instance(x, y, w, h);
  instances.push_back(ptr);
  return ptr;
}
// Draw all instances of this object.
void Object::draw_all_instances(SDL_Surface* screenSurface) {
  for (Instance* ptr : instances) {
    ptr->draw_instance(surface, screenSurface);
  }
}

// Destructor method.
Objects::~Objects() {
  objects.clear();
}
// Create a type of object.
Object* Objects::create_object(const char* filename, SDL_Renderer* renderer) {
  Object* object = new Object(filename, renderer);
  pair<string, Object*> p(filename, object);
  objects.insert(p);
  return object;
}
// Draw all instances of every type of object.
void Objects::draw_all_objects(SDL_Surface* screenSurface) {
  for (auto object : objects) {
    object.second->draw_all_instances(screenSurface);
  }
}

// Destructor method.
Group::~Group() {
  for (Instance* ptr : instances) {
    delete ptr;
  }
}
// Add the given instance to this group.
void Group::insert(Instance* instance) {
  instances.push_back(instance);
}
// Remove the given instance from this group.
void Group::remove(Instance* instance) {
  auto it = find(instances.begin(), instances.end(), instance);
  if (it != instances.end()) {
    delete *it;
  }
  instances.erase(it);
}

// Destructor method.
Groups::~Groups() {
  groups.clear();
}
// Get the group with the given name.
Group* Groups::get_group(string name) {
  return groups[name];
}
// Create a new group.
void Groups::create_group(string name) {
  Group* group = new Group();
  pair<string, Group*> p(name, group);
  groups.insert(p);
}
// Add the given object instance to the group with the given name.
void Groups::insert_into_group(string name, Instance* ptr) {
  groups[name]->insert(ptr);
}
// Remove the given object instance from the group with the given name.
void Groups::remove_from_group(string name, Instance* ptr) {
  groups[name]->remove(ptr);
}
// Remove the given object instance from every group.
void Groups::remove_from_all_groups(Instance* ptr) {
  for (auto group : groups) {
    group.second->remove(ptr);
  }
}