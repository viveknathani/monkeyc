CC := clang
CFLAGS := -std=c99 -Wall -Wextra -g
BIN_DIR := bin

$(shell mkdir -p $(BIN_DIR))

SRC := $(filter-out tests/%.c, $(wildcard */*.c)) main.c
VM_STUB_SRC := $(filter-out main.c, $(SRC)) vm_stub.c
MONKEYC_OBJ := $(patsubst %.c,$(BIN_DIR)/%.o,$(SRC))
VM_STUB_OBJ := $(patsubst %.c,$(BIN_DIR)/%.o,$(VM_STUB_SRC))
OUT := $(BIN_DIR)/monkeyc
VM_STUB_OUT := $(BIN_DIR)/vm_stub

TEST_SOURCES := $(wildcard tests/test_*.c)
TEST_BINS := $(patsubst tests/%.c, bin/%, $(TEST_SOURCES))
TEST_OBJS := $(filter-out $(BIN_DIR)/main.o, $(OBJ))

all: $(OUT) $(VM_STUB_OUT)

$(OUT): $(MONKEYC_OBJ)
	$(CC) $(CFLAGS) $^ -o $@

$(VM_STUB_OUT): $(VM_STUB_OBJ)
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
