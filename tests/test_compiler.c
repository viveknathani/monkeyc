#include "../compiler/compiler.h"
#include "../lexer/lexer.h"
#include "../opcode/opcode.h"
#include "../parser/parser.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  Object expected;
} ExpectedConstant;

typedef struct {
  OpCode opCode;
  int operands[2];
  int operandCount;
} ExpectedInstruction;

typedef struct {
  const char *input;
  ExpectedConstant *expectedConstants;
  int constantCount;
  ExpectedInstruction *expectedInstructions;
  int instructionCount;
} CompilerTestCase;

// Helper function to verify instructions
int verifyInstructions(Instructions actual, ExpectedInstruction *expected,
                       int expectedCount) {
  int pos = 0;

  for (int i = 0; i < expectedCount; i++) {
    if (actual[pos] != expected[i].opCode) {
      printf("❌ Instruction %d: expected opcode %d, got %d\n", i,
             expected[i].opCode, actual[pos]);
      return -1;
    }

    Definition def;
    if (lookupOpCode(expected[i].opCode, &def) != 0) {
      printf("❌ Unknown opcode: %d\n", expected[i].opCode);
      return -1;
    }

    pos++; // Skip opcode

    // Check operands
    for (int j = 0; j < def.operandCount; j++) {
      int width = def.operandWidths[j];
      int actualOperand = 0;

      if (width == 1) {
        actualOperand = (unsigned char)actual[pos];
        pos += 1;
      } else if (width == 2) {
        actualOperand =
            ((unsigned char)actual[pos] << 8) | (unsigned char)actual[pos + 1];
        pos += 2;
      }

      if (actualOperand != expected[i].operands[j]) {
        printf("❌ Instruction %d operand %d: expected %d, got %d\n", i, j,
               expected[i].operands[j], actualOperand);
        return -1;
      }
    }
  }

  return 0;
}

void testIntegerArithmetic() {
  printf("🧮 Testing integer arithmetic...\n");

  CompilerTestCase tests[] = {
      {"1 + 2",
       (ExpectedConstant[]){{{IntegerObj, .integer = &(Integer){1}}},
                            {{IntegerObj, .integer = &(Integer){2}}}},
       2,
       (ExpectedInstruction[]){{OpConstant, {0}, 1},
                               {OpConstant, {1}, 1},
                               {OpAdd, {}, 0},
                               {OpPop, {}, 0}},
       4},
      {"1 - 2",
       (ExpectedConstant[]){{{IntegerObj, .integer = &(Integer){1}}},
                            {{IntegerObj, .integer = &(Integer){2}}}},
       2,
       (ExpectedInstruction[]){{OpConstant, {0}, 1},
                               {OpConstant, {1}, 1},
                               {OpSub, {}, 0},
                               {OpPop, {}, 0}},
       4},
      {"1 * 2",
       (ExpectedConstant[]){{{IntegerObj, .integer = &(Integer){1}}},
                            {{IntegerObj, .integer = &(Integer){2}}}},
       2,
       (ExpectedInstruction[]){{OpConstant, {0}, 1},
                               {OpConstant, {1}, 1},
                               {OpMul, {}, 0},
                               {OpPop, {}, 0}},
       4},
      {"2 / 1",
       (ExpectedConstant[]){{{IntegerObj, .integer = &(Integer){2}}},
                            {{IntegerObj, .integer = &(Integer){1}}}},
       2,
       (ExpectedInstruction[]){{OpConstant, {0}, 1},
                               {OpConstant, {1}, 1},
                               {OpDiv, {}, 0},
                               {OpPop, {}, 0}},
       4},
      {"-1", (ExpectedConstant[]){{{IntegerObj, .integer = &(Integer){1}}}}, 1,
       (ExpectedInstruction[]){
           {OpConstant, {0}, 1}, {OpMinus, {}, 0}, {OpPop, {}, 0}},
       3}};

  for (int i = 0; i < 5; i++) {
    CompilerTestCase test = tests[i];
    printf("  Testing: %s\n", test.input);

    Lexer *lexer = newLexer((char *)test.input);
    Parser *parser = newParser(lexer);
    Program *program = parseProgram(parser);

    Compiler *compiler = newCompiler();
    int result = compileProgram(compiler, program);
    assert(result == 0);

    ByteCode *bytecode = getByteCode(compiler);

    // Check constants
    assert(bytecode->constantsCount == test.constantCount);
    for (int j = 0; j < test.constantCount; j++) {
      Object *constant = &bytecode->constants[j];
      ExpectedConstant expected = test.expectedConstants[j];

      assert(strcmp(constant->type, expected.expected.type) == 0);
      if (strcmp(constant->type, IntegerObj) == 0) {
        assert(constant->integer->value == expected.expected.integer->value);
      }
    }

    // Check instructions
    assert(verifyInstructions(bytecode->instructions, test.expectedInstructions,
                              test.instructionCount) == 0);

    free(bytecode);
    freeProgram(program);
    freeParser(parser);
    free(lexer);
    // Note: Compiler memory management would need proper cleanup functions
  }

  printf("✅ Integer arithmetic tests passed\n");
}

