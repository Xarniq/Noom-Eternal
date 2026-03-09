CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Iinclude

SRC_DIR        = src
UI_SCREENS_DIR = $(SRC_DIR)/ui_screens
BUILD_DIR      = build
ASSETS_DIR     = assets
CONFIG_DIR     = config
TARGET         = $(BUILD_DIR)/noometernal-client

# Use vpath to search for source files
vpath %.c $(SRC_DIR):$(UI_SCREENS_DIR)

SOURCES  = $(notdir $(wildcard $(SRC_DIR)/*.c)) \
           $(notdir $(wildcard $(UI_SCREENS_DIR)/*.c))
OBJECTS  = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SOURCES))

# ---------- libraries in ../lib/ ----------
LIB_DIR = lib

# --- raylib as submodule ---
RAYLIB_DIR        = $(LIB_DIR)/raylib
RAYLIB_SRC_DIR    = $(RAYLIB_DIR)/src
RAYLIB_INCLUDE_DIR= $(RAYLIB_DIR)/include
RAYLIB_LIB        = $(RAYLIB_SRC_DIR)/libraylib.a

# --- raylib-media built via its own CMake project ---
RAYMEDIA_DIR        = $(LIB_DIR)/raylib-media
RAYMEDIA_BUILD_DIR  = $(RAYMEDIA_DIR)/build
RAYMEDIA_BUILD_TYPE ?= Release
RAYMEDIA_LIB        = $(RAYMEDIA_BUILD_DIR)/libraymedia.a

# --- EMAP library ---
EMAP_DIR = $(LIB_DIR)/emap-lib
EMAP_BUILD_DIR = $(EMAP_DIR)/build
EMAP_INCLUDE_DIR = $(EMAP_DIR)/include
EMAP_LIB = $(EMAP_BUILD_DIR)/libemap.a

# all libs we build in ../lib/
LIBS = $(RAYLIB_LIB) $(RAYMEDIA_LIB) $(EMAP_LIB)

# ---------- FFmpeg from system (for final link) ----------
FFMPEG_PKGS    = libavcodec libavformat libavutil libswresample libswscale
FFMPEG_LIBS   := $(shell pkg-config --libs $(FFMPEG_PKGS) 2>/dev/null)
FFMPEG_CFLAGS := $(shell pkg-config --cflags $(FFMPEG_PKGS) 2>/dev/null)

FFMPEG_CHECK := $(shell pkg-config --exists $(FFMPEG_PKGS) && echo 1 || echo 0)

ifeq ($(FFMPEG_CHECK),0)
    $(warning ================================================)
    $(warning WARNING: FFmpeg libraries not found!)
    $(warning Please install FFmpeg development packages:)
    $(warning )
    $(warning   Linux (Ubuntu/Debian):)
    $(warning     sudo apt-get install libavcodec-dev libavformat-dev libavutil-dev libswresample-dev libswscale-dev)
    $(warning )
    $(warning   Linux (Fedora/RHEL):)
    $(warning     sudo dnf install ffmpeg-devel)
    $(warning )
    $(warning   macOS (Homebrew):)
    $(warning     brew install ffmpeg)
    $(warning )
    $(warning ================================================)
endif

CFLAGS += -I$(RAYLIB_SRC_DIR) -I$(RAYMEDIA_DIR)/src -I$(EMAP_INCLUDE_DIR) \
          -DPLATFORM_DESKTOP -DGRAPHICS_API_OPENGL_33 $(FFMPEG_CFLAGS) \
          -L$(EMAP_BUILD_DIR)

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Linux)
	RAYLIB_SYS_LIBS = -lGL -lm -lpthread -ldl -lrt -lX11
endif

ifeq ($(UNAME_S),Darwin)
	RAYLIB_SYS_LIBS = \
		-framework CoreVideo \
		-framework IOKit \
		-framework Cocoa \
		-framework OpenGL \
		-framework AudioToolbox \
		-framework CoreAudio
endif

RAYMEDIA_SYS_LIBS = $(FFMPEG_LIBS)

# ---------- Default target ----------
all: $(TARGET)

# ---------- Build all libs in ../lib/ ----------
build-lib: $(LIBS)

# raylib via its own Makefile
$(RAYLIB_LIB):
	$(MAKE) -C $(RAYLIB_SRC_DIR) PLATFORM=PLATFORM_DESKTOP

# raylib-media via its own CMakeLists.txt, using the *already built* raylib
$(RAYMEDIA_LIB): $(RAYLIB_LIB)
	@mkdir -p "$(RAYMEDIA_BUILD_DIR)"
	cmake -S "$(RAYMEDIA_DIR)" -B "$(RAYMEDIA_BUILD_DIR)" \
	      -DCMAKE_BUILD_TYPE="$(RAYMEDIA_BUILD_TYPE)" \
	      -DRAYLIB_INCLUDE_DIR="$(abspath $(RAYLIB_SRC_DIR))" \
	      -DRAYLIB_LIBRARY_DIR="$(abspath $(dir $(RAYLIB_LIB)))" \
	      -DRAYLIB_LIBRARY_RELEASE="$(abspath $(RAYLIB_LIB))" \
	      -DRAYLIB_LIBRARY_DEBUG="$(abspath $(RAYLIB_LIB))"
	cmake --build "$(RAYMEDIA_BUILD_DIR)" --config "$(RAYMEDIA_BUILD_TYPE)"

# emap library built via its own Makefile
$(EMAP_LIB):
	$(MAKE) -C $(EMAP_DIR)

# ---------- Build project ----------
$(TARGET): build-lib $(OBJECTS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(OBJECTS) $(LIBS) -lemap -L$(EMAP_BUILD_DIR) $(RAYLIB_SYS_LIBS) $(RAYMEDIA_SYS_LIBS) -o $(TARGET)
	@echo "Build complete: $(TARGET)"

# ---------- Compile .c → .o in build/ ----------
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# ---------- CLEAN ----------
clean:
	rm -rf $(BUILD_DIR)

clean-lib:
	$(MAKE) -C $(RAYLIB_SRC_DIR) clean || true
	rm -rf $(RAYMEDIA_BUILD_DIR)
	$(MAKE) -C $(EMAP_DIR) clean || true

clean-all: clean clean-lib

# ---------- RUN ----------
run: $(TARGET)
	./$(TARGET)

doc:
	doxygen Doxyfile
	@echo "Documentation generated in docs/html/index.html"

.PHONY: all build-all build-lib clean clean-lib clean-all run doc
