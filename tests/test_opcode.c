#include "../opcode/opcode.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void testInstructionsToString() {
  Instructions codes[4];
  int lens[4];

  // OpAdd
  codes[0] = makeInstruction(OpAdd, NULL, 0);
  lens[0] = 1;

  // OpGetLocal 1
  int ops1[] = {1};
  codes[1] = makeInstruction(OpGetLocal, ops1, 1);
  lens[1] = 1 + definitions[OpGetLocal].operandWidths[0];

  // OpConstant 2
  int ops2[] = {2};
  codes[2] = makeInstruction(OpConstant, ops2, 1);
  lens[2] = 1 + definitions[OpConstant].operandWidths[0];

  // OpConstant 65535
  int ops3[] = {65535};
  codes[3] = makeInstruction(OpConstant, ops3, 1);
  lens[3] = 1 + definitions[OpConstant].operandWidths[0];

  // concatenate all instructions
  int totalLen = 0;
  for (int i = 0; i < 4; i++)
    totalLen += lens[i];
  Instructions all = malloc(totalLen);
  int pos = 0;
  for (int i = 0; i < 4; i++) {
    memcpy(all + pos, codes[i], lens[i]);
    pos += lens[i];
  }

  char *result = instructionsToString(all, totalLen);

  const char *expected = "0000 OpAdd\n"
                         "0001 OpGetLocal      1\n"
                         "0003 OpConstant      2\n"
                         "0006 OpConstant      65535\n";

  if (strcmp(result, expected) == 0) {
    printf("✅ instructionsToString test passed\n");
  } else {
    printf("❌ instructionsToString test failed\n");
    printf("Expected:\n%s\nGot:\n%s\n", expected, result);
  }

  // cleanup
  for (int i = 0; i < 4; i++)
    free(codes[i]);
  free(all);
  free(result);
}

int main() {
  testInstructionsToString();
  return 0;
}
