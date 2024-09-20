SHELL = pwsh.exe
.SHELLFLAGS = -NoProfile -Command

BIN_DIR = ./bin
BUILD_DIR = ./build
SRC_DIR = ./src

TARGET = $(BIN_DIR)/client.exe $(BIN_DIR)/server.exe
DEBUG_TARGET = $(BIN_DIR)/client_debug.exe $(BIN_DIR)/server_debug.exe

CC = g++
STD = c++20
CFLAGS = -O2
LIBS = -lws2_32

# source files
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
# object files
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRCS))
DEPS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.d, $(SRCS))

.PHONY: all clean tes run debug
all: $(TARGET)

$(BUILD_DIR)/%.d: $(SRC_DIR)/%.cpp
	@if (!(Test-Path $(BUILD_DIR))) { New-Item -ItemType Directory -Path $(BUILD_DIR) }
	$$str = $(CC) -MM $<; $$str -replace '^(\w*)(\.o:.*)', '$$1.d $$1$$2' | Out-File -FilePath $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@if (!(Test-Path $(BIN_DIR))) { New-Item -ItemType Directory -Path $(BIN_DIR) }
	$(CC) -std=$(STD) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	@if (!(Test-Path $(BIN_DIR))) { New-Item -ItemType Directory -Path $(BIN_DIR) }
	$(CC) -std=$(STD) $(CFLAGS) $^ -o $@ $(LIBS)

clean:
	rm -r -fo $(BUILD_DIR)/*

run: $(TARGET)
	$(TARGET)

debug: $(DEBUG_TARGET)
	gdb $(DEBUG_TARGET)

$(DEBUG_TARGET): $(SRCS)
	@if (!(Test-Path $(BIN_DIR))) { New-Item -ItemType Directory -Path $(BIN_DIR) }
	$(CC) -std=$(STD) -Og -g $^ -o $@ $(LIBS)

test:
	@echo "$(SHELL)"
	@echo "$(SRCS)"
	@echo "$(OBJS)"
	@echo "$(DEPS)"

include $(DEPS)