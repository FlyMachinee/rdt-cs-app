SHELL = pwsh.exe
.SHELLFLAGS = -NoProfile -Command

BIN_DIR = ./bin
BUILD_DIR = ./build
SRC_DIR = ./src

TARGET = $(BIN_DIR)/client_test.exe $(BIN_DIR)/server_test.exe
DEBUG_TARGET = $(BIN_DIR)/test.exe

CC = g++
STD = c++20
CFLAGS = -O2
LIBS = -lws2_32

# source files
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
# object files
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRCS))

.PHONY: all clean debug
all: $(TARGET)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@if (!(Test-Path $(BIN_DIR))) { New-Item -ItemType Directory -Path $(BIN_DIR) }
	$(CC) -std=$(STD) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/%.exe: $(BUILD_DIR)/%.o $(BUILD_DIR)/UDPDataframe.o $(BUILD_DIR)/UDPFileReader.o $(BUILD_DIR)/UDPFileWriter.o $(BUILD_DIR)/wsa_wapper.o $(BUILD_DIR)/BasicRole.o
	@if (!(Test-Path $(BIN_DIR))) { New-Item -ItemType Directory -Path $(BIN_DIR) }
	$(CC) -std=$(STD) $(CFLAGS) $^ -o $@ $(LIBS)

clean:
	rm -r -fo $(BUILD_DIR)/*

debug: $(DEBUG_TARGET)

$(DEBUG_TARGET): $(SRC_DIR)/test.cpp
	$(CC) -std=$(STD) -g -Og $^ -o $@ $(LIBS)