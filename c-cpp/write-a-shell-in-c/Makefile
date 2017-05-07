# Makefile (based on https://github.com/mbcrawfo/GenericMakefile/)

### PROJECT SETTINGS ###

# Name of the excutable to be created
BIN_NAME := lsh

# Compiler used
CC ?= gcc

# Extension of source files in this project
SRC_EXT = c

# Path to the source dir, relative to this makefile.
SRC_PATH = ./src

# Space-seperated pkg-config libraries used by this project.
LIBS =

# General compiler flags.
COMPILE_FLAGS = -Wall -Wextra -g

# Additional release-specific compiler flags.
RCOMPILE_FLAGS = -D NDEBUG

# Additional debug-specific compiler flags.
DCOMPILE_FLAGS = -D DEBUG

# Add additional include paths.
INCLUDES = -I $(SRC_PATH)

# General linker settings
LINK_FLAGS =

# Additional release-specific linker settings
RLINK_FLAGS =

# Additional debug-specific linker settings
DLINK_FLAGS =

# Destination directory
DESTDIR = /

# Install path (/bin/ is appened automatically)
INSTALL_PREFIX = usr/local

### END PROJECT SETTINGS ###

# OS type - either 'Darwin' or 'Linux'
UNAME_S := $(shell uname -s)

# Check the value of a variable.
# Useful for debugging and adding features.
#
# Usage:
#   make print-<VARNAME>
print-%: ; @echo $*=$($*)

# Shell used in this makefile.
# bash is used for better cross-compatibility and `echo -en`.
SHELL := /usr/bin/env bash

# Clear built-in rules.
.SUFFIXES:

# Programs for installation
INSTALL := install
INSTALL_PROGRAM := $(INSTALL)
INSTALL_DATA := $(INSTALL) -m 644

# Append pkg-config specific libraries if need be.
ifneq ($(LIBS),)
	COMPILE_FLAGS += $(shell pkg-config --cflags $(LIBS))
	LINK_FLAGS += $(shell pkg-config --libs $(LIBS))
endif

# Verbose option, to output compile and link commands.
export V := false
export CMD_PREFIX := @
ifeq ($(V),true)
	CMD_PREFIX :=
endif

# Combine compiler and linker flags.
release: export CFLAGS := $(CFLAGS) $(COMPILE_FLAGS) $(RCOMPILE_FLAGS)
release: export LDFLAGS := $(LDFLAGS) $(LINK_FLAGS) $(RLINK_FLAGS)
debug: export CFLAGS := $(CFLAGS) $(COMPILE_FLAGS) $(DCOMPILE_FLAGS)
debug: export LDFLAGS := $(LDFLAGS) $(LINK_FLAGS) $(DLINK_FLAGS)

# Build and output paths
release: export BUILD_PATH := build/release
release: export BIN_PATH := bin/release
debug: export BUILD_PATH := build/debug
debug: export BIN_PATH := bin/debug
install: export BIN_PATH := bin/release

# Find all source files in the source directory, sorted by most
# recently modified.
ifeq ($(UNAME_S),Darwin)
	SOURCES = $(shell find $(SRC_PATH) -name '*.$(SRC_EXT)' | sort -k 1nr | cut -f2-)
else
	SOURCES = $(shell find $(SRC_PATH) -name '*.$(SRC_EXT)' -printf '%T@\t%p\n' \
						| sort -k 1nr | cut -f2-)
endif

# Fallback in case the above fails
rwildcard = $(foreach d, $(wildcard $1*), $(call rwildcard,$d/,$2) \
						$(filter $(subst *,%,$2), $d))

ifeq ($(SOURCES),)
	SOURCES := $(call rwildcard, $(SRC_PATH), *.$(SRC_EXT))
endif

# Set the object file names, with the source directory stripped from the path,
# and the build path prepended in its place.
OBJECTS = $(SOURCES:$(SRC_PATH)/%.$(SRC_EXT)=$(BUILD_PATH)/%.o)

# Set the dependency files that will be used to add header dependencies.
DEPS = $(OBJECTS:.o=.d)

# Macros for timing compilation
ifeq ($(UNAME_S),Darwin)
	CUR_TIME = awk 'BEGIN{srand(); print srand()}'
	TIME_FILE = $(dir $@).$(notdir $@)_time
	START_TIME = $(CUR_TIME) > $(TIME_FILE)
	END_TIME = read st < $(TIME_FILE) ; \
						 $(RM) $(TIME_FILE) ; \
						 st=$$((`$(CUR_TIME)` - $$st)) ; \
						 echo $$st
else
	TIME_FILE = $(dir $@).$(notdir $@)_time
	START_TIME = date '+%s' > $(TIME_FILE)
	END_TIME = read st < $(TIME_FILE) ; \
						 $(RM) $(TIME_FILE) ; \
						 st=$$((`date '+%s'` - $$st - 86400)) ; \
						 echo `date -u -d @$$st '+%H:%M:%S'`
endif

# Standard, non-optimized release build.
.PHONY: release
release: dirs
	@echo "Beginning release build."
	@$(START_TIME)
	@$(MAKE) all --no-print-directory
	@echo -n "Total build time: "
	@$(END_TIME)

# Debug build for gdb debugging.
.PHONY: debug
debug: dirs
	@echo "Beginning debug build."
	@$(START_TIME)
	@$(MAKE) all --no-print-directory
	@echo -n "Total build time: "
	@$(END_TIME)

# Create the directories used in the build.
.PHONY: dirs
dirs:
	@echo "Creating directories."
	@mkdir -p $(dir $(OBJECTS))
	@mkdir -p $(BIN_PATH)

# Installs to the set path.
.PHONY: install
install:
	@echo "Installing to $(DESTDIR)$(INSTALL_PREFIX)/bin"
	@$(INSTALL_PROGRAM) $(BIN_PATH)/$(BIN_NAME) $(DESTDIR)$(INSTALL_PREFIX)/bin

# Uninstalls the program.
.PHONY: uninstall
uninstall:
	@echo "Removing $(DESTDIR)$(INSTALL_PREFIX)/bin/$(BIN_NAME)"
	@$(RM) $(DESTDIR)$(INSTALL_PREFIX)/bin/$(BIN_NAME)

# Removes all build files.
.PHONY: clean
clean:
	@echo "Deleting $(BIN_NAME) symlink."
	@$(RM) $(BIN_NAME)
	@echo "Deleting directories."
	@$(RM) -r build
	@$(RM) -r bin

# Main rule. Checks the executable and symlinks to the output.
all: $(BIN_PATH)/$(BIN_NAME)
	@echo "Making symlink: $(BIN_NAME) -> $<"
	@$(RM) $(BIN_NAME)
	@ln -s $(BIN_PATH)/$(BIN_NAME) $(BIN_NAME)

# Link the executable.
$(BIN_PATH)/$(BIN_NAME): $(OBJECTS)
	@echo "Linking: $@"
	@$(START_TIME)
	$(CMD_PREFIX)$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	@echo -en "\t Link time: "
	@$(END_TIME)

# Add dependency files, if they exist.
-include $(DEPS)

# Source file rules.
# After the first compilation they will be joined with the rules from the
# dependency files to provide header dependencies.
$(BUILD_PATH)/%.o: $(SRC_PATH)/%.$(SRC_EXT)
	@echo "Compiling: $< -> $@"
	@$(START_TIME)
	$(CMD_PREFIX)$(CC) $(CFLAGS) $(INCLUDES) -MP -MMD -c $< -o $@
	@echo -en "\t Compile time: "
	@$(END_TIME)