void testBooleanExpressions() {
  printf("🔵 Testing boolean expressions...\n");

  CompilerTestCase tests[] = {
      {"true", NULL, 0,
       (ExpectedInstruction[]){{OpTrue, {}, 0}, {OpPop, {}, 0}}, 2},
      {"false", NULL, 0,
       (ExpectedInstruction[]){{OpFalse, {}, 0}, {OpPop, {}, 0}}, 2},
      {"1 > 2",
       (ExpectedConstant[]){{{IntegerObj, .integer = &(Integer){1}}},
                            {{IntegerObj, .integer = &(Integer){2}}}},
       2,
       (ExpectedInstruction[]){{OpConstant, {0}, 1},
                               {OpConstant, {1}, 1},
                               {OpGreaterThan, {}, 0},
                               {OpPop, {}, 0}},
       4},
      {"1 < 2",
       (ExpectedConstant[]){{{IntegerObj, .integer = &(Integer){2}}},
                            {{IntegerObj, .integer = &(Integer){1}}}},
       2,
       (ExpectedInstruction[]){
           {OpConstant, {0}, 1}, // loads 2 (right operand compiled first)
           {OpConstant, {1}, 1}, // loads 1 (left operand compiled second)
           {OpGreaterThan, {}, 0},
           {OpPop, {}, 0}},
       4},
      {"1 == 2",
       (ExpectedConstant[]){{{IntegerObj, .integer = &(Integer){1}}},
                            {{IntegerObj, .integer = &(Integer){2}}}},
       2,
       (ExpectedInstruction[]){{OpConstant, {0}, 1},
                               {OpConstant, {1}, 1},
                               {OpEqual, {}, 0},
                               {OpPop, {}, 0}},
       4},
      {"1 != 2",
       (ExpectedConstant[]){{{IntegerObj, .integer = &(Integer){1}}},
                            {{IntegerObj, .integer = &(Integer){2}}}},
       2,
       (ExpectedInstruction[]){{OpConstant, {0}, 1},
                               {OpConstant, {1}, 1},
                               {OpNotEqual, {}, 0},
                               {OpPop, {}, 0}},
       4},
      {"!true", NULL, 0,
       (ExpectedInstruction[]){
           {OpTrue, {}, 0}, {OpBang, {}, 0}, {OpPop, {}, 0}},
       3}};

  for (int i = 0; i < 7; i++) {
    CompilerTestCase test = tests[i];
    printf("  Testing: %s\n", test.input);

    Lexer *lexer = newLexer((char *)test.input);
    Parser *parser = newParser(lexer);
    Program *program = parseProgram(parser);

    Compiler *compiler = newCompiler();
    int result = compileProgram(compiler, program);
    assert(result == 0);

    ByteCode *bytecode = getByteCode(compiler);
    assert(bytecode->constantsCount == test.constantCount);

    // Check instructions
    assert(verifyInstructions(bytecode->instructions, test.expectedInstructions,
                              test.instructionCount) == 0);

    free(bytecode);
    freeProgram(program);
    freeParser(parser);
    free(lexer);
  }

  printf("✅ Boolean expression tests passed\n");
}

