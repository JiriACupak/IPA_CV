# --- CESTY (RELATIVNÍ) ---
# Předpokládáme, že mingw64 je o úroveň výš nebo v toolchain složce
# Uprav BIN_PATH pokud je tvůj mingw64 jinde
BIN_PATH = ../mingw64/bin/
CXX = $(BIN_PATH)g++
AS = $(BIN_PATH)as

CXXFLAGS = -std=c++17 -O2 -mavx2 -g
# -fno-use-linker-plugin řeší chybu s liblto_plugin.dll
LDFLAGS = -fno-use-linker-plugin -L../mingw64/lib -lopencv_world4110

TARGET = cv2
SRC = cv2_main.cpp cv2_student.s
OBJ = build/cv2_main.o build/cv2_student.o

all: build_dir $(TARGET)

# Vytvoří složku build, pokud neexistuje
build_dir:
	@if not exist build mkdir build

$(TARGET): $(OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS)

# Kompilace C++
build/cv2_main.o: cv2_main.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS)

# Kompilace ASM (používáme g++ pro konzistenci s ladicími symboly)
build/cv2_student.o: cv2_student.s
	$(CXX) -c $< -o $@ -g

clean:
	if exist build rmdir /s /q build
	if exist $(TARGET).exe del $(TARGET).exe

run: $(TARGET)
	./$(TARGET).exe

.PHONY: all clean run build_dir