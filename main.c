#include "compiler/compiler.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "vm/vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_REPL_INPUT 1024

void runProgram(char *input) {
  Lexer *lexer = newLexer(input);
  Parser *parser = newParser(lexer);
  Program *program = parseProgram(parser);

  Compiler *compiler = newCompiler();
  compileProgram(compiler, program);

  VM *vm = newVM(getByteCode(compiler));
  run(vm);

  Object *result = stackTop(vm);
  printf("=> %s\n", inspect(result));
}

char *readFile(const char *filename) {
  FILE *fp = fopen(filename, "r");
  if (!fp) {
    fprintf(stderr, "Could not open file: %s\n", filename);
    exit(1);
  }

  fseek(fp, 0, SEEK_END);
  long len = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  char *buffer = malloc(len + 1);
  if (!buffer) {
    fprintf(stderr, "Memory error\n");
    exit(1);
  }

  fread(buffer, 1, len, fp);
  buffer[len] = '\0';
  fclose(fp);
  return buffer;
}

void runRepl() {
  char line[1024];

  printf("Monkey REPL 🐵 — type `exit` to quit\n");

  while (1) {
    printf(">> ");
    if (!fgets(line, sizeof(line), stdin)) {
      printf("\n");
      break;
    }

    // strip newline
    line[strcspn(line, "\n")] = '\0';

    if (strcmp(line, "exit") == 0) {
      break;
    }

    runProgram(line);
  }
}

void readFileOrPrompt(int argc, char **argv) {
  if (argc > 1) {
    // File mode
    FILE *fp = fopen(argv[1], "r");
    char *buffer = readFile(argv[1]);
    runProgram(buffer);
  } else {
    // REPL mode
    runRepl();
  }
}

int main(int argc, char **argv) { readFileOrPrompt(argc, argv); }
