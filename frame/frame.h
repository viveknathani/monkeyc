#ifndef FRAME_H
#define FRAME_H

#include "../object/object.h"

typedef struct {
  CompiledFunction *compiledFunction;
  int ip;
  int basePointer;
} Frame;

Instructions getFrameInstructions(Frame *frame);

#endif