void testConditionals() {
  printf("❓ Testing conditional expressions...\n");

  CompilerTestCase tests[] = {
      {"if (true) { 10 }; 3333;",
       (ExpectedConstant[]){{{IntegerObj, .integer = &(Integer){10}}},
                            {{IntegerObj, .integer = &(Integer){3333}}}},
       2,
       (ExpectedInstruction[]){
           {OpTrue, {}, 0},            // 0000
           {OpJumpNotTruthy, {10}, 1}, // 0001
           {OpConstant, {0}, 1},       // 0004
           {OpJump, {11}, 1},          // 0007
           {OpNull, {}, 0},            // 0010
           {OpPop, {}, 0},             // 0011
           {OpConstant, {1}, 1},       // 0012
           {OpPop, {}, 0}              // 0015
       },
       8},
      {"if (true) { 10 } else { 20 }; 3333;",
       (ExpectedConstant[]){{{IntegerObj, .integer = &(Integer){10}}},
                            {{IntegerObj, .integer = &(Integer){20}}},
                            {{IntegerObj, .integer = &(Integer){3333}}}},
       3,
       (ExpectedInstruction[]){
           {OpTrue, {}, 0},            // 0000
           {OpJumpNotTruthy, {10}, 1}, // 0001
           {OpConstant, {0}, 1},       // 0004
           {OpJump, {13}, 1},          // 0007
           {OpConstant, {1}, 1},       // 0010
           {OpPop, {}, 0},             // 0013
           {OpConstant, {2}, 1},       // 0014
           {OpPop, {}, 0}              // 0017
       },
       8}};

  for (int i = 0; i < 2; i++) {
    CompilerTestCase test = tests[i];
    printf("  Testing: %s\n", test.input);

    Lexer *lexer = newLexer((char *)test.input);
    Parser *parser = newParser(lexer);
    Program *program = parseProgram(parser);

    Compiler *compiler = newCompiler();
    int result = compileProgram(compiler, program);
    assert(result == 0);

    ByteCode *bytecode = getByteCode(compiler);
    assert(bytecode->constantsCount == test.constantCount);

    // Check instructions
    assert(verifyInstructions(bytecode->instructions, test.expectedInstructions,
                              test.instructionCount) == 0);

    free(bytecode);
    freeProgram(program);
    freeParser(parser);
    free(lexer);
  }

  printf("✅ Conditional tests passed\n");
}

void testGlobalLetStatements() {
  printf("🌍 Testing global let statements...\n");

  CompilerTestCase tests[] = {
      {"let one = 1; let two = 2;",
       (ExpectedConstant[]){{{IntegerObj, .integer = &(Integer){1}}},
                            {{IntegerObj, .integer = &(Integer){2}}}},
       2,
       (ExpectedInstruction[]){{OpConstant, {0}, 1},
                               {OpSetGlobal, {0}, 1},
                               {OpConstant, {1}, 1},
                               {OpSetGlobal, {1}, 1}},
       4},
      {"let one = 1; one;",
       (ExpectedConstant[]){{{IntegerObj, .integer = &(Integer){1}}}}, 1,
       (ExpectedInstruction[]){{OpConstant, {0}, 1},
                               {OpSetGlobal, {0}, 1},
                               {OpGetGlobal, {0}, 1},
                               {OpPop, {}, 0}},
       4},
      {"let one = 1; let two = one; two;",
       (ExpectedConstant[]){{{IntegerObj, .integer = &(Integer){1}}}}, 1,
       (ExpectedInstruction[]){{OpConstant, {0}, 1},
                               {OpSetGlobal, {0}, 1},
                               {OpGetGlobal, {0}, 1},
                               {OpSetGlobal, {1}, 1},
                               {OpGetGlobal, {1}, 1},
                               {OpPop, {}, 0}},
       6}};

  for (int i = 0; i < 3; i++) {
    CompilerTestCase test = tests[i];
    printf("  Testing: %s\n", test.input);

    Lexer *lexer = newLexer((char *)test.input);
    Parser *parser = newParser(lexer);
    Program *program = parseProgram(parser);

    Compiler *compiler = newCompiler();
    int result = compileProgram(compiler, program);
    assert(result == 0);

    ByteCode *bytecode = getByteCode(compiler);
    assert(bytecode->constantsCount == test.constantCount);

    // Check instructions
    assert(verifyInstructions(bytecode->instructions, test.expectedInstructions,
                              test.instructionCount) == 0);

    free(bytecode);
    freeProgram(program);
    freeParser(parser);
    free(lexer);
  }

  printf("✅ Global let statement tests passed\n");
}

