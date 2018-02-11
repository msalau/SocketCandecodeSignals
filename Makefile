# compiler command and options
CC = gcc
CC_FLAGS = -Wall

# Final binary
BIN = candecode
# Put all auto generated stuff to this build dir.
BUILD_DIR = ./build

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

# Include all .d files
-include $(DEP)

# Build target for every single object file.
# The potential dependency on header files is covered
# by calling `-include $(DEP)`.
$(BUILD_DIR)/%.o : %.c
	$(CC) $(CC_FLAGS) -MMD -c $< -o $@

install:
	cp -f $(BIN) /usr/local/bin

clean :
	rm $(BIN) $(OBJ) $(DEP)
