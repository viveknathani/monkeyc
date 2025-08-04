#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "vm/vm.h"
#include "object/object.h"
#include "compiler/compiler.h"

#define BYTECODE_MARKER "MONKEY_BYTECODE"
#define CONST_INTEGER 1
#define CONST_STRING  2

// Little-endian decoding helpers
static uint32_t read_le32(const unsigned char *buf) {
  return (uint32_t)buf[0] |
         ((uint32_t)buf[1] << 8) |
         ((uint32_t)buf[2] << 16) |
         ((uint32_t)buf[3] << 24);
}

static uint64_t read_le64(const unsigned char *buf) {
  return (uint64_t)buf[0] |
         ((uint64_t)buf[1] << 8) |
         ((uint64_t)buf[2] << 16) |
         ((uint64_t)buf[3] << 24) |
         ((uint64_t)buf[4] << 32) |
         ((uint64_t)buf[5] << 40) |
         ((uint64_t)buf[6] << 48) |
         ((uint64_t)buf[7] << 56);
}

ByteCode *deserializeBytecode(unsigned char *data, int total_len) {
  ByteCode *bc = malloc(sizeof(ByteCode));
  int offset = 0;

  if (offset + sizeof(int32_t) > total_len) {
    fprintf(stderr, "❌ truncated bytecode: no instruction length\n");
    exit(1);
  }

  int32_t instr_len = read_le32(data + offset);
  offset += sizeof(int32_t);

  bc->instructions = malloc(instr_len);
  memcpy(bc->instructions, data + offset, instr_len);
  bc->instructionCount = instr_len;
  offset += instr_len;

  if (offset + sizeof(int32_t) > total_len) {
    fprintf(stderr, "❌ truncated bytecode: no constant count\n");
    exit(1);
  }

  int32_t const_count = read_le32(data + offset);
  offset += sizeof(int32_t);
  bc->constantsCount = const_count;
  bc->constants = malloc(sizeof(Object) * const_count);

  for (int i = 0; i < const_count; i++) {
    if (offset >= total_len) {
      fprintf(stderr, "❌ unexpected EOF while reading constant[%d] tag\n", i);
      exit(1);
    }

    uint8_t tag = *(uint8_t *)(data + offset);
    offset += 1;

    if (tag == CONST_INTEGER) {
      if (offset + sizeof(int64_t) > total_len) {
        fprintf(stderr, "❌ truncated integer constant at [%d]\n", i);
        exit(1);
      }

      int64_t val = read_le64(data + offset);
      offset += sizeof(int64_t);

      Integer *intObj = malloc(sizeof(Integer));
      intObj->value = val;

      bc->constants[i].type = "Integer";
      bc->constants[i].integer = intObj;

    } else if (tag == CONST_STRING) {
      if (offset + sizeof(int32_t) > total_len) {
        fprintf(stderr, "❌ truncated string length at [%d]\n", i);
        exit(1);
      }

      int32_t len = read_le32(data + offset);
      offset += sizeof(int32_t);

      if (offset + len > total_len) {
        fprintf(stderr, "❌ truncated string data at [%d]\n", i);
        exit(1);
      }

      char *str = malloc(len + 1);
      memcpy(str, data + offset, len);
      str[len] = '\0';
      offset += len;

      String *strObj = malloc(sizeof(String));
      strObj->value = str;

      bc->constants[i].type = "String";
      bc->constants[i].string = strObj;

    } else {
      fprintf(stderr, "❌ unknown constant tag: %d at constant[%d]\n", tag, i);
      exit(1);
    }
  }

  return bc;
}

int main(int argc, char **argv) {
  FILE *self = fopen(argv[0], "rb");
  if (!self) { perror("fopen self"); return 1; }

  fseek(self, 0, SEEK_END);
  long size = ftell(self);
  rewind(self);

  unsigned char *data = malloc(size);
  if (fread(data, 1, size, self) != size) {
    fprintf(stderr, "❌ failed to read full binary\n");
    return 1;
  }
  fclose(self);

  // Find the LAST occurrence of the marker (search backwards)
  unsigned char *marker = NULL;
  size_t marker_len = strlen(BYTECODE_MARKER);
  for (long i = size - marker_len; i >= 0; i--) {
    if (memcmp(data + i, BYTECODE_MARKER, marker_len) == 0) {
      marker = data + i;
      break;
    }
  }
  
  if (!marker) {
    fprintf(stderr, "❌ no bytecode marker found.\n");
    return 1;
  }

  size_t marker_offset = marker - data;
  size_t len_offset = marker_offset + strlen(BYTECODE_MARKER);

  if (len_offset + sizeof(int32_t) > size) {
    fprintf(stderr, "❌ not enough space for bytecode length\n");
    return 1;
  }

  int32_t bytecode_len = read_le32(data + len_offset);

  unsigned char *bytecode = data + len_offset + sizeof(int32_t);

  if (bytecode + bytecode_len > data + size) {
    fprintf(stderr, "❌ bytecode exceeds file size\n");
    return 1;
  }

  ByteCode *bc = deserializeBytecode(bytecode, bytecode_len);

  VM *vm = newVM(bc);
  run(vm);

  Object *top = stackTop(vm);
  if (!top) {
    return 0;
  }

  char *out = inspect(top);
  printf("%s\n", out);

  return 0;
}
