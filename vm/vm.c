#include "vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Null null_obj = {};

VM *newVM(ByteCode *bytecode) {
  VM *vm = malloc(sizeof(VM));
  if (!vm) {
    return NULL;
  }

  // Initialize constants
  vm->constants = malloc(sizeof(Object) * bytecode->constantsCount);
  if (!vm->constants && bytecode->constantsCount > 0) {
    free(vm);
    return NULL;
  }
  vm->constantsCount = bytecode->constantsCount;

  // Copy constants from bytecode
  for (int i = 0; i < bytecode->constantsCount; i++) {
    vm->constants[i] = bytecode->constants[i];
  }

  // Initialize stack
  vm->stack = malloc(sizeof(Object) * STACK_SIZE);
  if (!vm->stack) {
    free(vm->constants);
    free(vm);
    return NULL;
  }
  vm->stackCount = STACK_SIZE;
  vm->sp = 0;

  // Initialize globals
  vm->globals = malloc(sizeof(Object) * GLOBAL_SIZE);
  if (!vm->globals) {
    free(vm->stack);
    free(vm->constants);
    free(vm);
    return NULL;
  }
  vm->globalCount = GLOBAL_SIZE;

  // Initialize all globals to null
  Object nullObj = {.type = NullObj, .null = &null_obj};
  for (int i = 0; i < GLOBAL_SIZE; i++) {
    vm->globals[i] = nullObj;
  }

  // Initialize frames
  vm->frames = malloc(sizeof(Frame) * MAX_FRAMES);
  if (!vm->frames) {
    free(vm->globals);
    free(vm->stack);
    free(vm->constants);
    free(vm);
    return NULL;
  }
  vm->frameCount = MAX_FRAMES;
  vm->framesIndex = 1;

  // Create main compiled function from bytecode
  CompiledFunction *mainFn = malloc(sizeof(CompiledFunction));
  if (!mainFn) {
    free(vm->frames);
    free(vm->globals);
    free(vm->stack);
    free(vm->constants);
    free(vm);
    return NULL;
  }
  mainFn->instructions = bytecode->instructions;
  mainFn->numLocals = 0;
  mainFn->numParameters = 0;
  mainFn->instructionCount = bytecode->instructionCount;

  // Create the main frame (equivalent to mainFrame in Go)
  vm->frames[0].compiledFunction = mainFn;
  vm->frames[0].ip = -1;
  vm->frames[0].basePointer = 0;

  return vm;
}

void freeVM(VM *vm) {
  if (!vm) {
    return;
  }

  // Free the main compiled function from frame 0
  if (vm->frames && vm->frames[0].compiledFunction) {
    free(vm->frames[0].compiledFunction);
  }

  free(vm->constants);
  free(vm->stack);
  free(vm->globals);
  free(vm->frames);
  free(vm);
}

Frame *currentFrame(VM *vm) { return &vm->frames[vm->framesIndex - 1]; }

void pushFrame(VM *vm, Frame *frame) {
  vm->frames[vm->framesIndex] = *frame;
  vm->framesIndex++;
}

Frame *popFrame(VM *vm) {
  vm->framesIndex--;
  return &vm->frames[vm->framesIndex];
}

VM *newVMWithGlobalStore(ByteCode *bytecode, Object *globals, int globalCount) {
  VM *vm = newVM(bytecode);
  if (!vm) {
    return NULL;
  }

  // Replace the globals with the provided ones
  free(vm->globals);
  vm->globals = globals;
  vm->globalCount = globalCount;

  return vm;
}

int push(VM *vm, Object *object) {
  if (vm->sp >= STACK_SIZE) {
    return -1; // Stack overflow
  }
  vm->stack[vm->sp] = *object;
  vm->sp++;
  return 0;
}

Object *pop(VM *vm) {
  if (vm->sp <= 0) {
    return NULL;
  }
  vm->sp--;
  return &vm->stack[vm->sp];
}

Object *stackTop(VM *vm) {
  if (vm->sp == 0) {
    return NULL;
  }
  return &vm->stack[vm->sp - 1];
}

Object *lastPoppedStackElem(VM *vm) { return &vm->stack[vm->sp]; }

Object *nativeBoolToBooleanObject(bool input) {
  Object *obj = malloc(sizeof(Object));
  obj->type = BooleanObj;
  obj->boolean = malloc(sizeof(Boolean));
  obj->boolean->value = input;
  return obj;
}

bool isTruthy(Object *obj) {
  if (strcmp(obj->type, BooleanObj) == 0) {
    return obj->boolean->value;
  } else if (strcmp(obj->type, NullObj) == 0) {
    return false;
  }
  return true;
}

