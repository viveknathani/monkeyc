CC := clang
CFLAGS := -std=c99 -Wall -Wextra -g
BIN_DIR := bin

$(shell mkdir -p $(BIN_DIR))

# Source files
SRC := $(filter-out tests/%.c, $(wildcard */*.c)) main.c
VM_STUB_SRC := $(filter-out main.c, $(SRC)) vm_stub.c
MONKEYC_OBJ := $(patsubst %.c,$(BIN_DIR)/%.o,$(SRC))
VM_STUB_OBJ := $(patsubst %.c,$(BIN_DIR)/%.o,$(VM_STUB_SRC))

# Output files
OUT := $(BIN_DIR)/monkeyc
VM_STUB_OUT := $(BIN_DIR)/vm_stub
VM_STUB_EMBED := vm_stub_embed.h

# Test files
TEST_SOURCES := $(wildcard tests/test_*.c)
TEST_BINS := $(patsubst tests/%.c, bin/%, $(TEST_SOURCES))
TEST_OBJS := $(filter-out $(BIN_DIR)/main.o, $(MONKEYC_OBJ))

# Main target - build everything
all: $(OUT)

# Main executable depends on vm_stub_embed.h being up to date
$(OUT): $(MONKEYC_OBJ) | $(VM_STUB_EMBED)
	$(CC) $(CFLAGS) $(MONKEYC_OBJ) -o $@

# Generate vm_stub_embed.h from vm_stub binary
$(VM_STUB_EMBED): $(VM_STUB_OUT)
	@echo "📦 Embedding vm_stub into header..."
	xxd -i $< > $@

# Build vm_stub binary
$(VM_STUB_OUT): $(VM_STUB_OBJ)
	@echo "🔧 Building vm_stub..."
	$(CC) $(CFLAGS) $^ -o $@

# Object file compilation
$(BIN_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Force rebuild of main.o if vm_stub_embed.h changes
$(BIN_DIR)/main.o: main.c $(VM_STUB_EMBED)
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
	rm -rf $(BIN_DIR) $(VM_STUB_EMBED)
	@echo "🧹 Cleaned build artifacts"

# Rebuild everything from scratch
rebuild: clean all

.PHONY: all clean test rebuild
