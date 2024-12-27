.DEFAULT_GOAL = all

CXX = g++
CXXFLAGS = -g -Wall -Wextra -Werror -std=c++20
LDFLAGS = $$(pkg-config --cflags --libs sdl2 sdl2_image sdl2_mixer)

SRCS = src/*.cpp src/imgui/*.cpp

khel:
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(SRCS) -o khel

all: khel
	./khel