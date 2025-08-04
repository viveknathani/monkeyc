#include "frame.h"
#include <stdlib.h>

// create a new call frame for function execution
// basePointer: stack position where this frame's locals start
Frame* newFrame(Instructions instructions, int basePointer) {
  Frame *frame = malloc(sizeof(Frame));
  frame->ip = -1;  // instruction pointer starts at -1, gets incremented before first use
  frame->basePointer = basePointer;

  return frame;
}

// get the bytecode instructions for this frame's function
Instructions getFrameInstructions(Frame *frame) {
  if (frame == NULL || frame->compiledFunction == NULL) {
    return NULL;
  }
  return frame->compiledFunction->instructions;
}
