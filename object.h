#ifndef KHEL_OBJECT_H
#define KHEL_OBJECT_H

#include <string>
#include <unordered_map>
#include <vector>

struct Instance {
  SDL_Rect* rect;
  Instance(int x, int y, int w, int h);
  ~Instance();
  void move(int x, int y, int w, int h);
  void draw_instance(SDL_Surface* surface, SDL_Surface* screenSurface);
  void destroy_instance();
};

struct Object {
  SDL_Surface* surface;
  SDL_Texture* texture;
  std::vector<Instance*> instances;
  Object(const char* filename, SDL_Renderer* renderer);
  ~Object();
  Instance* instantiate(int x, int y, int w, int h);
  void draw_all_instances(SDL_Surface* screenSurface);
};

struct Objects {
  std::unordered_map<std::string, Object*> objects;
  ~Objects();
  Object* create_object(const char *filename, SDL_Renderer* renderer);
  void draw_all_objects(SDL_Surface* screenSurface);
};

struct Group {
  std::vector<Instance*> instances;
  ~Group();
  void insert(Instance* instance);
  void remove(Instance* instance);
};

struct Groups {
  std::unordered_map<std::string, Group*> groups;
  ~Groups();
  Group* get_group(std::string name);
  void create_group(std::string name);
  void insert_into_group(std::string name, Instance* ptr);
  void remove_from_group(std::string name, Instance* ptr);
  void remove_from_all_groups(Instance* ptr);
};

#endif