#include "vm.h"
#include <stdlib.h>
#include <string.h>

static Null null_obj = {};

VM* newVM(ByteCode *bytecode) {
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
    Object nullObj = {
        .type = NullObj,
        .null = &null_obj
    };
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

Frame* currentFrame(VM *vm) {
    return &vm->frames[vm->framesIndex - 1];
}

void pushFrame(VM *vm, Frame *frame) {
    vm->frames[vm->framesIndex] = *frame;
    vm->framesIndex++;
}

Frame* popFrame(VM *vm) {
    vm->framesIndex--;
    return &vm->frames[vm->framesIndex];
}

VM* newVMWithGlobalStore(ByteCode *bytecode, Object* globals, int globalCount) {
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
