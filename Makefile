# ===== Makefile =====


TARGET := p2rom


CPP_SRCS := builder.cpp
C_SRCS   := $(wildcard zx7/*.c)


BUILD_DIR := build


CXX      ?= c++
CC       ?= cc
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
CFLAGS   ?= -std=c11   -O2 -Wall -Wextra -Wpedantic
LDFLAGS  ?=
LDLIBS   ?=


DEBUG_CXXFLAGS := -std=c++17 -g -O0 -Wall -Wextra -Wpedantic
DEBUG_CFLAGS   := -std=c11   -g -O0 -Wall -Wextra -Wpedantic


CPP_OBJS := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(CPP_SRCS))
C_OBJS   := $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SRCS))
OBJS     := $(CPP_OBJS) $(C_OBJS)


.PHONY: all
all: release


.PHONY: release
release: CXXFLAGS := $(CXXFLAGS)
release: CFLAGS := $(CFLAGS)
release: $(TARGET)


.PHONY: debug
debug: CXXFLAGS := $(DEBUG_CXXFLAGS)
debug: CFLAGS := $(DEBUG_CFLAGS)
debug: $(TARGET)


$(TARGET): $(OBJS)
	@echo "  [LD]  $@"
	$(CXX) $(OBJS) -o $@ $(LDFLAGS) $(LDLIBS)

# C++ -> .o
$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo "  [C++] $<"
	$(CXX) $(CXXFLAGS) -c $< -o $@

# C -> .o
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "  [C]   $<"
	$(CC) $(CFLAGS) -c $< -o $@


.PHONY: clean
clean:
	@echo "  [CLEAN]"
	@rm -rf $(BUILD_DIR) $(TARGET)


.PHONY: help
help:
	@echo "Targets:"
	@echo "  make / make release   - build i release mode"
	@echo "  make debug            - build i debug mode"
	@echo "  make clean            - slet build-artifacts"

