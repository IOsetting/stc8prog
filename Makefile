TARGET_EXEC := stc8prog
BUILD_DIR := ./build
SRC_DIRS := ./src

TARGET_OS :=
ifeq ($(OS),Windows_NT)
	TARGET_OS += win32
	SRC_DIRS += ./src/serial/win32
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		TARGET_OS += Linux
		SRC_DIRS += ./src/serial/linux
	endif
endif

# Find all the C files we want to compile
# Note the single quotes around the * expressions. Make will incorrectly expand these otherwise.
SRCS := $(shell find $(SRC_DIRS) -maxdepth 1 -name '*.c')

# String substitution for every source file.
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

# Every folder in ./src will need to be passed to GCC so that it can find header files
INC_DIRS := $(shell find $(SRC_DIRS) -maxdepth 1 -type d)
# Add a prefix to INC_DIRS. So moduleA would become -ImoduleA. GCC understands this -I flag
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

# The final build step.
$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)
# Move binary to root catalog to make dobin happy
	mv $(BUILD_DIR)/$(TARGET_EXEC) ./$(TARGET_EXEC)

# Build step for C source
$(BUILD_DIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INC_FLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f -r $(BUILD_DIR)
	rm -f ./$(TARGET_EXEC)