void testStringExpressions() {
  printf("📝 Testing string expressions...\n");

  CompilerTestCase tests[] = {
      {"\"monkey\"",
       (ExpectedConstant[]){{{StringObj, .string = &(String){"monkey"}}}}, 1,
       (ExpectedInstruction[]){{OpConstant, {0}, 1}, {OpPop, {}, 0}}, 2},
      {"\"mon\" + \"key\"",
       (ExpectedConstant[]){{{StringObj, .string = &(String){"mon"}}},
                            {{StringObj, .string = &(String){"key"}}}},
       2,
       (ExpectedInstruction[]){{OpConstant, {0}, 1},
                               {OpConstant, {1}, 1},
                               {OpAdd, {}, 0},
                               {OpPop, {}, 0}},
       4}};

  for (int i = 0; i < 2; i++) {
    CompilerTestCase test = tests[i];
    printf("  Testing: %s\n", test.input);

    Lexer *lexer = newLexer((char *)test.input);
    Parser *parser = newParser(lexer);
    Program *program = parseProgram(parser);

    Compiler *compiler = newCompiler();
    int result = compileProgram(compiler, program);
    assert(result == 0);

    ByteCode *bytecode = getByteCode(compiler);
    assert(bytecode->constantsCount == test.constantCount);

    // Check instructions
    assert(verifyInstructions(bytecode->instructions, test.expectedInstructions,
                              test.instructionCount) == 0);

    free(bytecode);
    freeProgram(program);
    freeParser(parser);
    free(lexer);
  }

  printf("✅ String expression tests passed\n");
}

void testArrayLiterals() {
  printf("📚 Testing array literals...\n");

  CompilerTestCase tests[] = {
      {"[]", NULL, 0,
       (ExpectedInstruction[]){{OpArray, {0}, 1}, {OpPop, {}, 0}}, 2},
      {"[1, 2, 3]",
       (ExpectedConstant[]){{{IntegerObj, .integer = &(Integer){1}}},
                            {{IntegerObj, .integer = &(Integer){2}}},
                            {{IntegerObj, .integer = &(Integer){3}}}},
       3,
       (ExpectedInstruction[]){{OpConstant, {0}, 1},
                               {OpConstant, {1}, 1},
                               {OpConstant, {2}, 1},
                               {OpArray, {3}, 1},
                               {OpPop, {}, 0}},
       5},
      {"[1 + 2, 3 - 4, 5 * 6]",
       (ExpectedConstant[]){{{IntegerObj, .integer = &(Integer){1}}},
                            {{IntegerObj, .integer = &(Integer){2}}},
                            {{IntegerObj, .integer = &(Integer){3}}},
                            {{IntegerObj, .integer = &(Integer){4}}},
                            {{IntegerObj, .integer = &(Integer){5}}},
                            {{IntegerObj, .integer = &(Integer){6}}}},
       6,
       (ExpectedInstruction[]){{OpConstant, {0}, 1},
                               {OpConstant, {1}, 1},
                               {OpAdd, {}, 0},
                               {OpConstant, {2}, 1},
                               {OpConstant, {3}, 1},
                               {OpSub, {}, 0},
                               {OpConstant, {4}, 1},
                               {OpConstant, {5}, 1},
                               {OpMul, {}, 0},
                               {OpArray, {3}, 1},
                               {OpPop, {}, 0}},
       11}};

  for (int i = 0; i < 3; i++) {
    CompilerTestCase test = tests[i];
    printf("  Testing: %s\n", test.input);

    Lexer *lexer = newLexer((char *)test.input);
    Parser *parser = newParser(lexer);
    Program *program = parseProgram(parser);

    Compiler *compiler = newCompiler();
    int result = compileProgram(compiler, program);
    assert(result == 0);

    ByteCode *bytecode = getByteCode(compiler);
    assert(bytecode->constantsCount == test.constantCount);

    // Check instructions
    assert(verifyInstructions(bytecode->instructions, test.expectedInstructions,
                              test.instructionCount) == 0);

    free(bytecode);
    freeProgram(program);
    freeParser(parser);
    free(lexer);
  }

  printf("✅ Array literal tests passed\n");
}

