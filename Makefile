CC := clang
CFLAGS := -std=c99 -Wall -Wextra -g
BIN_DIR := bin

$(shell mkdir -p $(BIN_DIR))

SRC := $(filter-out tests/%.c, $(wildcard */*.c)) main.c
OBJ := $(patsubst %.c,$(BIN_DIR)/%.o,$(SRC))
OUT := $(BIN_DIR)/monkeyc

TEST_SOURCES := $(wildcard tests/test_*.c)
TEST_BINS := $(patsubst tests/%.c, bin/%, $(TEST_SOURCES))
TEST_OBJS := $(filter-out $(BIN_DIR)/main.o, $(OBJ))

all: $(OUT)

$(OUT): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@

$(BIN_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TEST_BINS)
	@echo "🔬 Running tests..."
	@for testbin in $(TEST_BINS); do \
		echo "→ $$testbin"; \
		./$$testbin || exit 1; \
	done
	@echo "✅ all tests passed"

bin/%: tests/%.c $(TEST_OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< $(TEST_OBJS) -o $@

clean:
	rm -rf $(BIN_DIR)

.PHONY: all clean
