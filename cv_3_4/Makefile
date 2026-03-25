CXX = g++

BIN_PATH = ../mingw64/bin/
CXX = $(BIN_PATH)g++
AS = $(BIN_PATH)as

CXXFLAGS = -std=c++17 -O2 -mavx2 -I../include
INC_DIR = include
LIBS = -lfreeglut -lglew32 -lSOIL -lopengl32 -lglu32
LIB_DIR = ..\libs

# **Linker flags (ADD THIS)**
LDFLAGS = -L$(LIB_DIR)

TARGET = nbody
SRC = nbody.cpp nbody_student.s  # Added cv2_student.s to SRC
OBJ = build/nbody.o build/nbody_student.o  # Added cv2_student.o to OBJ

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS)  $(LIBS)

# Rule to compile C++ source files (.cpp)
build/nbody.o: nbody.cpp
	$(CXX) $(CXXFLAGS) -g -c $< -o $@

# Rule to assemble assembly source files (.s)
build/nbody_student.o: nbody_student.s
	$(CXX) -g -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJ)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run