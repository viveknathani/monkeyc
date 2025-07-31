#include "compiler.h"
#include "../opcode/opcode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CONSTANTS_CAPACITY 256
#define INITIAL_INSTRUCTIONS_CAPACITY 1024
#define INITIAL_SCOPES_CAPACITY 8

static int compileExpression(Compiler *compiler, Expression *expression);
static int compileBlockStatement(Compiler *compiler, BlockStatement *block);
static int emit(Compiler *compiler, OpCode opCode, int *operands,
                int operandCount);
static int addConstant(Compiler *compiler, Object *obj);
static void setLastInstruction(Compiler *compiler, OpCode opCode, int position);
static int lastInstructionIs(Compiler *compiler, OpCode opCode);
static void removeLastPop(Compiler *compiler);
static void replaceInstruction(Compiler *compiler, int pos,
                               Instructions newInstruction, int length);
static void changeOperand(Compiler *compiler, int opPos, int operand);
static Instructions getCurrentInstructions(Compiler *compiler);
static void enterScope(Compiler *compiler);
static Instructions leaveScope(Compiler *compiler, int *length);
static void replaceLastPopWithReturn(Compiler *compiler);
static void loadSymbol(Compiler *compiler, Symbol symbol);
static int addInstruction(Compiler *compiler, Instructions ins, int length);

Compiler *newCompiler() {
  Compiler *compiler = malloc(sizeof(Compiler));

  compiler->constants = malloc(sizeof(Object) * INITIAL_CONSTANTS_CAPACITY);
  compiler->constantsCount = 0;
  compiler->constantsCapacity = INITIAL_CONSTANTS_CAPACITY;

  compiler->symbolTable = newSymbolTable();

  // Define built-in functions
  BuiltinEntry *builtins = malloc(sizeof(BuiltinEntry) * 6);
  builtins[0] = (BuiltinEntry){"len", NULL};
  builtins[1] = (BuiltinEntry){"first", NULL};
  builtins[2] = (BuiltinEntry){"last", NULL};
  builtins[3] = (BuiltinEntry){"rest", NULL};
  builtins[4] = (BuiltinEntry){"push", NULL};
  builtins[5] = (BuiltinEntry){"puts", NULL};

  for (int i = 0; i < 6; i++) {
    defineBuiltin(compiler->symbolTable, builtins[i].name, i);
  }
  free(builtins);

  // Initialize scopes
  compiler->scopes = malloc(sizeof(CompilationScope) * INITIAL_SCOPES_CAPACITY);
  compiler->scopesLength = 1;
  compiler->scopesCapacity = INITIAL_SCOPES_CAPACITY;
  compiler->scopeIndex = 0;

  // Initialize main scope
  compiler->scopes[0].instructions = malloc(INITIAL_INSTRUCTIONS_CAPACITY);
  compiler->scopes[0].instructionsLength = 0;
  compiler->scopes[0].instructionsCapacity = INITIAL_INSTRUCTIONS_CAPACITY;
  compiler->scopes[0].lastInstruction = (EmittedInstruction){0, -1};
  compiler->scopes[0].previousInstruction = (EmittedInstruction){0, -1};

  return compiler;
}

Compiler *newCompilerWithState(SymbolTable *symbolTable, Object *constants) {
  Compiler *compiler = newCompiler();
  compiler->symbolTable = symbolTable;
  if (constants) {
    // Copy constants - assume they're already allocated
    // This is a simplified implementation
    compiler->constants = constants;
  }
  return compiler;
}

int compileProgram(Compiler *compiler, Program *program) {
  for (int i = 0; i < program->statementCount; i++) {
    if (compileStatement(compiler, program->statements[i]) != 0) {
      return -1;
    }
  }

  /*
   * If the last emitted instruction was a pop, remove it. This keeps the
   * result of the final expression on the stack so that the VM can inspect
   * it after execution (needed by the unit tests).
   */
  if (lastInstructionIs(compiler, OpPop)) {
    removeLastPop(compiler);
  }

  return 0;
}

