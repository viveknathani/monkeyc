#include "opcode.h"
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Definition definitions[MAX_OPCODE + 1] = {
    [OpConstant] = {"OpConstant", {2, 0}, 1},
    [OpPop] = {"OpPop", {0, 0}, 0},
    [OpAdd] = {"OpAdd", {0, 0}, 0},
    [OpSub] = {"OpSub", {0, 0}, 0},
    [OpMul] = {"OpMul", {0, 0}, 0},
    [OpDiv] = {"OpDiv", {0, 0}, 0},
    [OpTrue] = {"OpTrue", {0, 0}, 0},
    [OpFalse] = {"OpFalse", {0, 0}, 0},
    [OpEqual] = {"OpEqual", {0, 0}, 0},
    [OpNotEqual] = {"OpNotEqual", {0, 0}, 0},
    [OpGreaterThan] = {"OpGreaterThan", {0, 0}, 0},
    [OpMinus] = {"OpMinus", {0, 0}, 0},
    [OpBang] = {"OpBang", {0, 0}, 0},
    [OpJumpNotTruthy] = {"OpJumpNotTruthy", {2, 0}, 1},
    [OpJump] = {"OpJump", {2, 0}, 1},
    [OpNull] = {"OpNull", {0, 0}, 0},
    [OpGetGlobal] = {"OpGetGlobal", {2, 0}, 1},
    [OpSetGlobal] = {"OpSetGlobal", {2, 0}, 1},
    [OpArray] = {"OpArray", {2, 0}, 1},
    [OpHash] = {"OpHash", {2, 0}, 1},
    [OpIndex] = {"OpIndex", {0, 0}, 0},
    [OpCall] = {"OpCall", {1, 0}, 1},
    [OpReturnValue] = {"OpReturnValue", {0, 0}, 0},
    [OpReturn] = {"OpReturn", {0, 0}, 0},
    [OpGetLocal] = {"OpGetLocal", {1, 0}, 1},
    [OpSetLocal] = {"OpSetLocal", {1, 0}, 1},
    [OpGetBuiltin] = {"OpGetBuiltin", {1, 0}, 1},
    [OpGetFree] = {"OpGetFree", {1, 0}, 1},
};

int lookupOpCode(char opCode, Definition *out) {
  if (opCode < 0 || opCode > MAX_OPCODE) {
    return -1;
  }

  *out = definitions[(int)opCode];
  return 0;
}

Instructions makeInstruction(char opCode, int *operands, int operandCount) {
  Definition definition;
  if (lookupOpCode(opCode, &definition) != 0) {
    return NULL;
  }

  // account for the opCode's length
  int instructionLength = 1;

  // next: account for the operands' length
  for (int i = 0; i < definition.operandCount; i++) {
    instructionLength += definition.operandWidths[i];
  }

  Instructions instructions =
      (Instructions)malloc(sizeof(char) * instructionLength);

  instructions[0] = opCode;

  int offset = 1;
  for (int i = 0; i < operandCount; i++) {
    int width = definition.operandWidths[i];
    int operand = operands[i];

    switch (width) {
    case 1: {
      instructions[offset] = operand & 0xff;
      break;
    }

    case 2: {
      instructions[offset] = (operand >> 8) & 0xff;
      instructions[offset + 1] = operand & 0xff;
      break;
    }

    default:
      break;
    }

    offset += width;
  }

  return instructions;
}

int readOperands(Definition *definition, Instructions instructions,
                 int instructionLength __attribute__((unused)), int *operands,
                 int *offset) {
  if (definition == NULL || instructions == NULL || operands == NULL ||
      offset == NULL) {
    return -1;
  }

  int localOffset = 0;

  for (int i = 0; i < definition->operandCount; i++) {
    int width = definition->operandWidths[i];
    switch (width) {
    case 1:
      operands[i] = (unsigned char)instructions[localOffset];
      break;

    case 2:
      operands[i] = ((unsigned char)instructions[localOffset] << 8) |
                    (unsigned char)instructions[localOffset + 1];
      break;

    default:
      return -1;
    }

    localOffset += width;
  }

  *offset = localOffset;
  return 0;
}

char *instructionsToString(Instructions instructions, int length) {
  int pos = 0;
  int capacity = 256;
  char *output = malloc(capacity);
  output[0] = '\0';
  int outputLen = 0;

  while (pos < length) {
    unsigned char op = instructions[pos];

    Definition def;
    if (lookupOpCode(op, &def) != 0) {
      const char *unknown = "UNKNOWN_OPCODE";
      outputLen += snprintf(output + outputLen, capacity - outputLen,
                            "%04d %s\n", pos, unknown);
      pos += 1;
      continue;
    }

    int operands[2] = {0};
    int read = 0;
    readOperands(&def, &instructions[pos + 1], 0, operands, &read);

    char line[128];
    int written;
    if (def.operandCount > 0) {
      written = snprintf(line, sizeof(line), "%04d %-16s", pos, def.name);
      for (int i = 0; i < def.operandCount; i++) {
        written +=
            snprintf(line + written, sizeof(line) - written, "%d", operands[i]);
      }
    } else {
      written = snprintf(line, sizeof(line), "%04d %s", pos, def.name);
    }
    written += snprintf(line + written, sizeof(line) - written, "\n");

    if (outputLen + written + 1 > capacity) {
      capacity *= 2;
      output = realloc(output, capacity);
    }

    strcpy(output + outputLen, line);
    outputLen += written;
    pos += 1 + read;
  }

  return output;
}