int executeBinaryOperation(VM *vm, OpCode opCode) {
  Object *right = pop(vm);
  Object *left = pop(vm);

  if (!right || !left) {
    return -1;
  }

  ObjectType rightType = right->type;
  ObjectType leftType = left->type;

  if (strcmp(leftType, IntegerObj) == 0 && strcmp(rightType, IntegerObj) == 0) {
    return executeBinaryIntegerOperation(vm, opCode, left, right);
  } else if (strcmp(leftType, StringObj) == 0 &&
             strcmp(rightType, StringObj) == 0) {
    return executeBinaryStringOperation(vm, opCode, left, right);
  }

  return -1;
}

int executeBinaryIntegerOperation(VM *vm, OpCode opCode, Object *left,
                                  Object *right) {
  int64_t leftValue = left->integer->value;
  int64_t rightValue = right->integer->value;
  int64_t result;

  switch (opCode) {
  case OpAdd:
    result = leftValue + rightValue;
    break;
  case OpSub:
    result = leftValue - rightValue;
    break;
  case OpMul:
    result = leftValue * rightValue;
    break;
  case OpDiv:
    result = leftValue / rightValue;
    break;
  default:
    return -1;
  }

  Object *resultObj = malloc(sizeof(Object));
  resultObj->type = IntegerObj;
  resultObj->integer = malloc(sizeof(Integer));
  resultObj->integer->value = result;

  return push(vm, resultObj);
}

int executeBinaryStringOperation(VM *vm, OpCode opCode, Object *left,
                                 Object *right) {
  if (opCode != OpAdd) {
    return -1; // Only concatenation supported for strings
  }

  char *leftValue = left->string->value;
  char *rightValue = right->string->value;

  int leftLen = strlen(leftValue);
  int rightLen = strlen(rightValue);
  char *result = malloc(leftLen + rightLen + 1);
  strcpy(result, leftValue);
  strcat(result, rightValue);

  Object *resultObj = malloc(sizeof(Object));
  resultObj->type = StringObj;
  resultObj->string = malloc(sizeof(String));
  resultObj->string->value = result;

  return push(vm, resultObj);
}

int executeComparison(VM *vm, OpCode opCode) {
  Object *right = pop(vm);
  Object *left = pop(vm);

  if (!right || !left) {
    return -1;
  }

  if (strcmp(left->type, IntegerObj) == 0 &&
      strcmp(right->type, IntegerObj) == 0) {
    return executeIntegerComparison(vm, opCode, left, right);
  }

  Object *result;
  switch (opCode) {
  case OpEqual:
    // Simple pointer comparison for now
    result = nativeBoolToBooleanObject(left == right);
    break;
  case OpNotEqual:
    result = nativeBoolToBooleanObject(left != right);
    break;
  default:
    return -1;
  }

  return push(vm, result);
}

int executeIntegerComparison(VM *vm, OpCode opCode, Object *left,
                             Object *right) {
  int64_t leftValue = left->integer->value;
  int64_t rightValue = right->integer->value;
  Object *result;

  switch (opCode) {
  case OpEqual:
    result = nativeBoolToBooleanObject(leftValue == rightValue);
    break;
  case OpNotEqual:
    result = nativeBoolToBooleanObject(leftValue != rightValue);
    break;
  case OpGreaterThan:
    result = nativeBoolToBooleanObject(leftValue > rightValue);
    break;
  default:
    return -1;
  }

  return push(vm, result);
}

int executeBangOperator(VM *vm) {
  Object *operand = pop(vm);
  if (!operand) {
    return -1;
  }

  Object *result;
  if (strcmp(operand->type, BooleanObj) == 0) {
    result = nativeBoolToBooleanObject(!operand->boolean->value);
  } else if (strcmp(operand->type, NullObj) == 0) {
    result = nativeBoolToBooleanObject(true);
  } else {
    result = nativeBoolToBooleanObject(false);
  }

  return push(vm, result);
}

int executeMinusOperator(VM *vm) {
  Object *operand = pop(vm);
  if (!operand) {
    return -1;
  }

  if (strcmp(operand->type, IntegerObj) != 0) {
    return -1; // Unsupported type for negation
  }

  Object *result = malloc(sizeof(Object));
  result->type = IntegerObj;
  result->integer = malloc(sizeof(Integer));
  result->integer->value = -operand->integer->value;

  return push(vm, result);
}

int executeIndexExpression(VM *vm, Object *left, Object *index) {
  if (strcmp(left->type, ArrayObj) == 0 &&
      strcmp(index->type, IntegerObj) == 0) {
    return executeArrayIndex(vm, left, index);
  } else if (strcmp(left->type, HashObj) == 0) {
    return executeHashIndex(vm, left, index);
  }
  return -1;
}

int executeArrayIndex(VM *vm, Object *array, Object *index) {
  Array *arrayObj = array->array;
  int64_t i = index->integer->value;
  int max = arrayObj->count - 1;

  if (i < 0 || i > max) {
    Object nullObj = {.type = NullObj, .null = &null_obj};
    return push(vm, &nullObj);
  }

  return push(vm, &arrayObj->elements[i]);
}