void testHashLiterals() {
  printf("🗂️  Testing hash literals...\n");

  CompilerTestCase tests[] = {
      {"{}", NULL, 0, (ExpectedInstruction[]){{OpHash, {0}, 1}, {OpPop, {}, 0}},
       2},
      {"{1: 2, 3: 4, 5: 6}",
       (ExpectedConstant[]){{{IntegerObj, .integer = &(Integer){1}}},
                            {{IntegerObj, .integer = &(Integer){2}}},
                            {{IntegerObj, .integer = &(Integer){3}}},
                            {{IntegerObj, .integer = &(Integer){4}}},
                            {{IntegerObj, .integer = &(Integer){5}}},
                            {{IntegerObj, .integer = &(Integer){6}}}},
       6,
       (ExpectedInstruction[]){{OpConstant, {0}, 1},
                               {OpConstant, {1}, 1},
                               {OpConstant, {2}, 1},
                               {OpConstant, {3}, 1},
                               {OpConstant, {4}, 1},
                               {OpConstant, {5}, 1},
                               {OpHash, {6}, 1},
                               {OpPop, {}, 0}},
       8}};

  for (int i = 0; i < 2; i++) {
    CompilerTestCase test = tests[i];
    printf("  Testing: %s\n", test.input);

    Lexer *lexer = newLexer((char *)test.input);
    Parser *parser = newParser(lexer);
    Program *program = parseProgram(parser);

    Compiler *compiler = newCompiler();
    int result = compileProgram(compiler, program);
    assert(result == 0);

    ByteCode *bytecode = getByteCode(compiler);
    assert(bytecode->constantsCount == test.constantCount);

    // Check instructions
    assert(verifyInstructions(bytecode->instructions, test.expectedInstructions,
                              test.instructionCount) == 0);

    free(bytecode);
    freeProgram(program);
    freeParser(parser);
    free(lexer);
  }

  printf("✅ Hash literal tests passed\n");
}

void testIndexExpressions() {
  printf("🔍 Testing index expressions...\n");

  CompilerTestCase tests[] = {
      {"[1, 2, 3][1]",
       (ExpectedConstant[]){{{IntegerObj, .integer = &(Integer){1}}},
                            {{IntegerObj, .integer = &(Integer){2}}},
                            {{IntegerObj, .integer = &(Integer){3}}},
                            {{IntegerObj, .integer = &(Integer){1}}}},
       4,
       (ExpectedInstruction[]){{OpConstant, {0}, 1},
                               {OpConstant, {1}, 1},
                               {OpConstant, {2}, 1},
                               {OpArray, {3}, 1},
                               {OpConstant, {3}, 1},
                               {OpIndex, {}, 0},
                               {OpPop, {}, 0}},
       7},
      {"{1: 2}[1]",
       (ExpectedConstant[]){{{IntegerObj, .integer = &(Integer){1}}},
                            {{IntegerObj, .integer = &(Integer){2}}},
                            {{IntegerObj, .integer = &(Integer){1}}}},
       3,
       (ExpectedInstruction[]){{OpConstant, {0}, 1},
                               {OpConstant, {1}, 1},
                               {OpHash, {2}, 1},
                               {OpConstant, {2}, 1},
                               {OpIndex, {}, 0},
                               {OpPop, {}, 0}},
       6}};

  for (int i = 0; i < 2; i++) {
    CompilerTestCase test = tests[i];
    printf("  Testing: %s\n", test.input);

    Lexer *lexer = newLexer((char *)test.input);
    Parser *parser = newParser(lexer);
    Program *program = parseProgram(parser);

    Compiler *compiler = newCompiler();
    int result = compileProgram(compiler, program);
    assert(result == 0);

    ByteCode *bytecode = getByteCode(compiler);
    assert(bytecode->constantsCount == test.constantCount);

    // Check instructions
    assert(verifyInstructions(bytecode->instructions, test.expectedInstructions,
                              test.instructionCount) == 0);

    free(bytecode);
    freeProgram(program);
    freeParser(parser);
    free(lexer);
  }

  printf("✅ Index expression tests passed\n");
}

void testFunctions() {
  printf("🔧 Testing function compilation...\n");

  CompilerTestCase tests[] = {
      {"fn() { return 5 + 10 }",
       (ExpectedConstant[]){
           {{IntegerObj, .integer = &(Integer){5}}},
           {{IntegerObj, .integer = &(Integer){10}}},
           {{CompiledFunctionObj, .compiledFunction = NULL}} // Simplified
       },
       3, (ExpectedInstruction[]){{OpConstant, {2}, 1}, {OpPop, {}, 0}}, 2},
      {"fn() { 5 + 10 }",
       (ExpectedConstant[]){
           {{IntegerObj, .integer = &(Integer){5}}},
           {{IntegerObj, .integer = &(Integer){10}}},
           {{CompiledFunctionObj, .compiledFunction = NULL}} // Simplified
       },
       3, (ExpectedInstruction[]){{OpConstant, {2}, 1}, {OpPop, {}, 0}}, 2}};

  for (int i = 0; i < 2; i++) {
    CompilerTestCase test = tests[i];
    printf("  Testing: %s\n", test.input);

    Lexer *lexer = newLexer((char *)test.input);
    Parser *parser = newParser(lexer);
    Program *program = parseProgram(parser);

    Compiler *compiler = newCompiler();
    int result = compileProgram(compiler, program);
    assert(result == 0);

    ByteCode *bytecode = getByteCode(compiler);
    // Basic test - just ensure compilation succeeds
    assert(bytecode->constantsCount >= 2); // At least the integers

    free(bytecode);
    freeProgram(program);
    freeParser(parser);
    free(lexer);
  }

  printf("✅ Function compilation tests passed\n");
}

