# --- PATHS ---
# Assuming mingw64 is up one level as per your other project
BIN_PATH = ../mingw64/bin/
CXX = $(BIN_PATH)g++
CC = $(BIN_PATH)gcc

# --- FLAGS ---

# Flags for the Main Application
# Includes ../include for FreeGLUT headers
CXXFLAGS_APP = -std=c++11 -g -I../include
# Links ../lib for FreeGLUT/OpenGL libraries
LDFLAGS_APP = -L../libs -lfreeglut -lopengl32 -lglu32

# Flags for the DLL
# -masm=intel is crucial for your assembly file syntax
ASMFLAGS = -masm=intel -g
CXXFLAGS_DLL = -g
LDFLAGS_DLL = -shared

# --- TARGETS ---
EXE_TARGET = cv1.exe
DLL_TARGET = cv1.dll
BUILD_DIR = build

# Objects required for the DLL
DLL_OBJS = $(BUILD_DIR)/cv1_asm_dll.o $(BUILD_DIR)/cv1_cpp_dll.o

# --- RULES ---

all: build_dir $(DLL_TARGET) $(EXE_TARGET)

# Create build directory if it doesn't exist
build_dir:
	@if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)

# 1. Link the Main Executable
$(EXE_TARGET): cv1_main.cpp cv1.h
	$(CXX) $(CXXFLAGS_APP) -o $@ $< $(LDFLAGS_APP)

# 2. Link the DLL
# Combines the ASM object and the CPP object into a shared library
$(DLL_TARGET): $(DLL_OBJS)
	$(CC) $(LDFLAGS_DLL) -o $@ $^

# 3. Compile Assembly part (Intel Syntax)
$(BUILD_DIR)/cv1_asm_dll.o: cv1_student.s
	$(CC) $(ASMFLAGS) -c $< -o $@

# 4. Compile C++ DLL part (dllmain.cpp)
# Note: Ensure dllmain.cpp exists in the folder (referenced in your tasks.json)
$(BUILD_DIR)/cv1_cpp_dll.o: dllmain.cpp
	$(CC) $(CXXFLAGS_DLL) -c $< -o $@

# --- CLEAN ---
clean:
	if exist $(BUILD_DIR) rmdir /s /q $(BUILD_DIR)
	if exist $(EXE_TARGET) del $(EXE_TARGET)
	if exist $(DLL_TARGET) del $(DLL_TARGET)

# --- RUN ---
run: all
	./$(EXE_TARGET)

.PHONY: all clean run build_dir