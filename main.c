#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "lexer/lexer.h"
#include "object/object.h"
#include "parser/parser.h"
#include "compiler/compiler.h"
#include "vm/vm.h"

#define BYTECODE_MARKER "MONKEY_BYTECODE"
#include "vm_stub_embed.h"

typedef struct {
  unsigned char *data;
  int length;
} SerializedBytecode;

// --- Helper to read a file into memory ---
char *readFile(const char *filename) {
  FILE *fp = fopen(filename, "rb");
  if (!fp) return NULL;
  fseek(fp, 0, SEEK_END);
  long size = ftell(fp);
  rewind(fp);
  char *buffer = malloc(size + 1);
  fread(buffer, 1, size, fp);
  buffer[size] = '\0';
  fclose(fp);
  return buffer;
}

#include <stdint.h>
#include <string.h>

#define CONST_INTEGER 1
#define CONST_STRING  2

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
    size += 1; // tag
    if (strcmp(obj->type, "Integer") == 0) {
      size += sizeof(int64_t);
    } else if (strcmp(obj->type, "String") == 0) {
      int32_t len = strlen(obj->string->value);
      size += sizeof(int32_t); // string length
      size += len;             // string content
    } else {
      fprintf(stderr, "⚠️ Skipping unsupported constant[%d] with type: %s\n", i, obj->type);
    }
  }

  printf("🧮 Total size to serialize: %zu bytes\n", size);

  unsigned char *buf = malloc(size);
  if (!buf) {
    fprintf(stderr, "❌ Failed to allocate %zu bytes\n", size);
    exit(1);
  }

  size_t offset = 0;


  memcpy(buf + offset, &instr_len, sizeof(int32_t));
  offset += sizeof(int32_t);

  memcpy(buf + offset, bc->instructions, instr_len);
  offset += instr_len;

  memcpy(buf + offset, &const_count, sizeof(int32_t));
  offset += sizeof(int32_t);

  for (int i = 0; i < const_count; i++) {
    Object *obj = &bc->constants[i];
    printf("🔍 Constant[%d] type = '%s'\n", i, obj->type);

    if (strcmp(obj->type, "Integer") == 0) {
      uint8_t tag = CONST_INTEGER;
      memcpy(buf + offset, &tag, 1);
      offset += 1;

      memcpy(buf + offset, &obj->integer->value, sizeof(int64_t));
      offset += sizeof(int64_t);

      printf("   ↳ INTEGER value = %lld (8 bytes)\n", obj->integer->value);

    } else if (strcmp(obj->type, "String") == 0) {
      uint8_t tag = CONST_STRING;
      int32_t len = strlen(obj->string->value);

      memcpy(buf + offset, &tag, 1);
      offset += 1;

      memcpy(buf + offset, &len, sizeof(int32_t));
      offset += sizeof(int32_t);

      memcpy(buf + offset, obj->string->value, len);
      offset += len;

      printf("   ↳ STRING length = %d, value = \"%s\"\n", len, obj->string->value);

    } else {
      printf("   ⚠️ Unknown type, skipped.\n");
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
  fwrite(&sb.length, sizeof(int), 1, out);
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

  Compiler *compiler = newCompiler();
  compileProgram(compiler, program);

  ByteCode *bytecode =getByteCode(compiler);

  VM *vm = newVM(bytecode);
  run(vm);
  char *buf = inspect(stackTop(vm));
  printf("%s\n", buf) ;
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

int main(int argc, char **argv) {
  if (argc >= 3 && strcmp(argv[1], "build") == 0) {
    // monkeyc build foo.mon -o foo
    const char *sourcePath = argv[2];
    const char *outputPath = argv[4];
    buildExecutable(sourcePath, outputPath);
  } else if (argc == 2) {
    // monkeyc foo.mon
    char *input = readFile(argv[1]);
    if (!input) {
      fprintf(stderr, "Could not read %s\n", argv[1]);
      return 1;
    }
    runSource(input);
    free(input);
  } else {
    // monkeyc
    repl();
  }
  return 0;
}
