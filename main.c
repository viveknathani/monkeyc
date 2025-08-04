#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>

#include "lexer/lexer.h"
#include "object/object.h"
#include "parser/parser.h"
#include "compiler/compiler.h"
#include "vm/vm.h"

#define BYTECODE_MARKER "MONKEY_BYTECODE"
#include "vm_stub_embed.h"

#define VERSION "1.0.0"
#define PROGRAM_NAME "monkeyc"

typedef struct {
  unsigned char *data;
  int length;
} SerializedBytecode;

typedef enum {
  CMD_REPL,
  CMD_RUN,
  CMD_BUILD,
  CMD_HELP,
  CMD_VERSION,
  CMD_INVALID
} CommandType;

typedef struct {
  CommandType type;
  char *input_file;
  char *output_file;
  char *error_message;
} ParsedArgs;

// --- Helper to read a file into memory ---
char *readFile(const char *filename) {
  FILE *fp = fopen(filename, "rb");
  if (!fp) {
    return NULL;
  }

  if (fseek(fp, 0, SEEK_END) != 0) {
    fclose(fp);
    return NULL;
  }

  long size = ftell(fp);
  if (size < 0) {
    fclose(fp);
    return NULL;
  }

  rewind(fp);
  char *buffer = malloc(size + 1);
  if (!buffer) {
    fclose(fp);
    return NULL;
  }

  size_t read_size = fread(buffer, 1, size, fp);
  buffer[read_size] = '\0';
  fclose(fp);
  return buffer;
}

#define CONST_INTEGER 1
#define CONST_STRING  2
#define CONST_COMPILED_FUNCTION 3
#define CONST_BOOLEAN 4
#define CONST_NULL 5
#define CONST_ARRAY 6
#define CONST_HASH 7

// Little-endian encoding/decoding helpers
static void write_le32(unsigned char *buf, uint32_t val) {
  buf[0] = val & 0xFF;
  buf[1] = (val >> 8) & 0xFF;
  buf[2] = (val >> 16) & 0xFF;
  buf[3] = (val >> 24) & 0xFF;
}

static void write_le64(unsigned char *buf, uint64_t val) {
  buf[0] = val & 0xFF;
  buf[1] = (val >> 8) & 0xFF;
  buf[2] = (val >> 16) & 0xFF;
  buf[3] = (val >> 24) & 0xFF;
  buf[4] = (val >> 32) & 0xFF;
  buf[5] = (val >> 40) & 0xFF;
  buf[6] = (val >> 48) & 0xFF;
  buf[7] = (val >> 56) & 0xFF;
}

// Forward declaration
static size_t calculateObjectSize(Object *obj);

// Calculate size needed to serialize an object
static size_t calculateObjectSize(Object *obj) {
  if (strcmp(obj->type, "Integer") == 0) {
    return 1 + sizeof(int64_t); // tag + value
  } else if (strcmp(obj->type, "String") == 0) {
    return 1 + sizeof(int32_t) + strlen(obj->string->value); // tag + length + content
  } else if (strcmp(obj->type, "Boolean") == 0) {
    return 1 + 1; // tag + bool value (1 byte)
  } else if (strcmp(obj->type, "Null") == 0) {
    return 1; // just tag
  } else if (strcmp(obj->type, "Array") == 0) {
    size_t size = 1 + sizeof(int32_t); // tag + count
    for (int i = 0; i < obj->array->count; i++) {
      size += calculateObjectSize(&obj->array->elements[i]);
    }
    return size;
  } else if (strcmp(obj->type, "Hash") == 0) {
    size_t size = 1 + sizeof(int32_t); // tag + pair count
    // Iterate through all buckets to count pairs
    for (int i = 0; i < obj->hash->bucketCount; i++) {
      HashEntry *entry = obj->hash->buckets[i];
      while (entry != NULL) {
        size += calculateObjectSize(&entry->key);
        size += calculateObjectSize(&entry->value);
        entry = entry->next;
      }
    }
    return size;
  } else if (strcmp(obj->type, "CompiledFunction") == 0) {
    return 1 + sizeof(int32_t) + obj->compiledFunction->instructionCount + sizeof(int32_t) + sizeof(int32_t);
  }
  return 0; // unsupported type
}

// Forward declaration
static size_t serializeObject(Object *obj, unsigned char *buf, size_t offset);

