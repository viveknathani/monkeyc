#ifndef COMPILER_H
#define COMPILER_H

#include "../ast/ast.h"
#include "../object/object.h"
#include "../symbol/symbol.h"

typedef struct {
  Instructions instructions;
  Object *constants;
  int constantsCount;
  int constantsCapacity;
} ByteCode;

typedef struct {
  OpCode opCode;
  int position;
} EmittedInstruction;

typedef struct {
  Instructions instructions;
  int instructionsLength;
  int instructionsCapacity;
  EmittedInstruction lastInstruction;
  EmittedInstruction previousInstruction;
} CompilationScope;

typedef struct {
  Instructions instructions;
  Object *constants;
  int constantsCount;
  int constantsCapacity;
  EmittedInstruction lastInstruction;
  EmittedInstruction previousInstruction;
  SymbolTable *symbolTable;
  CompilationScope *scopes;
  int scopesLength;
  int scopesCapacity;
  int scopeIndex;
} Compiler;

Compiler *newCompiler();
Compiler *newCompilerWithState(SymbolTable *symbolTable, Object *constants);
int compileProgram(Compiler *compiler, Program *program);
int compileStatement(Compiler *compiler, Statement *statement);
ByteCode *getByteCode(Compiler *compiler);
int getByteCodeInstructionsLength(Compiler *compiler);

#endif
