#ifndef VM_H
#define VM_H

#include "../object/object.h"
#include "../frame/frame.h"
#include "../compiler/compiler.h"

#define STACK_SIZE 2048
#define GLOBAL_SIZE 65536
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

#endif