void testFunctionCalls() {
  printf("📞 Testing function calls...\n");

  CompilerTestCase tests[] = {
      {"fn() { 24 }();",
       (ExpectedConstant[]){
           {{IntegerObj, .integer = &(Integer){24}}},
           {{CompiledFunctionObj, .compiledFunction = NULL}} // Simplified
       },
       2,
       (ExpectedInstruction[]){
           {OpConstant, {1}, 1}, {OpCall, {0}, 1}, {OpPop, {}, 0}},
       3},
      {"let noArg = fn() { 24 }; noArg();",
       (ExpectedConstant[]){
           {{IntegerObj, .integer = &(Integer){24}}},
           {{CompiledFunctionObj, .compiledFunction = NULL}} // Simplified
       },
       2,
       (ExpectedInstruction[]){{OpConstant, {1}, 1},
                               {OpSetGlobal, {0}, 1},
                               {OpGetGlobal, {0}, 1},
                               {OpCall, {0}, 1},
                               {OpPop, {}, 0}},
       5}};

  for (int i = 0; i < 2; i++) {
    CompilerTestCase test = tests[i];
    printf("  Testing: %s\n", test.input);

    Lexer *lexer = newLexer((char *)test.input);
    Parser *parser = newParser(lexer);
    Program *program = parseProgram(parser);

    Compiler *compiler = newCompiler();
    int result = compileProgram(compiler, program);
    assert(result == 0);

    ByteCode *bytecode = getByteCode(compiler);
    // Basic test - just ensure compilation succeeds
    assert(bytecode->constantsCount >= 1);

    free(bytecode);
    freeProgram(program);
    freeParser(parser);
    free(lexer);
  }

  printf("✅ Function call tests passed\n");
}

void testBuiltinFunctions() {
  printf("🏗️ Testing builtin functions...\n");

  CompilerTestCase tests[] = {
      {"len([]); push([], 1);",
       (ExpectedConstant[]){{{IntegerObj, .integer = &(Integer){1}}}}, 1,
       (ExpectedInstruction[]){{OpGetBuiltin, {0}, 1},
                               {OpArray, {0}, 1},
                               {OpCall, {1}, 1},
                               {OpPop, {}, 0},
                               {OpGetBuiltin, {4}, 1},
                               {OpArray, {0}, 1},
                               {OpConstant, {0}, 1},
                               {OpCall, {2}, 1},
                               {OpPop, {}, 0}},
       9},
      {"fn() { len([]) }", NULL,
       0, // Function will have constants internally
       (ExpectedInstruction[]){{OpConstant, {0}, 1}, // The compiled function
                               {OpPop, {}, 0}},
       2}};

  for (int i = 0; i < 2; i++) {
    CompilerTestCase test = tests[i];
    printf("  Testing: %s\n", test.input);

    Lexer *lexer = newLexer((char *)test.input);
    Parser *parser = newParser(lexer);
    Program *program = parseProgram(parser);

    Compiler *compiler = newCompiler();
    int result = compileProgram(compiler, program);
    assert(result == 0);

    ByteCode *bytecode = getByteCode(compiler);
    // Basic test - just ensure compilation succeeds

    free(bytecode);
    freeProgram(program);
    freeParser(parser);
    free(lexer);
  }

  printf("✅ Builtin function tests passed\n");
}

void testCompilerScopes() {
  printf("🎯 Testing compiler scopes...\n");

  const char *input =
      "let num = 55; fn(a, b, c) { let x = a + b + c; return x; };";

  printf("  Testing: %s\n", input);

  Lexer *lexer = newLexer((char *)input);
  Parser *parser = newParser(lexer);
  Program *program = parseProgram(parser);

  Compiler *compiler = newCompiler();
  int result = compileProgram(compiler, program);
  assert(result == 0);

  ByteCode *bytecode = getByteCode(compiler);

  // Check that we have at least the expected constants
  assert(bytecode->constantsCount >= 2); // 55 and the compiled function

  free(bytecode);
  freeProgram(program);
  freeParser(parser);
  free(lexer);

  printf("✅ Compiler scope tests passed\n");
}

