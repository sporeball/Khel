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
// Move this instance to the given coordinates.
void Instance::move(double new_x, double new_y) {
  x = new_x;
  y = new_y;
  rect->x = (int) x;
  rect->y = (int) y;
}
// Draw this instance.
void Instance::draw_instance(SDL_Renderer* renderer, SDL_Texture* texture) {
  SDL_RenderCopy(renderer, texture, NULL, rect);
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
// Get a pointer to the instance with the given ID, if it is an instance of this object.
Instance* Object::get_instance(int id) {
  for (auto instance : instances) {
    if (instance.first == id) {
      return instance.second;
    }
  }
  return nullptr;
}
// Draw all instances of this object.
void Object::draw_all_instances(SDL_Renderer* renderer) {
  for (auto& it : instances) {
    it.second->draw_instance(renderer, texture);
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
// Create a type of object and return a pointer to it.
Object* Objects::create_object(string filename, SDL_Renderer* renderer) {
  Object* object = new Object(filename, renderer);
  pair<string, Object*> p(filename, object);
  objects.insert(p);
  return object;
}
// Get a pointer to the object which holds an instance with the given ID.
Object* Objects::get_object(int id) {
  for (auto object : objects) {
    for (auto instance : object.second->instances) {
      if (instance.first == id) {
        return object.second;
      }
    }
  }
  return nullptr;
}
// Create an object instance at the given coordinates and return its ID.
// Creates the object if it does not exist.
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
    for (auto it = object.second->instances.begin(); it != object.second->instances.end(); ) {
      if (it->first == id) {
        it->second->~Instance();
        it = object.second->instances.erase(it);
      } else {
        ++it;
      }
    }
  }
}
// Draw all instances of every object.
void Objects::draw_all_objects(SDL_Renderer* renderer) {
  for (auto object : objects) {
    object.second->draw_all_instances(renderer);
  }
}

// Destructor method.
Group::~Group() {
  instances.clear();
}
// Add the object instance with the given ID to this group.
void Group::insert(int id) {
  instances.push_back(id);
}
// Remove the object instance with the given ID from this group.
void Group::remove(int id) {
  erase(instances, id);
}
// Return the size of this group.
int Group::size() {
  return instances.size();
}

// Destructor method.
Groups::~Groups() {
  groups.clear();
}
// Create a new group and return a pointer to it.
Group* Groups::create_group(string name) {
  Group* group = new Group();
  pair<string, Group*> p(name, group);
  groups.insert(p);
  return group;
}
// Get a pointer to the group with the given name.
Group* Groups::get_group(string name) {
  for (auto group : groups) {
    if (group.first == name) {
      return group.second;
    }
  }
  return nullptr;
}
// Add the object instance with the given ID to the group with the given name.
// Creates the group if it does not exist.
void Groups::insert_into_group(string name, int id) {
  unordered_map<string, Group*>::const_iterator got = groups.find(name);
  Group* group;
  if (got == groups.end()) {
    group = create_group(name);
  } else {
    group = got->second;
  }
  group->insert(id);
}
// Remove the object instance with the given ID from the group with the given name.
void Groups::remove_from_group(string name, int id) {
  groups[name]->remove(id);
}
// Remove the object instance with the given ID from every group.
void Groups::remove_from_all_groups(int id) {
  for (auto group : groups) {
    group.second->remove(id);
  }
}