// Serialize an object to buffer
static size_t serializeObject(Object *obj, unsigned char *buf, size_t offset) {
  if (strcmp(obj->type, "Integer") == 0) {
    uint8_t tag = CONST_INTEGER;
    memcpy(buf + offset, &tag, 1);
    offset += 1;
    write_le64(buf + offset, obj->integer->value);
    offset += sizeof(int64_t);
    
  } else if (strcmp(obj->type, "String") == 0) {
    uint8_t tag = CONST_STRING;
    int32_t len = strlen(obj->string->value);
    memcpy(buf + offset, &tag, 1);
    offset += 1;
    write_le32(buf + offset, len);
    offset += sizeof(int32_t);
    memcpy(buf + offset, obj->string->value, len);
    offset += len;
    
  } else if (strcmp(obj->type, "Boolean") == 0) {
    uint8_t tag = CONST_BOOLEAN;
    memcpy(buf + offset, &tag, 1);
    offset += 1;
    uint8_t val = obj->boolean->value ? 1 : 0;
    memcpy(buf + offset, &val, 1);
    offset += 1;
    
  } else if (strcmp(obj->type, "Null") == 0) {
    uint8_t tag = CONST_NULL;
    memcpy(buf + offset, &tag, 1);
    offset += 1;
    
  } else if (strcmp(obj->type, "Array") == 0) {
    uint8_t tag = CONST_ARRAY;
    memcpy(buf + offset, &tag, 1);
    offset += 1;
    write_le32(buf + offset, obj->array->count);
    offset += sizeof(int32_t);
    for (int i = 0; i < obj->array->count; i++) {
      offset = serializeObject(&obj->array->elements[i], buf, offset);
    }
    
  } else if (strcmp(obj->type, "Hash") == 0) {
    uint8_t tag = CONST_HASH;
    memcpy(buf + offset, &tag, 1);
    offset += 1;
    write_le32(buf + offset, obj->hash->size); // number of key-value pairs
    offset += sizeof(int32_t);
    // Serialize all key-value pairs
    for (int i = 0; i < obj->hash->bucketCount; i++) {
      HashEntry *entry = obj->hash->buckets[i];
      while (entry != NULL) {
        offset = serializeObject(&entry->key, buf, offset);
        offset = serializeObject(&entry->value, buf, offset);
        entry = entry->next;
      }
    }
    
  } else if (strcmp(obj->type, "CompiledFunction") == 0) {
    uint8_t tag = CONST_COMPILED_FUNCTION;
    CompiledFunction *fn = obj->compiledFunction;
    memcpy(buf + offset, &tag, 1);
    offset += 1;
    write_le32(buf + offset, fn->instructionCount);
    offset += sizeof(int32_t);
    memcpy(buf + offset, fn->instructions, fn->instructionCount);
    offset += fn->instructionCount;
    write_le32(buf + offset, fn->numLocals);
    offset += sizeof(int32_t);
    write_le32(buf + offset, fn->numParameters);
    offset += sizeof(int32_t);
  }
  return offset;
}

SerializedBytecode serializeBytecode(ByteCode *bc) {
  int instr_len = bc->instructionCount;
  int const_count = bc->constantsCount;
  size_t size = 0;

  printf("📦 Preparing serialization...\n");
  printf("📐 Instruction count: %d bytes\n", instr_len);
  printf("📐 Constant count   : %d\n", const_count);

  size += sizeof(int32_t); // instruction length
  size += instr_len;       // instruction bytes
  size += sizeof(int32_t); // constant count

  for (int i = 0; i < const_count; i++) {
    Object *obj = &bc->constants[i];
    size_t obj_size = calculateObjectSize(obj);
    if (obj_size == 0) {
      fprintf(stderr, "⚠️ Skipping unsupported constant[%d] with type: %s\n", i, obj->type);
    } else {
      size += obj_size;
    }
  }

  printf("🧮 Total size to serialize: %zu bytes\n", size);

  unsigned char *buf = malloc(size);
  if (!buf) {
    fprintf(stderr, "❌ Failed to allocate %zu bytes\n", size);
    exit(1);
  }

  size_t offset = 0;


  write_le32(buf + offset, instr_len);
  offset += sizeof(int32_t);

  memcpy(buf + offset, bc->instructions, instr_len);
  offset += instr_len;

  write_le32(buf + offset, const_count);
  offset += sizeof(int32_t);

  for (int i = 0; i < const_count; i++) {
    Object *obj = &bc->constants[i];
    printf("🔍 Constant[%d] type = '%s'\n", i, obj->type);
    
    size_t old_offset = offset;
    offset = serializeObject(obj, buf, offset);
    
    if (offset == old_offset) {
      printf("   ⚠️ Unknown type, skipped.\n");
    } else {
      size_t obj_size = offset - old_offset;
      if (strcmp(obj->type, "Integer") == 0) {
        printf("   ↳ INTEGER value = %lld (%zu bytes)\n", obj->integer->value, obj_size);
      } else if (strcmp(obj->type, "String") == 0) {
        printf("   ↳ STRING length = %d, value = \"%s\" (%zu bytes)\n", 
               (int)strlen(obj->string->value), obj->string->value, obj_size);
      } else if (strcmp(obj->type, "Boolean") == 0) {
        printf("   ↳ BOOLEAN value = %s (%zu bytes)\n", 
               obj->boolean->value ? "true" : "false", obj_size);
      } else if (strcmp(obj->type, "Null") == 0) {
        printf("   ↳ NULL (%zu bytes)\n", obj_size);
      } else if (strcmp(obj->type, "Array") == 0) {
        printf("   ↳ ARRAY count = %d (%zu bytes)\n", obj->array->count, obj_size);
      } else if (strcmp(obj->type, "Hash") == 0) {
        printf("   ↳ HASH pairs = %d (%zu bytes)\n", obj->hash->size, obj_size);
      } else if (strcmp(obj->type, "CompiledFunction") == 0) {
        printf("   ↳ COMPILED_FUNCTION instructions=%d, locals=%d, params=%d (%zu bytes)\n", 
               obj->compiledFunction->instructionCount, obj->compiledFunction->numLocals, 
               obj->compiledFunction->numParameters, obj_size);
      }
    }
  }

  printf("✅ Final serialized size: %zu bytes\n", offset);

  SerializedBytecode sb;
  sb.data = buf;
  sb.length = offset;
  return sb;
}

