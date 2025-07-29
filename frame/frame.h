#ifndef FRAME_H
#define FRAME_H

#include "../object/object.h"

typedef struct {
  CompiledFunction *compiledFunction;
  int ip;
  int basePointer;
} Frame;

Frame* newFrame(Instructions instructions, int basePointer);
Instructions getFrameInstructions(Frame *frame);

#endif
