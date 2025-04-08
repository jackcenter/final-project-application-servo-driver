TARGET ?= servo_driver

TGT_DIR := ./bin
BUILD_DIR := ./build
SRC_DIR := ./src
INC_DIR := ./include

SRCS := $(filter-out $(SRC_DIR)/main.c, $(wildcard $(SRC_DIR)/*.c))	# Find all c files
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)	# make object files for all c files 

CC ?= $(CROSS_COMPILE)gcc
LDFLAGS ?=
CFLAGS ?= -g -Wall -Werror -O0 -I$(INC_DIR)

all: $(TGT_DIR)/$(TARGET)

# Target to build final executable
$(TGT_DIR)/$(TARGET): $(OBJS) $(BUILD_DIR)/main.o
	mkdir -p $(dir $@)
	$(CC) $(OBJS) $(BUILD_DIR)/main.o $(LDFLAGS) -o $@

# Target to build the object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/main.o: $(SRC_DIR)/main.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	@if [ -d "${BUILD_DIR}" ]; \
	then \
		rm -r ${BUILD_DIR}; \
	fi

	@if [ -d "${TGT_DIR}" ]; \
	then \
		rm -r ${TGT_DIR}; \
	fi

# ============== TESTS ==============
TEST_TGT_DIR := $(TGT_DIR)/tests
TEST_BUILD_DIR := $(BUILD_DIR)/tests
TEST_DIR := ./test

define build_test
$(eval test_name := $1)
$(eval source := $2)

$(eval target := $(TEST_TGT_DIR)/$(test_name))
$(eval object := $(TEST_BUILD_DIR)/$(test_name).o)

$(target): $(OBJS) $(object)
	mkdir -p $(dir $(target))
	$(CC) $(object) $(OBJS) $(LDFLAGS) -o $(target)

$(object): $(source)
	mkdir -p $(dir $(object))
	$(CC) $(CFLAGS) -c $(source) -o $(object)
endef

# List tests here
test: \
	$(TEST_TGT_DIR)/test_utilities \

# Build tests here
$(eval $(call build_test, test_utilities, $(TEST_DIR)/test_utilities.c))
