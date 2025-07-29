#include "frame.h"
#include <stdlib.h>

Instructions getFrameInstructions(Frame *frame) {
  if (frame == NULL || frame->compiledFunction == NULL) {
    return NULL;
  }
  return frame->compiledFunction->instructions;
}