int executeHashIndex(VM *vm, Object *hash, Object *index) {
  Hash *hashObj = hash->hash;
  for (int i = 0; i < hashObj->count; i++) {
    Object *key = &hashObj->pairs[i].key;
    bool match = false;
    if (strcmp(key->type, StringObj) == 0 &&
        strcmp(index->type, StringObj) == 0) {
      match = strcmp(key->string->value, index->string->value) == 0;
    } else if (strcmp(key->type, IntegerObj) == 0 &&
               strcmp(index->type, IntegerObj) == 0) {
      match = key->integer->value == index->integer->value;
    }

    if (match) {
      return push(vm, &hashObj->pairs[i].value);
    }
  }

  // key not found – push null
  Object nullObj = {.type = NullObj, .null = &null_obj};
  return push(vm, &nullObj);
}

Object *buildArray(VM *vm, int startIndex, int endIndex) {
  Object *arrayObj = malloc(sizeof(Object));
  arrayObj->type = ArrayObj;
  arrayObj->array = malloc(sizeof(Array));

  int numElements = endIndex - startIndex;
  arrayObj->array->elements = malloc(sizeof(Object) * numElements);
  arrayObj->array->count = numElements;

  for (int i = startIndex; i < endIndex; i++) {
    arrayObj->array->elements[i - startIndex] = vm->stack[i];
  }

  return arrayObj;
}

Object *buildHash(VM *vm, int startIndex, int endIndex) {
  /*
   * The stack currently contains key / value objects in sequence:
   *   ... key1, value1, key2, value2, ...
   * startIndex points to key1 and endIndex is the original vm->sp value
   * (one past the last pushed element).
   */
  int numElements = endIndex - startIndex;
  int numPairs = numElements / 2;

  Object *hashObj = malloc(sizeof(Object));
  hashObj->type = HashObj;
  hashObj->hash = malloc(sizeof(Hash));
  hashObj->hash->count = numPairs;
  hashObj->hash->pairs = malloc(sizeof(HashPair) * numPairs);

  for (int i = 0; i < numPairs; i++) {
    Object keyObj = vm->stack[startIndex + (i * 2)];
    Object valueObj = vm->stack[startIndex + (i * 2) + 1];
    hashObj->hash->pairs[i].key = keyObj;
    hashObj->hash->pairs[i].value = valueObj;
  }

  return hashObj;
}

int executeCall(VM *vm, int numArgs) {
  Object *callee = &vm->stack[vm->sp - 1 - numArgs];

  if (strcmp(callee->type, CompiledFunctionObj) == 0) {
    return callCompiledFunction(vm, callee->compiledFunction, numArgs);
  } else if (strcmp(callee->type, BuiltinObj) == 0) {
    return callBuiltin(vm, callee->builtin, numArgs);
  }

  return -1; // Calling non-function
}

int callCompiledFunction(VM *vm, CompiledFunction *fn, int numArgs) {
  if (numArgs != fn->numParameters) {
    return -1; // Wrong number of arguments
  }

  Frame *frame = &vm->frames[vm->framesIndex++];
  frame->compiledFunction = fn;
  frame->ip = -1;
  frame->basePointer = vm->sp - numArgs;
  // vm->sp = frame.basePointer + fn->numLocals;

  return 0;
}

int callBuiltin(VM *vm, Builtin *builtin, int numArgs) {
  Object *args = &vm->stack[vm->sp - numArgs];
  Object *result = builtin->function(&args, numArgs);

  vm->sp = vm->sp - numArgs - 1;

  if (result != NULL) {
    push(vm, result);
  } else {
    Object nullObj = {.type = NullObj, .null = &null_obj};
    push(vm, &nullObj);
  }

  return 0;
}

