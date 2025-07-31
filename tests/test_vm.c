#include "../compiler/compiler.h"
#include "../lexer/lexer.h"
#include "../object/object.h"
#include "../parser/parser.h"
#include "../vm/vm.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void testIntegerArithmetic() {
  printf("Testing integer arithmetic...\n");

  // Test 1 + 2
  char *input = "1 + 2";
  Lexer *lexer = newLexer(input);
  Parser *parser = newParser(lexer);
  Program *program = parseProgram(parser);

  Compiler *compiler = newCompiler();
  int result = compileProgram(compiler, program);
  assert(result == 0);

  ByteCode *bytecode = getByteCode(compiler);
  VM *vm = newVM(bytecode);

  result = run(vm);
  assert(result == 0);

  Object *top = stackTop(vm);
  assert(top != NULL);
  assert(strcmp(top->type, IntegerObj) == 0);
  assert(top->integer->value == 3);

  freeVM(vm);
  printf("✓ Integer arithmetic test passed\n");
}

void testBooleanExpressions() {
  printf("Testing boolean expressions...\n");

  // Test true
  char *input = "true";
  Lexer *lexer = newLexer(input);
  Parser *parser = newParser(lexer);
  Program *program = parseProgram(parser);

  Compiler *compiler = newCompiler();
  int result = compileProgram(compiler, program);
  assert(result == 0);

  ByteCode *bytecode = getByteCode(compiler);
  VM *vm = newVM(bytecode);

  result = run(vm);
  assert(result == 0);

  Object *top = stackTop(vm);
  assert(top != NULL);
  assert(strcmp(top->type, BooleanObj) == 0);
  assert(top->boolean->value == true);

  freeVM(vm);
  printf("✓ Boolean expressions test passed\n");
}

void testComparisons() {
  printf("Testing comparisons...\n");

  // Test 1 < 2
  char *input = "1 < 2";
  Lexer *lexer = newLexer(input);
  Parser *parser = newParser(lexer);
  Program *program = parseProgram(parser);

  Compiler *compiler = newCompiler();
  int result = compileProgram(compiler, program);
  assert(result == 0);

  ByteCode *bytecode = getByteCode(compiler);
  VM *vm = newVM(bytecode);

  result = run(vm);
  assert(result == 0);

  Object *top = stackTop(vm);
  assert(top != NULL);
  assert(strcmp(top->type, BooleanObj) == 0);
  assert(top->boolean->value == true);

  freeVM(vm);
  printf("✓ Comparisons test passed\n");
}

void testBangOperator() {
  printf("Testing bang operator...\n");

  // Test !true
  char *input = "!true";
  Lexer *lexer = newLexer(input);
  Parser *parser = newParser(lexer);
  Program *program = parseProgram(parser);

  Compiler *compiler = newCompiler();
  int result = compileProgram(compiler, program);
  assert(result == 0);

  ByteCode *bytecode = getByteCode(compiler);
  VM *vm = newVM(bytecode);

  result = run(vm);
  assert(result == 0);

  Object *top = stackTop(vm);
  assert(top != NULL);
  assert(strcmp(top->type, BooleanObj) == 0);
  assert(top->boolean->value == false);

  freeVM(vm);
  printf("✓ Bang operator test passed\n");
}

void testMinusOperator() {
  printf("Testing minus operator...\n");

  // Test -1
  char *input = "-1";
  Lexer *lexer = newLexer(input);
  Parser *parser = newParser(lexer);
  Program *program = parseProgram(parser);

  Compiler *compiler = newCompiler();
  int result = compileProgram(compiler, program);
  assert(result == 0);

  ByteCode *bytecode = getByteCode(compiler);
  VM *vm = newVM(bytecode);

  result = run(vm);
  assert(result == 0);

  Object *top = stackTop(vm);
  assert(top != NULL);
  assert(strcmp(top->type, IntegerObj) == 0);
  assert(top->integer->value == -1);

  freeVM(vm);
  printf("✓ Minus operator test passed\n");
}

void testGlobalVariables() {
  printf("Testing global variables...\n");

  // Test let one = 1; one;
  char *input = "let one = 1; one;";
  Lexer *lexer = newLexer(input);
  Parser *parser = newParser(lexer);
  Program *program = parseProgram(parser);

  Compiler *compiler = newCompiler();
  int result = compileProgram(compiler, program);
  assert(result == 0);

  ByteCode *bytecode = getByteCode(compiler);
  VM *vm = newVM(bytecode);

  result = run(vm);
  assert(result == 0);

  Object *top = stackTop(vm);
  assert(top != NULL);
  assert(strcmp(top->type, IntegerObj) == 0);
  assert(top->integer->value == 1);

  freeVM(vm);
  printf("✓ Global variables test passed\n");
}

void testArrayLiterals() {
  printf("Testing array literals...\n");

  // Test [1, 2, 3]
  char *input = "[1, 2, 3]";
  Lexer *lexer = newLexer(input);
  Parser *parser = newParser(lexer);
  Program *program = parseProgram(parser);

  Compiler *compiler = newCompiler();
  int result = compileProgram(compiler, program);
  assert(result == 0);

  ByteCode *bytecode = getByteCode(compiler);
  VM *vm = newVM(bytecode);

  result = run(vm);
  assert(result == 0);

  Object *top = stackTop(vm);
  assert(top != NULL);
  assert(strcmp(top->type, ArrayObj) == 0);
  assert(top->array->count == 3);

  freeVM(vm);
  printf("✓ Array literals test passed\n");
}

void testBasicFunctionCalls() {
  printf("Testing basic function calls...\n");

  // Test a simple function call that should work with our implementation
  char *input = "let fivePlusTen = fn() { 5 + 10; }; fivePlusTen();";
  Lexer *lexer = newLexer(input);
  Parser *parser = newParser(lexer);
  Program *program = parseProgram(parser);

  Compiler *compiler = newCompiler();
  int result = compileProgram(compiler, program);
  assert(result == 0);

  ByteCode *bytecode = getByteCode(compiler);
  VM *vm = newVM(bytecode);

  result = run(vm);
  assert(result == 0);

  Object *top = stackTop(vm);
  assert(top != NULL);
  assert(strcmp(top->type, IntegerObj) == 0);
  assert(top->integer->value == 15);

  freeVM(vm);
  printf("✓ Basic function calls test passed\n");
}

void testComplexProgram() {
  const char *input = "let getAge = fn(user) {\n"
                      "  return user[\"age\"];\n"
                      "};\n"
                      "let person = {\"name\": \"Alice\", \"age\": 30};\n"
                      "let age = getAge(person);\n"
                      "age;";

  Lexer *lexer = newLexer((char *)input);
  Parser *parser = newParser(lexer);
  Program *program = parseProgram(parser);
  Compiler *compiler = newCompiler();
  int result = compileProgram(compiler, program);
  ByteCode *bytecode = getByteCode(compiler);
  VM *vm = newVM(bytecode);
  result = run(vm);

  printf("🗂️ Source program: \n%s\n\n", input);

  printf("\n🎬 Result:\n");
  Object *top = stackTop(vm);
  assert(top != NULL);
  printObject(top);

  free(bytecode);
  freeProgram(program);
  freeParser(parser);
  free(lexer);
}

int main() {
  printf("🚀 Starting VM tests...\n\n");
  testIntegerArithmetic();
  testBooleanExpressions();
  testComparisons();
  testBangOperator();
  testMinusOperator();
  testGlobalVariables();
  testArrayLiterals();
  testBasicFunctionCalls();
  testComplexProgram();

  printf("\n🎉 All VM tests passed!\n");
  return 0;
}
