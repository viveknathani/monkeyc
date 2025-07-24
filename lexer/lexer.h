#ifndef LEXER_H
#define LEXER_H

#include "../token/token.h"

typedef struct {
  char *input;
  int position;
  int nextPosition;
  int inputLength;
  unsigned char currentChar;
} Lexer;

Lexer *newLexer(char *input);

Token nextToken(Lexer *lexer);

#endif