int run(VM *vm) {
  int ip;
  Instructions instructions;
  OpCode opCode;

  while (currentFrame(vm)->ip <
         currentFrame(vm)->compiledFunction->instructionCount - 1) {
    currentFrame(vm)->ip++;

    ip = currentFrame(vm)->ip;
    instructions = currentFrame(vm)->compiledFunction->instructions;
    opCode = (OpCode)instructions[ip];

    switch (opCode) {
    case OpConstant: {
      int constIndex = ((unsigned char)instructions[ip + 1] << 8) |
                       (unsigned char)instructions[ip + 2];
      currentFrame(vm)->ip += 2;

      if (push(vm, &vm->constants[constIndex]) != 0) {
        return -1;
      }
      break;
    }

    case OpPop:
      pop(vm);
      break;

    case OpAdd:
    case OpSub:
    case OpMul:
    case OpDiv:
      if (executeBinaryOperation(vm, opCode) != 0) {
        return -1;
      }
      break;

    case OpTrue: {
      Object trueObj = {.type = BooleanObj, .boolean = &TRUE};
      if (push(vm, &trueObj) != 0) {
        return -1;
      }
      break;
    }

    case OpFalse: {
      Object falseObj = {.type = BooleanObj, .boolean = &FALSE};
      if (push(vm, &falseObj) != 0) {
        return -1;
      }
      break;
    }

    case OpEqual:
    case OpNotEqual:
    case OpGreaterThan:
      if (executeComparison(vm, opCode) != 0) {
        return -1;
      }
      break;

    case OpBang:
      if (executeBangOperator(vm) != 0) {
        return -1;
      }
      break;

    case OpMinus:
      if (executeMinusOperator(vm) != 0) {
        return -1;
      }
      break;

    case OpJumpNotTruthy: {
      int pos = ((unsigned char)instructions[ip + 1] << 8) |
                (unsigned char)instructions[ip + 2];
      currentFrame(vm)->ip += 2;

      Object *condition = pop(vm);
      if (!isTruthy(condition)) {
        currentFrame(vm)->ip = pos - 1;
      }
      break;
    }

    case OpJump: {
      int pos = ((unsigned char)instructions[ip + 1] << 8) |
                (unsigned char)instructions[ip + 2];
      currentFrame(vm)->ip = pos - 1;
      break;
    }

    case OpNull: {
      Object nullObj = {.type = NullObj, .null = &null_obj};
      if (push(vm, &nullObj) != 0) {
        return -1;
      }
      break;
    }

    case OpSetGlobal: {
      int globalIndex = ((unsigned char)instructions[ip + 1] << 8) |
                        (unsigned char)instructions[ip + 2];
      currentFrame(vm)->ip += 2;

      vm->globals[globalIndex] = *pop(vm);
      break;
    }

    case OpGetGlobal: {
      int globalIndex = ((unsigned char)instructions[ip + 1] << 8) |
                        (unsigned char)instructions[ip + 2];
      currentFrame(vm)->ip += 2;

      if (push(vm, &vm->globals[globalIndex]) != 0) {
        return -1;
      }
      break;
    }

    case OpArray: {
      int numElements = ((unsigned char)instructions[ip + 1] << 8) |
                        (unsigned char)instructions[ip + 2];
      currentFrame(vm)->ip += 2;

      Object *array = buildArray(vm, vm->sp - numElements, vm->sp);
      vm->sp = vm->sp - numElements;

      if (push(vm, array) != 0) {
        return -1;
      }
      break;
    }

    case OpHash: {
      int numElements = ((unsigned char)instructions[ip + 1] << 8) |
                        (unsigned char)instructions[ip + 2];
      currentFrame(vm)->ip += 2;

      Object *hash = buildHash(vm, vm->sp - numElements, vm->sp);
      vm->sp = vm->sp - numElements;

      if (push(vm, hash) != 0) {
        return -1;
      }
      break;
    }

    case OpIndex: {
      Object *index = pop(vm);
      Object *left = pop(vm);

      if (executeIndexExpression(vm, left, index) != 0) {
        return -1;
      }
      break;
    }

    case OpCall: {
      int numArgs = (unsigned char)instructions[ip + 1];
      currentFrame(vm)->ip += 1;

      if (executeCall(vm, numArgs) != 0) {
        return -1;
      }
      break;
    }

    case OpReturnValue: {
      Object *returnValue = pop(vm);
      Frame *frame = popFrame(vm);
      vm->sp = frame->basePointer - 1;

      if (push(vm, returnValue) != 0) {
        return -1;
      }
      break;
    }

    case OpReturn: {
      Frame *frame = popFrame(vm);
      vm->sp = frame->basePointer - 1;

      Object nullObj = {.type = NullObj, .null = &null_obj};
      if (push(vm, &nullObj) != 0) {
        return -1;
      }
      break;
    }

    case OpSetLocal: {
      int localIndex = (unsigned char)instructions[ip + 1];
      currentFrame(vm)->ip += 1;

      Frame *frame = currentFrame(vm);
      vm->stack[frame->basePointer + localIndex] = *pop(vm);
      break;
    }

    case OpGetLocal: {
      int localIndex = (unsigned char)instructions[ip + 1];
      currentFrame(vm)->ip += 1;

      Frame *frame = currentFrame(vm);
      if (push(vm, &vm->stack[frame->basePointer + localIndex]) != 0) {
        return -1;
      }
      break;
    }

    case OpGetBuiltin: {
      int builtinIndex = (unsigned char)instructions[ip + 1];
      currentFrame(vm)->ip += 1;

      // Get builtin from builtins array
      if (push(vm, (Object *)&builtins[builtinIndex].function) != 0) {
        return -1;
      }
      break;
    }

    default:
      return -1; // Unknown opcode
    }
  }

  return 0;
}
