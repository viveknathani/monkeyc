#include "frame.h"
#include <stdlib.h>

Frame* newFrame(Instructions instructions, int basePointer) {
  Frame *frame = malloc(sizeof(Frame));
  frame->ip = -1;
  frame->basePointer = basePointer;

  return frame;
}

Instructions getFrameInstructions(Frame *frame) {
  if (frame == NULL || frame->compiledFunction == NULL) {
    return NULL;
  }
  return frame->compiledFunction->instructions;
}
