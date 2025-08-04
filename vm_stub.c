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
#define CONST_COMPILED_FUNCTION 3
#define CONST_BOOLEAN 4
#define CONST_NULL 5
#define CONST_ARRAY 6
#define CONST_HASH 7

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

// Forward declaration
static int deserializeObject(Object *obj, unsigned char *data, int offset, int total_len);

// Deserialize an object from buffer, returns new offset
static int deserializeObject(Object *obj, unsigned char *data, int offset, int total_len) {
  if (offset >= total_len) {
    fprintf(stderr, "❌ unexpected EOF while reading object tag\n");
    exit(1);
  }
  
  uint8_t tag = *(uint8_t *)(data + offset);
  offset += 1;
  
  if (tag == CONST_INTEGER) {
    if (offset + sizeof(int64_t) > total_len) {
      fprintf(stderr, "❌ truncated integer object\n");
      exit(1);
    }
    int64_t val = read_le64(data + offset);
    offset += sizeof(int64_t);
    
    Integer *intObj = malloc(sizeof(Integer));
    intObj->value = val;
    obj->type = "Integer";
    obj->integer = intObj;
    
  } else if (tag == CONST_STRING) {
    if (offset + sizeof(int32_t) > total_len) {
      fprintf(stderr, "❌ truncated string length\n");
      exit(1);
    }
    int32_t len = read_le32(data + offset);
    offset += sizeof(int32_t);
    
    if (offset + len > total_len) {
      fprintf(stderr, "❌ truncated string data\n");
      exit(1);
    }
    char *str = malloc(len + 1);
    memcpy(str, data + offset, len);
    str[len] = '\0';
    offset += len;
    
    String *strObj = malloc(sizeof(String));
    strObj->value = str;
    obj->type = "String";
    obj->string = strObj;
    
  } else if (tag == CONST_BOOLEAN) {
    if (offset + 1 > total_len) {
      fprintf(stderr, "❌ truncated boolean value\n");
      exit(1);
    }
    uint8_t val = *(uint8_t *)(data + offset);
    offset += 1;
    
    Boolean *boolObj = malloc(sizeof(Boolean));
    boolObj->value = (val != 0);
    obj->type = "Boolean";
    obj->boolean = boolObj;
    
  } else if (tag == CONST_NULL) {
    Null *nullObj = malloc(sizeof(Null));
    obj->type = "Null";
    obj->null = nullObj;
    
  } else if (tag == CONST_ARRAY) {
    if (offset + sizeof(int32_t) > total_len) {
      fprintf(stderr, "❌ truncated array count\n");
      exit(1);
    }
    int32_t count = read_le32(data + offset);
    offset += sizeof(int32_t);
    
    Array *arrayObj = malloc(sizeof(Array));
    arrayObj->count = count;
    arrayObj->elements = malloc(sizeof(Object) * count);
    
    for (int i = 0; i < count; i++) {
      offset = deserializeObject(&arrayObj->elements[i], data, offset, total_len);
    }
    
    obj->type = "Array";
    obj->array = arrayObj;
    
  } else if (tag == CONST_HASH) {
    if (offset + sizeof(int32_t) > total_len) {
      fprintf(stderr, "❌ truncated hash pair count\n");
      exit(1);
    }
    int32_t pairCount = read_le32(data + offset);
    offset += sizeof(int32_t);
    
    // Create hash with reasonable bucket count
    int bucketCount = (pairCount > 0) ? (pairCount * 2) : 8;
    Hash *hashObj = malloc(sizeof(Hash));
    hashObj->buckets = calloc(bucketCount, sizeof(HashEntry*));
    hashObj->bucketCount = bucketCount;
    hashObj->size = pairCount;
    hashObj->capacity = bucketCount;
    
    // Read key-value pairs and insert into hash
    for (int i = 0; i < pairCount; i++) {
      Object key, value;
      offset = deserializeObject(&key, data, offset, total_len);
      offset = deserializeObject(&value, data, offset, total_len);
      
      // Simple hash insertion (just use first bucket for simplicity)
      HashEntry *entry = malloc(sizeof(HashEntry));
      entry->key = key;
      entry->value = value;
      entry->next = hashObj->buckets[0];
      hashObj->buckets[0] = entry;
    }
    
    obj->type = "Hash";
    obj->hash = hashObj;
    
  } else if (tag == CONST_COMPILED_FUNCTION) {
    // Read instruction count
    if (offset + sizeof(int32_t) > total_len) {
      fprintf(stderr, "❌ truncated compiled function instruction count\n");
      exit(1);
    }
    int32_t instr_count = read_le32(data + offset);
    offset += sizeof(int32_t);
    
    // Read instructions
    if (offset + instr_count > total_len) {
      fprintf(stderr, "❌ truncated compiled function instructions\n");
      exit(1);
    }
    unsigned char *instructions = malloc(instr_count);
    memcpy(instructions, data + offset, instr_count);
    offset += instr_count;
    
    // Read numLocals
    if (offset + sizeof(int32_t) > total_len) {
      fprintf(stderr, "❌ truncated compiled function numLocals\n");
      exit(1);
    }
    int32_t numLocals = read_le32(data + offset);
    offset += sizeof(int32_t);
    
    // Read numParameters
    if (offset + sizeof(int32_t) > total_len) {
      fprintf(stderr, "❌ truncated compiled function numParameters\n");
      exit(1);
    }
    int32_t numParameters = read_le32(data + offset);
    offset += sizeof(int32_t);
    
    // Create CompiledFunction object
    CompiledFunction *fnObj = malloc(sizeof(CompiledFunction));
    fnObj->instructions = (char*)instructions;
    fnObj->instructionCount = instr_count;
    fnObj->numLocals = numLocals;
    fnObj->numParameters = numParameters;
    
    obj->type = "CompiledFunction";
    obj->compiledFunction = fnObj;
    
  } else {
    fprintf(stderr, "❌ unknown object tag: %d\n", tag);
    exit(1);
  }
  
  return offset;
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
    offset = deserializeObject(&bc->constants[i], data, offset, total_len);
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
