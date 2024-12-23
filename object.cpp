#include <iostream>
#include <vector>
#include <unordered_map>
#include <SDL.h>
#include <SDL_image.h>
#include "object.h"

using namespace std;

// Constructor method.
Instance::Instance(double x, double y, int w, int h) {
  rect = new SDL_Rect;
  rect->w = w;
  rect->h = h;
  move(x, y);
}
// Destructor method.
Instance::~Instance() {
  delete rect;
}
// Move this instance.
void Instance::move(double new_x, double new_y) {
  x = new_x;
  y = new_y;
  rect->x = (int) x;
  rect->y = (int) y;
  // printf("moved instance to (%f, %f) [rect: (%d, %d)]\n", x, y, rect->x, rect->y);
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
Object::Object(string filename, SDL_Renderer* renderer) {
  surface = IMG_Load(filename.c_str());
  texture = SDL_CreateTextureFromSurface(renderer, surface);
}
// Destructor method.
Object::~Object() {
  SDL_FreeSurface(surface);
  SDL_DestroyTexture(texture);
  instances.clear();
}
Instance* Object::get_instance(int id) {
  return instances[id];
}
// Draw all instances of this object.
void Object::draw_all_instances(SDL_Surface* screenSurface) {
  for (auto& it : instances) {
    it.second->draw_instance(surface, screenSurface);
  }
}

// Constructor method.
Objects::Objects() {
  min_available_id = 0;
}
// Destructor method.
Objects::~Objects() {
  objects.clear();
}
// Create a type of object.
Object* Objects::create_object(string filename, SDL_Renderer* renderer) {
  Object* object = new Object(filename, renderer);
  pair<string, Object*> p(filename, object);
  objects.insert(p);
  return object;
}
// Get the type of object which holds an instance with the given ID.
Object* Objects::get_object(int id) {
  for (auto object : objects) {
    for (auto instance : object.second->instances) {
      if (instance.first == id) {
        return objects[object.first];
      }
    }
  }
  return nullptr;
}
// Create an object instance at the given coordinates.
// Returns the ID of the object instance.
int Objects::create_instance(string filename, double x, double y, int w, int h, SDL_Renderer* renderer) {
  unordered_map<string, Object*>::const_iterator got = objects.find(filename);
  Object* object;
  if (got == objects.end()) {
    object = create_object(filename, renderer);
  } else {
    object = got->second;
  }
  Instance* instance = new Instance(x, y, w, h);
  pair<int, Instance*> p(min_available_id, instance);
  object->instances.insert(p);
  return min_available_id++;
}
// Get the object instance with the given ID.
Instance* Objects::get_instance(int id) {
  for (auto object : objects) {
    for (auto instance : object.second->instances) {
      if (instance.first == id) {
        return instance.second;
      }
    }
  }
  return nullptr;
}
// Destroy the object instance with the given ID.
void Objects::destroy_instance(int id) {
  for (auto object : objects) {
    for (auto instance : object.second->instances) {
      if (instance.first == id) {
        instance.second->destroy_instance();
      }
    }
  }
}
// Draw all instances of every type of object.
void Objects::draw_all_objects(SDL_Surface* screenSurface) {
  for (auto object : objects) {
    object.second->draw_all_instances(screenSurface);
  }
}

// Destructor method.
Group::~Group() {
  instances.clear();
}
// Add the given instance to this group.
void Group::insert(int id) {
  instances.push_back(id);
}
// Remove the given instance from this group.
void Group::remove(int id) {
  instances.erase(std::remove(instances.begin(), instances.end(), id), instances.end());
}
// Return the size of this group.
int Group::size() {
  return instances.size();
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
void Groups::insert_into_group(string name, int id) {
  groups[name]->insert(id);
}
// Remove the given object instance from the group with the given name.
void Groups::remove_from_group(string name, int id) {
  groups[name]->remove(id);
}
// Remove the given object instance from every group.
void Groups::remove_from_all_groups(int id) {
  for (auto group : groups) {
    group.second->remove(id);
  }
}