ARCH := $(shell uname -m)

# Determine if we are cross-compiling
ifeq ($(ARCH),x86_64)
	ifeq ($(CROSS_COMPILE),true)
		CC := aarch64-linux-gnu-gcc
		ARCH := aarch64
	else
		CC := gcc
	endif
else ifeq ($(ARCH),aarch64)
	CC := gcc
endif

INCLUDE_DIR = linux_code/include
SHARED_INCLUDE_DIR = ../SHARED_CODE/include
INCL = $(wildcard $(INCLUDE_DIR)/*.h) $(wildcard $(INCLUDE_DIR)/rpmsg_char/*.h)
INCL += $(wildcard $(SHARED_INCLUDE_DIR)/*.h)
SRCDIR = linux_code/src
SHARED_SRCDIR = ../SHARED_CODE/src
SRC_LOCAL = $(wildcard $(SRCDIR)/*.c) $(wildcard $(SRCDIR)/rpmsg_char/*.c)
SRC_SHARED = $(wildcard $(SHARED_SRCDIR)/*.c)
SRC = $(SRC_LOCAL) $(SRC_SHARED)
BUILD_DIR := ../build/linux

RM = /bin/rm -f

BUILD_MODE ?= debug

# Object files
OBJ_LOCAL = $(patsubst $(SRCDIR)/%.c, $(BUILD_DIR)/%.$(ARCH).$(BUILD_MODE).o, $(SRC_LOCAL))
OBJ_SHARED = $(patsubst $(SHARED_SRCDIR)/%.c, $(BUILD_DIR)/shared/%.$(ARCH).$(BUILD_MODE).o, $(SRC_SHARED))
OBJ = $(OBJ_LOCAL) $(OBJ_SHARED)

# Compiler defines
CFGENERAL = -D_GNU_SOURCE -DARCH='"$(ARCH)"'
WARNINGS = -W -Wall -Wstrict-prototypes -Wmissing-prototypes -Wshadow -fno-common
STANDARD = -std=c99 -D_POSIX_C_SOURCE=200112L
FSAN_FLAGS = -fsanitize=address -fsanitize=pointer-compare -fsanitize=pointer-subtract
INCLUDES = -Ilinux_code/include -Ilinux_code/include/rpmsg_char -I../SHARED_CODE/include
DEBUG_CFLAGS := -pedantic -Og -g3 $(WARNINGS) $(STANDARD) $(CFGENERAL) -fmacro-prefix-map=linux_code/=

ifeq ($(BUILD_MODE),release)
	CFLAGS := -g -O3 $(WARNINGS) $(STANDARD) $(CFGENERAL) -fmacro-prefix-map=linux_code/= -DNDEBUG
else ifeq ($(BUILD_MODE),debug)
	CFLAGS := -Og -g3 $(WARNINGS) $(STANDARD) $(CFGENERAL) -fmacro-prefix-map=linux_code/=
else
$(error Unknown BUILD_MODE, set BUILD_MODE=[debug, release])
endif

# Build options
CUSTOMDEFINES ?= -DLINUX

# Linker defines
LIBPATH := /usr/lib
ifeq ($(ARCH), x86_64)
	LIBPATH := -L$(LIBPATH)/x86_64-linux-gnu
else ifeq ($(ARCH), aarch64)
	LIBPATH := -L$(LIBPATH)/aarch64-linux-gnu
endif
LIBS += -ldl -lm -lgpiod
LDFLAGS += $(LIBPATH) $(LIBS) -pthread

# Name
EXE = LINUX_SIDE_$(ARCH)

.PHONY: clean all help debug release mem_debug

all: $(BUILD_DIR)/$(EXE) Makefile
	@echo --- LINUX_SIDE compile done

debug: BUILD_MODE := debug
debug: all

release: BUILD_MODE := release
release: all

# Rule to create the build directory
$(BUILD_DIR):
	@mkdir -p $@

# Compilation rules
$(BUILD_DIR)/%.$(ARCH).$(BUILD_MODE).o: $(SRCDIR)/%.c Makefile
	@mkdir -p $(@D)
	$(CC) -c $(CFLAGS) $(CUSTOMDEFINES) $(INCLUDES) $< -o $@

$(BUILD_DIR)/shared/%.$(ARCH).$(BUILD_MODE).o: $(SHARED_SRCDIR)/%.c Makefile
	@mkdir -p $(@D)
	$(CC) -c $(CFLAGS) $(CUSTOMDEFINES) $(INCLUDES) $< -o $@

$(OBJ): $(INCL)

$(BUILD_DIR)/$(EXE): $(OBJ) Makefile
	$(CC) $(OBJ) -o $(BUILD_DIR)/$(EXE) $(LDFLAGS)

# Debug build with address sanitizer 
mem_debug:
	@mkdir -p $(BUILD_DIR)
	$(CC) $(DEBUG_CFLAGS) $(FSAN_FLAGS) $(CUSTOMDEFINES) $(INCLUDES) $(SRC) -o $(BUILD_DIR)/$@_$(EXE) $(LDFLAGS)

help:
	@echo ------------------------------------------------------
	@echo A72 make help
	@echo ------------------------------------------------------
	@echo make [OPTIONAL MAKE VARIABLES]
	@echo
	@echo "Supported targets:"
	@echo "------------------"
	@echo "all            : Builds A72 code with default options"
	@echo "debug          : Builds with BUILD_MODE=debug"
	@echo "release        : Builds with BUILD_MODE=release"
	@echo "mem_debug      : Debug build with address sanitizer"
	@echo "clean          : Cleans"
	@echo ""
	@echo ""
	@echo "Optional make variables:"
	@echo "------------------------"
	@echo "BUILD_MODE=[debug, release]"
	@echo "    Default: debug"
	@echo "    Controls Linux compiler flags only"
	@echo "    debug: -Og -g3"
	@echo "    release: -O3 -DNDEBUG"
	@echo "CROSS_COMPILE=[true, false]"
	@echo "    Use true on x86_64 to build the aarch64 target"

clean:
	$(RM) -r $(BUILD_DIR)

# Debugging
$(info SRC_SHARED: $(SRC_SHARED))
$(info OBJ_SHARED: $(OBJ_SHARED))
$(info OBJ: $(OBJ))