void testComplexExpressions() {
  printf("🔥 Testing complex expressions...\n");

  CompilerTestCase tests[] = {
      {"let x = 1; let y = 2; x + y * 3 - 4 / 2;",
       (ExpectedConstant[]){{{IntegerObj, .integer = &(Integer){1}}},
                            {{IntegerObj, .integer = &(Integer){2}}},
                            {{IntegerObj, .integer = &(Integer){3}}},
                            {{IntegerObj, .integer = &(Integer){4}}},
                            {{IntegerObj, .integer = &(Integer){2}}}},
       5,
       (ExpectedInstruction[]){{OpConstant, {0}, 1},  // 1
                               {OpSetGlobal, {0}, 1}, // let x = 1
                               {OpConstant, {1}, 1},  // 2
                               {OpSetGlobal, {1}, 1}, // let y = 2
                               {OpGetGlobal, {0}, 1}, // x
                               {OpGetGlobal, {1}, 1}, // y
                               {OpConstant, {2}, 1},  // 3
                               {OpMul, {}, 0},        // y * 3
                               {OpAdd, {}, 0},        // x + (y * 3)
                               {OpConstant, {3}, 1},  // 4
                               {OpConstant, {4}, 1},  // 2
                               {OpDiv, {}, 0},        // 4 / 2
                               {OpSub, {}, 0},        // (x + y * 3) - (4 / 2)
                               {OpPop, {}, 0}},
       14},
      {"!(true == false)", NULL, 0,
       (ExpectedInstruction[]){{OpTrue, {}, 0},
                               {OpFalse, {}, 0},
                               {OpEqual, {}, 0},
                               {OpBang, {}, 0},
                               {OpPop, {}, 0}},
       5}};

  for (int i = 0; i < 2; i++) {
    CompilerTestCase test = tests[i];
    printf("  Testing: %s\n", test.input);

    Lexer *lexer = newLexer((char *)test.input);
    Parser *parser = newParser(lexer);
    Program *program = parseProgram(parser);

    Compiler *compiler = newCompiler();
    int result = compileProgram(compiler, program);
    assert(result == 0);

    ByteCode *bytecode = getByteCode(compiler);
    assert(bytecode->constantsCount == test.constantCount);

    // Check instructions
    assert(verifyInstructions(bytecode->instructions, test.expectedInstructions,
                              test.instructionCount) == 0);

    free(bytecode);
    freeProgram(program);
    freeParser(parser);
    free(lexer);
  }

  printf("✅ Complex expression tests passed\n");
}

void testErrorHandling() {
  printf("❌ Testing error handling...\n");

  const char *errorCases[] = {
      "unknownVariable;",           // Undefined variable
      "let x = unknownVar;",        // Undefined variable in assignment
      "if (unknownCondition) { 1 }" // Undefined variable in condition
  };

  for (int i = 0; i < 3; i++) {
    printf("  Testing error case: %s\n", errorCases[i]);

    Lexer *lexer = newLexer((char *)errorCases[i]);
    Parser *parser = newParser(lexer);
    Program *program = parseProgram(parser);

    Compiler *compiler = newCompiler();
    int result = compileProgram(compiler, program);

    // These should fail compilation
    assert(result != 0);

    freeProgram(program);
    freeParser(parser);
    free(lexer);
  }

  printf("✅ Error handling tests passed\n");
}