int compileStatement(Compiler *compiler, Statement *statement) {
  if (!statement || !statement->type) {
    return -1;
  }

  if (strcmp(statement->type, NODE_EXPRESSION_STATEMENT) == 0) {
    ExpressionStatement *exprStmt = statement->expressionStatement;
    if (compileExpression(compiler, exprStmt->expression) != 0) {
      return -1;
    }
    emit(compiler, OpPop, NULL, 0);

  } else if (strcmp(statement->type, NODE_LET_STATEMENT) == 0) {
    LetStatement *letStmt = statement->letStatement;
    Symbol symbol = define(compiler->symbolTable, letStmt->name->value);

    if (compileExpression(compiler, letStmt->value) != 0) {
      return -1;
    }

    if (strcmp(symbol.scope, GlobalScope) == 0) {
      int operands[] = {symbol.index};
      emit(compiler, OpSetGlobal, operands, 1);
    } else {
      int operands[] = {symbol.index};
      emit(compiler, OpSetLocal, operands, 1);
    }

  } else if (strcmp(statement->type, NODE_RETURN_STATEMENT) == 0) {
    ReturnStatement *returnStmt = statement->returnStatement;
    if (compileExpression(compiler, returnStmt->return_value) != 0) {
      return -1;
    }
    emit(compiler, OpReturnValue, NULL, 0);

  } else if (strcmp(statement->type, NODE_BLOCK_STATEMENT) == 0) {
    BlockStatement *blockStmt = statement->blockStatement;
    return compileBlockStatement(compiler, blockStmt);

  } else {
    return -1; // Unknown statement type
  }

  return 0;
}

