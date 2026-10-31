.RECIPEPREFIX = >

CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -Iinclude -MMD -MP
BUILD    := build

LIB_SRC  := $(wildcard src/*.cpp)
LIB_OBJ  := $(patsubst src/%.cpp,$(BUILD)/%.o,$(LIB_SRC))
APP_SRC  := $(wildcard apps/*.cpp)
APP_BIN  := $(patsubst apps/%.cpp,$(BUILD)/%.exe,$(APP_SRC))
TEST_SRC := $(wildcard tests/test_*.cpp)
TEST_BIN := $(patsubst tests/%.cpp,$(BUILD)/%.exe,$(TEST_SRC))

.PHONY: all test bench clean data

all: $(APP_BIN)

test: $(TEST_BIN)
> @for t in $(TEST_BIN); do echo "== $$t"; ./$$t || exit 1; done

bench: $(BUILD)/benchmark.exe
> ./$(BUILD)/benchmark.exe

$(BUILD)/%.o: src/%.cpp | $(BUILD)
> $(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD)/%.exe: apps/%.cpp $(LIB_OBJ) | $(BUILD)
> $(CXX) $(CXXFLAGS) $< $(LIB_OBJ) -o $@

$(BUILD)/%.exe: tests/%.cpp $(LIB_OBJ) | $(BUILD)
> $(CXX) $(CXXFLAGS) $< $(LIB_OBJ) -o $@

$(BUILD):
> mkdir -p $(BUILD)

clean:
> rm -rf $(BUILD)

data: $(BUILD)/gen_data.exe
> mkdir -p data/small data/medium data/large
> mkdir -p data/adversarial/sorted data/adversarial/reversed data/adversarial/equal
> ./$(BUILD)/gen_data.exe random 10 data/small
> ./$(BUILD)/gen_data.exe random 1000 data/medium
> ./$(BUILD)/gen_data.exe random 100000 data/large
> ./$(BUILD)/gen_data.exe sorted 10000 data/adversarial/sorted
> ./$(BUILD)/gen_data.exe reversed 10000 data/adversarial/reversed
> ./$(BUILD)/gen_data.exe equal 10000 data/adversarial/equal

-include $(wildcard $(BUILD)/*.d)