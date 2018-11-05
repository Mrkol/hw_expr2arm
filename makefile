CXX = arm-linux-gnueabi-g++
CXXFLAGS = -Wall -Wextra -Werror -ggdb -std=c++17

SRC_DIR = ./src
BIN_DIR = ./bin

REP_NAME = https://github.com/Mrkol/hw_expr2arm
AUTOGEN_MSG = // You are NOT supposed to look at this automatically generated file!!!\n// Please, refer to this repository instead: $(REP_NAME).

SOURCES = $(SRC_DIR)/jit.cpp

main: init $(SRC_DIR)/main.cpp $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SRC_DIR)/main.cpp $(SOURCES) -o $(BIN_DIR)/main

test: init $(SRC_DIR)/test.cpp $(SRC_DIR)/jit.cpp
	$(CXX) $(CXXFLAGS) $(SRC_DIR)/test.cpp $(SOURCES) -o $(BIN_DIR)/test

singlefile: init
	echo "$(AUTOGEN_MSG)" > $(BIN_DIR)/main.cpp
	cat $(SRC_DIR)/jit.hpp $(SRC_DIR)/jit.cpp\
		 >> $(BIN_DIR)/main.cpp

	sed -i '/#include "/d' $(BIN_DIR)/main.cpp
	sed -i '/#pragma once/d' $(BIN_DIR)/main.cpp
	sed -i '/#ifndef/d' $(BIN_DIR)/main.cpp
	sed -i '/#define/d' $(BIN_DIR)/main.cpp
	sed -i '/#endif/d' $(BIN_DIR)/main.cpp



init:
	mkdir -p $(BIN_DIR)