static int compileExpression(Compiler *compiler, Expression *expression) {
  if (!expression || !expression->type) {
    return -1;
  }

  if (strcmp(expression->type, NODE_INTEGER_LITERAL) == 0) {
    IntegerLiteral *intLit = expression->integerLiteral;
    Object *obj = malloc(sizeof(Object));
    obj->type = IntegerObj;
    obj->integer = malloc(sizeof(Integer));
    obj->integer->value = intLit->value;

    int constIndex = addConstant(compiler, obj);
    int operands[] = {constIndex};
    emit(compiler, OpConstant, operands, 1);

  } else if (strcmp(expression->type, NODE_BOOLEAN) == 0) {
    BooleanLiteral *boolLit = expression->booleanLiteral;
    if (boolLit->value) {
      emit(compiler, OpTrue, NULL, 0);
    } else {
      emit(compiler, OpFalse, NULL, 0);
    }

  } else if (strcmp(expression->type, NODE_STRING_LITERAL) == 0) {
    StringLiteral *strLit = expression->stringLiteral;
    Object *obj = malloc(sizeof(Object));
    obj->type = StringObj;
    obj->string = malloc(sizeof(String));
    obj->string->value = strdup(strLit->value);

    int constIndex = addConstant(compiler, obj);
    int operands[] = {constIndex};
    emit(compiler, OpConstant, operands, 1);

  } else if (strcmp(expression->type, NODE_IDENTIFIER) == 0) {
    Identifier *ident = expression->identifier;
    Symbol symbol;
    if (resolve(compiler->symbolTable, ident->value, &symbol) != 0) {
      return -1; // Undefined variable
    }
    loadSymbol(compiler, symbol);

  } else if (strcmp(expression->type, NODE_INFIX_EXPRESSION) == 0) {
    InfixExpression *infix = expression->infixExpression;

    // Special case for "<" operator - swap operands and use ">"
    if (strcmp(infix->op, "<") == 0) {
      if (compileExpression(compiler, infix->right) != 0) {
        return -1;
      }
      if (compileExpression(compiler, infix->left) != 0) {
        return -1;
      }
      emit(compiler, OpGreaterThan, NULL, 0);
      return 0;
    }

    // Compile left operand first
    if (compileExpression(compiler, infix->left) != 0) {
      return -1;
    }

    // Compile right operand
    if (compileExpression(compiler, infix->right) != 0) {
      return -1;
    }

    // Emit appropriate operator
    if (strcmp(infix->op, "+") == 0) {
      emit(compiler, OpAdd, NULL, 0);
    } else if (strcmp(infix->op, "-") == 0) {
      emit(compiler, OpSub, NULL, 0);
    } else if (strcmp(infix->op, "*") == 0) {
      emit(compiler, OpMul, NULL, 0);
    } else if (strcmp(infix->op, "/") == 0) {
      emit(compiler, OpDiv, NULL, 0);
    } else if (strcmp(infix->op, ">") == 0) {
      emit(compiler, OpGreaterThan, NULL, 0);
    } else if (strcmp(infix->op, "==") == 0) {
      emit(compiler, OpEqual, NULL, 0);
    } else if (strcmp(infix->op, "!=") == 0) {
      emit(compiler, OpNotEqual, NULL, 0);
    } else {
      return -1; // Unknown operator
    }

  } else if (strcmp(expression->type, NODE_PREFIX_EXPRESSION) == 0) {
    PrefixExpression *prefix = expression->prefixExpression;

    if (compileExpression(compiler, prefix->right) != 0) {
      return -1;
    }

    if (strcmp(prefix->op, "!") == 0) {
      emit(compiler, OpBang, NULL, 0);
    } else if (strcmp(prefix->op, "-") == 0) {
      emit(compiler, OpMinus, NULL, 0);
    } else {
      return -1; // Unknown operator
    }

  } else if (strcmp(expression->type, NODE_IF_EXPRESSION) == 0) {
    IfExpression *ifExpr = expression->ifExpression;

    // Compile condition
    if (compileExpression(compiler, ifExpr->condition) != 0) {
      return -1;
    }

    // Emit jump if not truthy with placeholder
    int operands[] = {9999};
    int jumpNotTruthyPos = emit(compiler, OpJumpNotTruthy, operands, 1);

    // Compile consequence
    if (compileBlockStatement(compiler, ifExpr->consequence) != 0) {
      return -1;
    }

    if (lastInstructionIs(compiler, OpPop)) {
      removeLastPop(compiler);
    }

    // Emit jump with placeholder
    int jumpOperands[] = {9999};
    int jumpPos = emit(compiler, OpJump, jumpOperands, 1);

    // Fix jump not truthy position
    CompilationScope *scope = &compiler->scopes[compiler->scopeIndex];
    int afterConsequencePos = scope->instructionsLength;
    changeOperand(compiler, jumpNotTruthyPos, afterConsequencePos);

    // Handle alternative
    if (ifExpr->alternative == NULL) {
      emit(compiler, OpNull, NULL, 0);
    } else {
      if (compileBlockStatement(compiler, ifExpr->alternative) != 0) {
        return -1;
      }
      if (lastInstructionIs(compiler, OpPop)) {
        removeLastPop(compiler);
      }
    }

    // Fix jump position
    int afterAlternativePos = scope->instructionsLength;
    changeOperand(compiler, jumpPos, afterAlternativePos);

  } else if (strcmp(expression->type, NODE_ARRAY_LITERAL) == 0) {
    ArrayLiteral *arrayLit = expression->arrayLiteral;

    for (int i = 0; i < arrayLit->count; i++) {
      if (compileExpression(compiler, arrayLit->elements[i]) != 0) {
        return -1;
      }
    }

    int operands[] = {arrayLit->count};
    emit(compiler, OpArray, operands, 1);

  } else if (strcmp(expression->type, NODE_HASH_LITERAL) == 0) {
    HashLiteral *hashLit = expression->hashLiteral;

    // Compile key-value pairs
    // Note: In the Go version, keys are sorted, but we'll compile them in order
    for (int i = 0; i < hashLit->count; i++) {
      if (compileExpression(compiler, hashLit->keys[i]) != 0) {
        return -1;
      }
      if (compileExpression(compiler, hashLit->values[i]) != 0) {
        return -1;
      }
    }

    int operands[] = {hashLit->count * 2};
    emit(compiler, OpHash, operands, 1);

  } else if (strcmp(expression->type, NODE_INDEX_EXPRESSION) == 0) {
    IndexExpression *indexExpr = expression->indexExpression;

    if (compileExpression(compiler, indexExpr->left) != 0) {
      return -1;
    }
    if (compileExpression(compiler, indexExpr->index) != 0) {
      return -1;
    }

    emit(compiler, OpIndex, NULL, 0);

  } else if (strcmp(expression->type, NODE_CALL_EXPRESSION) == 0) {
    CallExpression *callExpr = expression->callExpression;

    if (compileExpression(compiler, callExpr->function) != 0) {
      return -1;
    }

    for (int i = 0; i < callExpr->arg_count; i++) {
      if (compileExpression(compiler, callExpr->arguments[i]) != 0) {
        return -1;
      }
    }

    int operands[] = {callExpr->arg_count};
    emit(compiler, OpCall, operands, 1);

  } else if (strcmp(expression->type, NODE_FUNCTION_LITERAL) == 0) {
    FunctionLiteral *funcLit = expression->functionLiteral;

    enterScope(compiler);

    // Define parameters
    for (int i = 0; i < funcLit->param_count; i++) {
      define(compiler->symbolTable, funcLit->parameters[i]->value);
    }

    // Compile function body
    if (compileBlockStatement(compiler, funcLit->body) != 0) {
      return -1;
    }

    if (lastInstructionIs(compiler, OpPop)) {
      replaceLastPopWithReturn(compiler);
    }
    if (!lastInstructionIs(compiler, OpReturnValue)) {
      emit(compiler, OpReturn, NULL, 0);
    }

    int numLocals = compiler->symbolTable->numDefinitions;
    int instructionsLength;
    Instructions instructions = leaveScope(compiler, &instructionsLength);

    Object *compiledFn = malloc(sizeof(Object));
    compiledFn->type = CompiledFunctionObj;
    compiledFn->compiledFunction = malloc(sizeof(CompiledFunction));
    compiledFn->compiledFunction->instructions = instructions;
    compiledFn->compiledFunction->instructionCount = instructionsLength;
    compiledFn->compiledFunction->numLocals = numLocals;
    compiledFn->compiledFunction->numParameters = funcLit->param_count;

    int fnIndex = addConstant(compiler, compiledFn);
    int operands[] = {fnIndex};
    emit(compiler, OpConstant, operands, 1);

  } else {
    return -1; // Unknown expression type
  }

  return 0;
}

