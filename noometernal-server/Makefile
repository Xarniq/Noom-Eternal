#
# 'make'        build executable file 'main'
# 'make clean'  removes all .o and executable files
#

# define the C compiler to use
CC = gcc

# define any compile-time flags
CFLAGS	:= -Wall -Wextra -g

# define library paths in addition to /usr/lib
#   if I wanted to include libraries not in /usr/lib I'd specify
#   their path using -Lpath, something like:
LFLAGS =

# define output directory
OUTPUT	:= output

# define source directory
SRC		:= src

# define include directory
INCLUDE	:= include

# define lib directory
LIB		:= lib

# define build directory
BUILD_DIR	:= build

# Submodules to compile

# --- EMAP library ---
EMAP_DIR = $(LIB)/emap-lib
EMAP_BUILD_DIR = $(EMAP_DIR)/build
EMAP_LIB = $(EMAP_BUILD_DIR)/libemap.a

# Build wrappers.c locally to ensure helper symbols are available even if
# the submodule archive omits them
EMAP_WRAPPERS_SRC := $(EMAP_DIR)/src/wrappers.c
EMAP_WRAPPERS_OBJ := $(BUILD_DIR)/emap_wrappers.o

# Sources de la lib EMAP (hors wrappers.c déjà séparé)
EMAP_SRC := $(wildcard $(EMAP_DIR)/src/*.c)
EMAP_SRC_NO_WRAPPERS := $(filter-out $(EMAP_WRAPPERS_SRC),$(EMAP_SRC))
EMAP_OBJ := $(patsubst $(EMAP_DIR)/src/%.c,$(EMAP_BUILD_DIR)/%.o,$(EMAP_SRC_NO_WRAPPERS))

# (Restauration bloc OS manquant)
ifeq ($(OS),Windows_NT)
MAIN	:= noometernal-server.exe
SOURCEDIRS	:= $(SRC)
INCLUDEDIRS	:= $(INCLUDE)
LIBDIRS		:= $(LIB)
FIXPATH = $(subst /,\,$1)
RM			:= del /q /f
MD	:= mkdir
else
MAIN	:= noometernal-server
SOURCEDIRS	:= $(shell find $(SRC) -type d)
INCLUDEDIRS	:= $(shell find $(INCLUDE) -type d)
LIBDIRS		:= $(shell find $(LIB) -type d)
FIXPATH = $1
RM = rm -f
MD	:= mkdir -p
endif

# define any directories containing header files other than /usr/include
INCLUDES	:= $(patsubst %,-I%, $(INCLUDEDIRS:%/=%)) -I$(EMAP_DIR)/include -I$(EMAP_DIR)

# define the C lib search paths
LIBPATHS	:= $(patsubst %,-L%, $(LIBDIRS:%/=%)) -L$(EMAP_BUILD_DIR)

# define the C source files
SOURCES		:= $(wildcard $(patsubst %,%/*.c, $(SOURCEDIRS)))

# define the C object files 
OBJECTS		:= $(patsubst $(SRC)/%.c,$(BUILD_DIR)/%.o,$(SOURCES)) $(EMAP_WRAPPERS_OBJ)

# define the dependency output files
DEPS		:= $(OBJECTS:.o=.d)

# define target executable
TARGET		:= $(OUTPUT)/$(MAIN)

#
# The following part of the makefile is generic; it can be used to 
# build any executable just by changing the definitions above and by
# deleting dependencies appended to the file from 'make depend'
#

# ---------- Default target ----------
build: $(TARGET)

# ---------- Build all libs in lib/ ----------
build-lib: submodules $(EMAP_LIB)

submodules:
	-@git submodule update --init --recursive || true

# (Suppression de l’ancienne recette récursive de $(EMAP_LIB))
# $(EMAP_LIB):
#	$(MAKE) -C $(EMAP_DIR) build/libemap.a

# Construction directe de la bibliothèque statique sans récursion
$(EMAP_BUILD_DIR)/%.o: $(EMAP_DIR)/src/%.c
	@mkdir -p $(EMAP_BUILD_DIR)
	$(CC) $(CFLAGS) -I$(EMAP_DIR)/include -c $< -o $@

$(EMAP_LIB): $(EMAP_OBJ)
	@mkdir -p $(EMAP_BUILD_DIR)
	ar rcs $@ $(EMAP_OBJ)

# Compilation des sources de l'application (gère les sous-dossiers)
$(BUILD_DIR)/%.o: $(SRC)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Objet wrappers déjà listé dans OBJECTS
$(EMAP_WRAPPERS_OBJ): $(EMAP_WRAPPERS_SRC)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I$(EMAP_DIR)/include -c $< -o $@

# Lien de l'exécutable
$(TARGET): build-lib $(OBJECTS)
	@mkdir -p $(OUTPUT)
	$(CC) $(CFLAGS) $(INCLUDES) $(OBJECTS) $(LIBPATHS) -lemap -lpthread -o $(TARGET)
	@echo "Build complete: $(TARGET)"

# ---------- CLEAN ----------
clean:
	rm -rf $(BUILD_DIR) $(OUTPUT)

clean-lib:
	rm -f $(EMAP_LIB) $(EMAP_OBJ)

clean-all: clean clean-lib

# ---------- RUN ----------
run: build
	./$(TARGET)

# ---------- DOCUMENTATION ----------
doc:
	doxygen Doxyfile
	@echo "Documentation generated in docs/html/index.html"

.PHONY: build build-lib clean clean-lib clean-all run doc submodules