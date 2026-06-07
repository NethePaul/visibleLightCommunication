DIR:=$(patsubst $(abspath ..)/%,%,$(abspath .))
BIN:=bin/$(DIR)
CXXFLAGS:=-O3 -g -Wall -Werror --std=c++20 -Wno-comment $(shell pkg-config --cflags opencv4)
LINKS:=$(shell pkg-config --libs opencv4)
SRC:=$(wildcard *.cpp)
OBJ:=$(patsubst %.cpp,obj/%.o,$(SRC))
HEADER:=$(wildcard *.h)

default: folders $(BIN) compile_flags.txt

bin:
	mkdir bin

obj:
	mkdir obj

folders: | bin obj

compile_flags.txt:Makefile
	echo $(CXXFLAGS) | tr '[:blank:]' '\n' > compile_flags.txt

$(BIN):$(OBJ)
	$(CXX) $(CXXFLAGS) $(CFLAGS) $(LINKS) $(OBJ) -o $(BIN)

$(OBJ):obj/%.o:%.cpp
	$(CXX) $(CXXFLAGS) $(CFLAGS) $^ -c -o $@

.PHONY: clean play live help camera_config
clean:
	rm -r $(OBJ) $(BIN)

play: $(BIN)
	$(BIN)

live: $(BIN) camera_config
	$(BIN) /dev/video2

help: $(BIN)
	$(BIN) -h

camera_config:
	cameractrls -d /dev/video2 -c "zoom_absolute=100,fps=30,white_balance_automatic=0,white_balance_temperature=7500,brightness=128,saturation=255,sharpness=255,contrast=180,focus_automatic_continuous=0,focus_absolute=0,auto_exposure=manual_mode,exposure_time_absolute=320,exposure_dynamic_framerate=0"