void buildExecutable(const char *sourcePath, const char *outputPath) {
  char *input = readFile(sourcePath);
  if (!input) {
    fprintf(stderr, "Failed to read %s\n", sourcePath);
    exit(1);
  }

  Lexer *lexer = newLexer(input);
  Parser *parser = newParser(lexer);
  Program *program = parseProgram(parser);

  Compiler *compiler = newCompiler();
  compileProgram(compiler, program);

  SerializedBytecode sb = serializeBytecode(getByteCode(compiler));

  FILE *out = fopen(outputPath, "wb");
  if (!out) {
    fprintf(stderr, "Cannot open %s for writing\n", outputPath);
    exit(1);
  }

  // Write embedded vm_stub to output binary
  fwrite(bin_vm_stub, 1, bin_vm_stub_len, out);

  // Write marker + length + bytecode
  fwrite(BYTECODE_MARKER, 1, strlen(BYTECODE_MARKER), out);

  // Write bytecode length in little-endian format
  unsigned char len_bytes[4];
  write_le32(len_bytes, sb.length);
  fwrite(len_bytes, 1, 4, out);

  fwrite(sb.data, 1, sb.length, out);
  fclose(out);

  chmod(outputPath, 0755);
  printf("✅ Built %s\n", outputPath);
}



// --- Run in interpreter mode ---
void runSource(char *input) {
  Lexer *lexer = newLexer(input);
  Parser *parser = newParser(lexer);
  Program *program = parseProgram(parser);

  // Check for parser errors
  if (parser->errors && parser->errorCount > 0) {
    printf("Parser errors found:\n");
    for (int i = 0; i < parser->errorCount; i++) {
      printf("  %s\n", parser->errors[i]);
    }
    return;
  }

  Compiler *compiler = newCompiler();
  compileProgram(compiler, program);

  ByteCode *bytecode = getByteCode(compiler);
  printf("Bytecode generated: %d instructions, %d constants\n", 
         bytecode->instructionCount, bytecode->constantsCount);

  VM *vm = newVM(bytecode);
  if (!vm) {
    printf("Failed to create VM\n");
    return;
  }

  printf("running vm...\n");
  int result = run(vm);
  printf("ran vm... (result: %d)\n", result);
  printf("Stack pointer: %d\n", vm->sp);
  
  Object *top = stackTop(vm);
  if (top == NULL) {
    printf("No result on stack\n");
  } else {
    char *buf = inspect(top);
    if (buf == NULL) {
      printf("inspect returned NULL\n");
    } else {
      printf("%s\n", buf);
    }
  }
}

void repl() {
  char line[1024];
  printf("Monkey REPL 🐵 — type 'exit' to quit\n");
  while (1) {

    printf(">> ");
    if (!fgets(line, sizeof(line), stdin)) break;
    if (strncmp(line, "exit", 4) == 0) break;
    runSource(line);
  }
}

