CC := clang
CFLAGS := -std=c99 -Wall -Wextra -g
BIN_DIR := bin

$(shell mkdir -p $(BIN_DIR))

SRC := $(wildcard */*.c) main.c
OBJ := $(patsubst %.c,$(BIN_DIR)/%.o,$(SRC))
OUT := $(BIN_DIR)/monkeyc

all: $(OUT)

$(OUT): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@

$(BIN_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BIN_DIR)

.PHONY: all clean