static int compileBlockStatement(Compiler *compiler, BlockStatement *block) {
  for (int i = 0; i < block->count; i++) {
    if (compileStatement(compiler, block->statements[i]) != 0) {
      return -1;
    }
  }
  return 0;
}

static int emit(Compiler *compiler, OpCode opCode, int *operands,
                int operandCount) {
  Instructions ins = makeInstruction(opCode, operands, operandCount);
  if (!ins) {
    return -1;
  }

  Definition def;
  if (lookupOpCode(opCode, &def) != 0) {
    free(ins);
    return -1;
  }

  int instructionLength = 1;
  for (int i = 0; i < def.operandCount; i++) {
    instructionLength += def.operandWidths[i];
  }

  int pos = addInstruction(compiler, ins, instructionLength);
  setLastInstruction(compiler, opCode, pos);

  free(ins);
  return pos;
}

static int addConstant(Compiler *compiler, Object *obj) {
  if (compiler->constantsCount >= compiler->constantsCapacity) {
    compiler->constantsCapacity *= 2;
    compiler->constants = realloc(compiler->constants,
                                  sizeof(Object) * compiler->constantsCapacity);
  }

  compiler->constants[compiler->constantsCount] = *obj;
  return compiler->constantsCount++;
}

static int addInstruction(Compiler *compiler, Instructions ins, int length) {
  CompilationScope *scope = &compiler->scopes[compiler->scopeIndex];

  int posNewInstruction = scope->instructionsLength;

  // Reallocate if needed
  if (scope->instructionsLength + length > scope->instructionsCapacity) {
    scope->instructionsCapacity *= 2;
    scope->instructions =
        realloc(scope->instructions, scope->instructionsCapacity);
  }

  // Copy instruction
  memcpy(scope->instructions + scope->instructionsLength, ins, length);
  scope->instructionsLength += length;

  return posNewInstruction;
}

static void setLastInstruction(Compiler *compiler, OpCode opCode,
                               int position) {
  CompilationScope *scope = &compiler->scopes[compiler->scopeIndex];
  EmittedInstruction previous = scope->lastInstruction;
  EmittedInstruction last = {opCode, position};

  scope->previousInstruction = previous;
  scope->lastInstruction = last;
}

static int lastInstructionIs(Compiler *compiler, OpCode opCode) {
  CompilationScope *scope = &compiler->scopes[compiler->scopeIndex];
  if (scope->instructionsLength == 0 || scope->lastInstruction.position == -1) {
    return 0;
  }
  return scope->lastInstruction.opCode == opCode;
}