// --- Print usage information ---
void printUsage(const char *program_name) {
  printf("MonkeyC Programming Language v%s\n\n", VERSION);
  printf("USAGE:\n");
  printf("  %s                           Start interactive REPL\n", program_name);
  printf("  %s <file.mon>                Run a MonkeyC script\n", program_name);
  printf("  %s build <file.mon> [options] Compile to executable\n", program_name);
  printf("  %s help                      Show this help message\n", program_name);
  printf("  %s version                   Show version information\n\n", program_name);

  printf("BUILD OPTIONS:\n");
  printf("  -o <output>                  Specify output filename\n");
  printf("                               (default: input filename without extension)\n\n");

  printf("EXAMPLES:\n");
  printf("  %s                           # Start REPL\n", program_name);
  printf("  %s hello.mon                 # Run hello.mon\n", program_name);
  printf("  %s build hello.mon           # Compile to 'hello'\n", program_name);
  printf("  %s build hello.mon -o app    # Compile to 'app'\n", program_name);
}

// --- Print version information ---
void printVersion() {
  printf("MonkeyC v%s\n", VERSION);
  printf("A fast, compiled programming language\n");
}

// --- Generate default output filename ---
char *getDefaultOutputName(const char *input_file) {
  if (!input_file) return NULL;

  char *output = malloc(strlen(input_file) + 1);
  if (!output) return NULL;

  strcpy(output, input_file);

  // Remove .mon extension if present
  char *dot = strrchr(output, '.');
  if (dot && strcmp(dot, ".mon") == 0) {
    *dot = '\0';
  }

  return output;
}

// --- Parse command line arguments ---
ParsedArgs parseArgs(int argc, char **argv) {
  ParsedArgs args = {0};

  if (argc == 1) {
    args.type = CMD_REPL;
    return args;
  }

  if (argc == 2) {
    if (strcmp(argv[1], "help") == 0 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
      args.type = CMD_HELP;
    } else if (strcmp(argv[1], "version") == 0 || strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
      args.type = CMD_VERSION;
    } else {
      // Assume it's a file to run
      args.type = CMD_RUN;
      args.input_file = argv[1];
    }
    return args;
  }

  if (argc >= 3 && strcmp(argv[1], "build") == 0) {
    args.type = CMD_BUILD;
    args.input_file = argv[2];

    // Parse build options
    for (int i = 3; i < argc; i++) {
      if (strcmp(argv[i], "-o") == 0) {
        if (i + 1 < argc) {
          args.output_file = argv[i + 1];
          i++; // Skip the next argument
        } else {
          args.type = CMD_INVALID;
          args.error_message = "Error: -o option requires an output filename";
          return args;
        }
      } else {
        args.type = CMD_INVALID;
        args.error_message = "Error: Unknown build option";
        return args;
      }
    }

    // Set default output name if not specified
    if (!args.output_file) {
      args.output_file = getDefaultOutputName(args.input_file);
    }

    return args;
  }

  args.type = CMD_INVALID;
  args.error_message = "Error: Invalid command. Use 'monkeyc help' for usage information.";
  return args;
}

// --- Validate file exists and is readable ---
bool validateInputFile(const char *filename) {
  if (!filename) {
    fprintf(stderr, "Error: No input file specified\n");
    return false;
  }

  // Check if file exists and is readable
  if (access(filename, R_OK) != 0) {
    fprintf(stderr, "Error: Cannot read file '%s'\n", filename);
    return false;
  }

  // Check if it's a .mon file (optional but good UX)
  const char *ext = strrchr(filename, '.');
  if (!ext || strcmp(ext, ".mon") != 0) {
    fprintf(stderr, "Warning: '%s' doesn't have a .mon extension\n", filename);
  }

  return true;
}

int main(int argc, char **argv) {
  ParsedArgs args = parseArgs(argc, argv);

  switch (args.type) {
    case CMD_REPL:
      printf("MonkeyC v%s - Interactive REPL\n", VERSION);
      printf("Type 'exit' or press Ctrl+C to quit\n\n");
      repl();
      break;

    case CMD_RUN: {
      if (!validateInputFile(args.input_file)) {
        return 1;
      }

      char *input = readFile(args.input_file);
      if (!input) {
        fprintf(stderr, "Error: Failed to read file '%s'\n", args.input_file);
        return 1;
      }

      printf("Running '%s'...\n", args.input_file);
      runSource(input);
      free(input);
      break;
    }

    case CMD_BUILD: {
      if (!validateInputFile(args.input_file)) {
        if (args.output_file && args.output_file != args.input_file) {
          free(args.output_file);
        }
        return 1;
      }

      printf("Building '%s' -> '%s'...\n", args.input_file, args.output_file);
      buildExecutable(args.input_file, args.output_file);
      printf("✅ Build completed successfully!\n");

      if (args.output_file && args.output_file != args.input_file) {
        free(args.output_file);
      }
      break;
    }

    case CMD_HELP:
      printUsage(argv[0]);
      break;

    case CMD_VERSION:
      printVersion();
      break;

    case CMD_INVALID:
      fprintf(stderr, "%s\n\n", args.error_message);
      printUsage(argv[0]);
      return 1;
  }

  return 0;
}
