#ifndef KHEL_OBJECT_H
#define KHEL_OBJECT_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// An instance of an object.
// Create an instance using `Objects::create_instance()`.
struct Instance {
  double x;
  double y;
  SDL_Rect* rect;
  Instance(double x, double y, int w, int h);
  ~Instance();
  void move(double new_x, double new_y);
  void draw_instance(SDL_Renderer* renderer, SDL_Texture* texture);
};

// A type of object.
// Create an object using `Objects::create_object()` or `Objects::create_instance()`.
struct Object {
  SDL_Surface* surface;
  SDL_Texture* texture;
  std::unordered_map<int, Instance*> instances;
  Object(std::string filename, SDL_Renderer* renderer);
  ~Object();
  Instance* get_instance(int id);
  void destroy(Instance* instance);
  void draw_all_instances(SDL_Renderer* renderer);
};

// A collection of all types of objects created by Khel.
struct Objects {
  std::unordered_map<std::string, Object*> objects;
  int min_available_id;
  Objects();
  ~Objects();
  Object* create_object(std::string filename, SDL_Renderer* renderer);
  Object* get_object(int id);
  int create_instance(std::string filename, double x, double y, int w, int h, SDL_Renderer* renderer);
  Instance* get_instance(int id);
  void destroy_instance(int id);
  void draw_all_objects(SDL_Renderer* renderer);
};

// A collection of object instances, able to be manipulated together.
// Create a group using `Groups::create_group()` or `Groups::insert_into_group()`.
// Use `for (int i = 0; i < group->size(); i++)` or `for (auto id : group->instances)` to iterate over the group.
struct Group {
  std::vector<int> instances;
  ~Group();
  void insert(int id);
  void remove(int id);
  int size();
};

// A collection of all groups created by Khel.
struct Groups {
  std::unordered_map<std::string, Group*> groups;
  ~Groups();
  Group* create_group(std::string name);
  Group* get_group(std::string name);
  void insert_into_group(std::string name, int id);
  void remove_from_group(std::string name, int id);
  void remove_from_all_groups(int id);
};

#endif