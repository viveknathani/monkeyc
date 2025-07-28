#ifndef OPCODE_H
#define OPCODE_H

typedef char OpCode;
typedef char *Instructions;

#define OpConstant 0
#define OpPop 1
#define OpAdd 2
#define OpSub 3
#define OpMul 4
#define OpDiv 5
#define OpTrue 6
#define OpFalse 7
#define OpEqual 8
#define OpNotEqual 9
#define OpGreaterThan 10
#define OpMinus 11
#define OpBang 12
#define OpJumpNotTruthy 13
#define OpJump 14
#define OpNull 15
#define OpGetGlobal 16
#define OpSetGlobal 17
#define OpArray 18
#define OpHash 19
#define OpIndex 20
#define OpCall 21
#define OpReturnValue 22
#define OpReturn 23
#define OpGetLocal 24
#define OpSetLocal 25
#define OpGetBuiltin 26
#define OpGetFree 27

typedef struct {
  const char *name;
  int operandWidths[2];
  int operandCount;
} Definition;

#define MAX_OPCODE 27
extern Definition definitions[MAX_OPCODE + 1];

int lookupOpCode(char opCode, Definition *out);
Instructions makeInstruction(char opCode, int *operands, int operandCount);
int readOperands(Definition *definition, Instructions instructions,
                 int instructionLength, int *operands, int *offset);
char *instructionsToString(Instructions instructions, int length);

#endif
