#ifndef VM_H
#define VM_H

#include "../object/object.h"
#include "../frame/frame.h"
#include "../compiler/compiler.h"

// vm limits - these are reasonable defaults for most programs
// stack size: max depth of expression evaluation and function calls
#define STACK_SIZE 2048
// global variables: max number of global bindings (let statements at top level)
#define GLOBAL_SIZE 65536
// call frames: max depth of function call nesting
#define MAX_FRAMES 1024

typedef struct VM VM;

static Boolean TRUE = {
  .value = 1
};

static Boolean FALSE = {
  .value = 0
};

struct VM {
  Object* constants;
  int constantsCount;
  Object* stack;
  int stackCount;
  int sp;
  Object* globals;
  int globalCount;
  Frame* frames;
  int frameCount;
  int framesIndex;
};

VM* newVM(ByteCode *bytecode);
VM* newVMWithGlobalStore(ByteCode *bytecode, Object* globals, int globalCount);
void freeVM(VM *vm);
Frame* currentFrame(VM *vm);
void pushFrame(VM *vm, Frame *frame);
Frame* popFrame(VM *vm);
int run(VM* vm);
int push(VM *vm, Object *object);
Object* pop(VM *vm);
Object* stackTop(VM *vm);
Object* lastPoppedStackElem(VM *vm);
Object* nativeBoolToBooleanObject(bool input);
bool isTruthy(Object *obj);
int executeBinaryOperation(VM *vm, OpCode opCode);
int executeBinaryIntegerOperation(VM *vm, OpCode opCode, Object *left, Object *right);
int executeBinaryStringOperation(VM *vm, OpCode opCode, Object *left, Object *right);
int executeComparison(VM *vm, OpCode opCode);
int executeIntegerComparison(VM *vm, OpCode opCode, Object *left, Object *right);
int executeBangOperator(VM *vm);
int executeMinusOperator(VM *vm);
int executeIndexExpression(VM *vm, Object *left, Object *index);
int executeArrayIndex(VM *vm, Object *array, Object *index);
int executeHashIndex(VM *vm, Object *hash, Object *index);
Object* buildArray(VM *vm, int startIndex, int endIndex);
Object* buildHash(VM *vm, int startIndex, int endIndex);
int executeCall(VM *vm, int numArgs);
int callCompiledFunction(VM *vm, CompiledFunction *fn, int numArgs);
int callBuiltin(VM *vm, Builtin *builtin, int numArgs);

#endif