static void removeLastPop(Compiler *compiler) {
  CompilationScope *scope = &compiler->scopes[compiler->scopeIndex];
  EmittedInstruction last = scope->lastInstruction;
  EmittedInstruction previous = scope->previousInstruction;

  scope->instructionsLength = last.position;
  scope->lastInstruction = previous;
}

static void replaceInstruction(Compiler *compiler, int pos,
                               Instructions newInstruction, int length) {
  CompilationScope *scope = &compiler->scopes[compiler->scopeIndex];

  for (int i = 0; i < length; i++) {
    scope->instructions[pos + i] = newInstruction[i];
  }
}

static void changeOperand(Compiler *compiler, int opPos, int operand) {
  CompilationScope *scope = &compiler->scopes[compiler->scopeIndex];
  OpCode op = scope->instructions[opPos];

  int operands[] = {operand};
  Instructions newInstruction = makeInstruction(op, operands, 1);

  Definition def;
  if (lookupOpCode(op, &def) == 0) {
    int instructionLength = 1;
    for (int i = 0; i < def.operandCount; i++) {
      instructionLength += def.operandWidths[i];
    }
    replaceInstruction(compiler, opPos, newInstruction, instructionLength);
  }

  free(newInstruction);
}

static Instructions getCurrentInstructions(Compiler *compiler) {
  return compiler->scopes[compiler->scopeIndex].instructions;
}

static int getCurrentInstructionsLength(Compiler *compiler) {
  return compiler->scopes[compiler->scopeIndex].instructionsLength;
}

static void enterScope(Compiler *compiler) {
  if (compiler->scopesLength >= compiler->scopesCapacity) {
    compiler->scopesCapacity *= 2;
    compiler->scopes = realloc(compiler->scopes, sizeof(CompilationScope) *
                                                     compiler->scopesCapacity);
  }

  CompilationScope scope = {0};
  scope.instructions = malloc(INITIAL_INSTRUCTIONS_CAPACITY);
  scope.instructionsLength = 0;
  scope.instructionsCapacity = INITIAL_INSTRUCTIONS_CAPACITY;
  scope.lastInstruction = (EmittedInstruction){0, -1};
  scope.previousInstruction = (EmittedInstruction){0, -1};

  compiler->scopes[compiler->scopesLength] = scope;
  compiler->scopesLength++;
  compiler->scopeIndex++;

  compiler->symbolTable = newEnclosedSymbolTable(compiler->symbolTable);
}

static Instructions leaveScope(Compiler *compiler, int *length) {
  Instructions instructions = getCurrentInstructions(compiler);
  *length = getCurrentInstructionsLength(compiler);

  compiler->scopesLength--;
  compiler->scopeIndex--;
  compiler->symbolTable = compiler->symbolTable->outer;

  return instructions;
}

static void replaceLastPopWithReturn(Compiler *compiler) {
  CompilationScope *scope = &compiler->scopes[compiler->scopeIndex];
  int lastPos = scope->lastInstruction.position;

  Instructions returnInst = makeInstruction(OpReturnValue, NULL, 0);
  replaceInstruction(compiler, lastPos, returnInst, 1);

  scope->lastInstruction.opCode = OpReturnValue;
  free(returnInst);
}

static void loadSymbol(Compiler *compiler, Symbol symbol) {
  int operands[] = {symbol.index};

  if (strcmp(symbol.scope, GlobalScope) == 0) {
    emit(compiler, OpGetGlobal, operands, 1);
  } else if (strcmp(symbol.scope, LocalScope) == 0) {
    emit(compiler, OpGetLocal, operands, 1);
  } else if (strcmp(symbol.scope, BuiltinScope) == 0) {
    emit(compiler, OpGetBuiltin, operands, 1);
  }
}

ByteCode *getByteCode(Compiler *compiler) {
  ByteCode *bytecode = malloc(sizeof(ByteCode));
  bytecode->instructions = getCurrentInstructions(compiler);
  bytecode->constants = compiler->constants;
  bytecode->constantsCount = compiler->constantsCount;
  bytecode->constantsCapacity = compiler->constantsCapacity;
  bytecode->instructionCount = getCurrentInstructionsLength(compiler);
  return bytecode;
}

int getByteCodeInstructionsLength(Compiler *compiler) {
  return getCurrentInstructionsLength(compiler);
}
