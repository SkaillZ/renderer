appname := app

CXX := g++
CXXFLAGS := -Wall -g -std=c++17 -O3 -I lib
LINKFLAGS := /usr/local/lib/libglfw.dylib /usr/local/lib/libvulkan.dylib /usr/local/lib/libassimp.dylib

srcfiles      := $(shell find ./src -maxdepth 1 -name "*.cpp")
objects       := $(patsubst %.cpp, %.o, $(srcfiles))
shaderfiles   := $(shell find ./src/shaders -maxdepth 1 -name "*.vert" -o -name "*.frag")
shaderoutputs := $(shaderfiles:=.spv)

all: $(appname)

$(appname): $(objects)
	$(CXX) $(CXXFLAGS) -o $(appname) $(objects) $(LINKFLAGS)

depend: .depend

.depend: $(srcfiles)
	rm -f ./.depend
	$(CXX) $(CXXFLAGS) -MM $^>>./.depend;

clean:
	rm -f $(appname)
	rm -f $(objects)
	rm -f $(shell find ./src/shaders -maxdepth 1 -name "*.spv")

include .depend
