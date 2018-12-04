# compiler command and options
CC = gcc
CC_FLAGS = -Wall -Wextra

# Final binary
BIN = candecode
# Put all auto generated stuff to this build dir.
BUILD_DIR = ./build

# Default installation directory
DESTDIR ?=
# Default installation prefix
PREFIX ?= /usr/local

# List of all .c source files.
C_SOURCES = $(wildcard *.c)
# List of all .c source files without directories
C_SOURCES_NAMES = $(notdir $(C_SOURCES))

# All .o files go to build dir.
OBJ = $(C_SOURCES_NAMES:%.c=$(BUILD_DIR)/%.o)
# Gcc/Clang will create these .d files containing dependencies.
DEP = $(OBJ:%.o=%.d)

# Actual target of the binary - depends on all .o files.
$(BIN) : $(OBJ)
	$(CC) $(CC_FLAGS) $^ -o $@

$(BUILD_DIR):
	mkdir -p $@

# Include all .d files
-include $(DEP)

# Build target for every single object file.
# The potential dependency on header files is covered
# by calling `-include $(DEP)`.
$(BUILD_DIR)/%.o : %.c | $(BUILD_DIR)
	$(CC) $(CC_FLAGS) -MMD -c $< -o $@

install: $(BIN)
	install -D -t $(DESTDIR)/$(PREFIX)/bin $(BIN)

clean :
	rm -rf $(BIN) $(BUILD_DIR)