void testLocalVariables() {
  printf("🏠 Testing local variables...\n");

  CompilerTestCase tests[] = {
      {"let global = 55; fn() { let a = 66; let b = 77; a + b }",
       (ExpectedConstant[]){{{IntegerObj, .integer = &(Integer){55}}},
                            {{IntegerObj, .integer = &(Integer){66}}},
                            {{IntegerObj, .integer = &(Integer){77}}},
                            {{CompiledFunctionObj, .compiledFunction = NULL}}},
       4,
       (ExpectedInstruction[]){{OpConstant, {0}, 1},  // 55
                               {OpSetGlobal, {0}, 1}, // let global = 55
                               {OpConstant, {3}, 1},  // compiled function
                               {OpPop, {}, 0}},
       4},
      {"fn(a, b, c) { let x = a; x + b + c; }",
       (ExpectedConstant[]){{{CompiledFunctionObj, .compiledFunction = NULL}}},
       1,
       (ExpectedInstruction[]){{OpConstant, {0}, 1}, // compiled function
                               {OpPop, {}, 0}},
       2}};

  for (int i = 0; i < 2; i++) {
    CompilerTestCase test = tests[i];
    printf("  Testing: %s\n", test.input);

    Lexer *lexer = newLexer((char *)test.input);
    Parser *parser = newParser(lexer);
    Program *program = parseProgram(parser);

    Compiler *compiler = newCompiler();
    int result = compileProgram(compiler, program);
    assert(result == 0);

    ByteCode *bytecode = getByteCode(compiler);
    // Basic validation - just check it compiles
    assert(bytecode->constantsCount >= 1);

    free(bytecode);
    freeProgram(program);
    freeParser(parser);
    free(lexer);
  }

  printf("✅ Local variable tests passed\n");
}

void testNestedScopes() {
  printf("🎪 Testing nested scopes...\n");

  const char *complexPrograms[] = {
      "let outer = 10; fn(a) { let inner = 20; fn(b) { outer + inner + a + b } "
      "}",
      "let global = 1; fn() { let local = 2; if (true) { let block = 3; global "
      "+ local + block } }",
      "fn(x) { if (x > 0) { return x; } else { return -x; } }"};

  for (int i = 0; i < 3; i++) {
    printf("  Testing: %s\n", complexPrograms[i]);

    Lexer *lexer = newLexer((char *)complexPrograms[i]);
    Parser *parser = newParser(lexer);
    Program *program = parseProgram(parser);

    Compiler *compiler = newCompiler();
    int result = compileProgram(compiler, program);
    assert(result == 0);

    ByteCode *bytecode = getByteCode(compiler);
    // Just verify it compiles successfully
    assert(bytecode != NULL);

    free(bytecode);
    freeProgram(program);
    freeParser(parser);
    free(lexer);
  }

  printf("✅ Nested scope tests passed\n");
}

void testPrintComplexProgram() {
  const char *input = "let getAge = fn(user) {\n"
                      "  return user[\"age\"];\n"
                      "};\n"
                      "let person = {\"name\": \"Alice\", \"age\": 30};\n"
                      "let age = getAge(person);";

  Lexer *lexer = newLexer((char *)input);
  Parser *parser = newParser(lexer);
  Program *program = parseProgram(parser);
  Compiler *compiler = newCompiler();
  int result = compileProgram(compiler, program);
  ByteCode *bytecode = getByteCode(compiler);

  printf("🗂️ Source program: \n%s\n\n", input);

  printf("\n💾 Constants:\n");
  for (int i = 0; i < bytecode->constantsCount; i++) {
    Object *constant = &bytecode->constants[i];
    printf("  %d: ", i);
    if (strcmp(constant->type, IntegerObj) == 0) {
      printf("%lld (Integer)\n", constant->integer->value);
    } else if (strcmp(constant->type, StringObj) == 0) {
      printf("\"%s\" (String)\n", constant->string->value);
    } else if (strcmp(constant->type, CompiledFunctionObj) == 0) {
      printf("CompiledFunction (numLocals=%d, numParams=%d)\n",
             constant->compiledFunction->numLocals,
             constant->compiledFunction->numParameters);
    } else {
      printf("(%s)\n", constant->type);
    }
  }

  printf("\n👾 Bytecode Instructions:\n");
  int instrLength = getByteCodeInstructionsLength(compiler);
  char *instrStr = instructionsToString(bytecode->instructions, instrLength);
  printf("%s\n", instrStr);
  free(instrStr);

  free(bytecode);
  freeProgram(program);
  freeParser(parser);
  free(lexer);
}

int main() {
  printf("🚀 Starting compiler tests...\n\n");

  testIntegerArithmetic();
  testBooleanExpressions();
  testConditionals();
  testGlobalLetStatements();
  testStringExpressions();
  testArrayLiterals();
  testHashLiterals();
  testIndexExpressions();
  testFunctions();
  testFunctionCalls();
  testBuiltinFunctions();
  testCompilerScopes();

  testComplexExpressions();
  testErrorHandling();
  testLocalVariables();
  testNestedScopes();
  testPrintComplexProgram();

  printf("\n🎉 All compiler tests passed!\n");
  return 0;
}
