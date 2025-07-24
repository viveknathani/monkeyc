#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned char peekChar(Lexer *lexer) {
  if (lexer->nextPosition >= lexer->inputLength) {
    return 0;
  }
  return lexer->input[lexer->nextPosition];
}

void readChar(Lexer *lexer) {
  if (lexer->nextPosition >= lexer->inputLength) {
    lexer->currentChar = 0;
  } else {
    lexer->currentChar = lexer->input[lexer->nextPosition];
  }
  lexer->position = lexer->nextPosition;
  lexer->nextPosition += 1;
}

Lexer *newLexer(char *input) {
  Lexer *lexer = malloc(sizeof(Lexer));
  lexer->input = input;
  lexer->position = 0;
  lexer->nextPosition = 0;
  lexer->currentChar = 0;
  lexer->inputLength = strlen(input);

  readChar(lexer);
  return lexer;
}

char *createSingleCharLiteral(unsigned char ch) {
  char *literal = malloc(2);
  if (!literal)
    return NULL;

  literal[0] = ch;
  literal[1] = '\0';

  return literal;
}

char *readString(Lexer *lexer) {
  int start = lexer->position + 1;

  for (;;) {
    readChar(lexer);
    if (lexer->currentChar == '"' || lexer->currentChar == 0) {
      break;
    }
  }

  int length = lexer->position - start;
  char *out = malloc(length + 1);
  if (!out)
    return NULL;

  memcpy(out, &lexer->input[start], length);
  out[length] = '\0';

  return out;
}

int isLetter(unsigned char ch) {
  return (('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z') || (ch == '_'));
}

int isDigit(unsigned char ch) { return '0' <= ch && ch <= '9'; }

void skipWhitespace(Lexer *lexer) {
  for (;;) {
    if ((lexer->currentChar == ' ') || (lexer->currentChar == '\t') ||
        (lexer->currentChar == '\n') || (lexer->currentChar == '\r')) {
      readChar(lexer);
    } else {
      break;
    }
  }
}

char *readNumber(Lexer *lexer) {
  int start = lexer->position;
  while (isDigit(lexer->currentChar)) {
    readChar(lexer);
  }

  int len = lexer->position - start;
  char *num = malloc(len + 1);
  if (!num)
    return NULL;

  memcpy(num, &lexer->input[start], len);
  num[len] = '\0';
  return num;
}

char *readIdentifier(Lexer *lexer) {
  int start = lexer->position;
  while (isLetter(lexer->currentChar)) {
    readChar(lexer);
  }

  int len = lexer->position - start;
  char *id = malloc(len + 1);
  if (!id)
    return NULL;

  memcpy(id, &lexer->input[start], len);
  id[len] = '\0';
  return id;
}

Token nextToken(Lexer *lexer) {
  Token token;

  skipWhitespace(lexer);

  switch (lexer->currentChar) {
  case '=': {
    if (peekChar(lexer) == '=') {
      unsigned char currentChar = lexer->currentChar;
      readChar(lexer);

      char *literal = malloc(3);
      if (literal == NULL) {
        abort();
      }
      literal[0] = currentChar;
      literal[1] = lexer->currentChar;
      literal[2] = '\0';

      token = newToken(EQ, literal);
    } else {
      token = newToken(ASSIGN, createSingleCharLiteral(lexer->currentChar));
    }
    break;
  }

  case '+': {
    token = newToken(PLUS, createSingleCharLiteral(lexer->currentChar));
    break;
  }

  case '-': {
    token = newToken(MINUS, createSingleCharLiteral(lexer->currentChar));
    break;
  }

  case '!': {
    if (peekChar(lexer) == '=') {
      unsigned char currentChar = lexer->currentChar;
      readChar(lexer);

      char *literal = malloc(3);
      if (literal == NULL) {
        abort();
      }
      literal[0] = currentChar;
      literal[1] = lexer->currentChar;
      literal[2] = '\0';

      token = newToken(NOT_EQ, literal);
    } else {
      token = newToken(BANG, createSingleCharLiteral(lexer->currentChar));
    }
    break;
  }

  case '*': {
    token = newToken(ASTERISK, createSingleCharLiteral(lexer->currentChar));
    break;
  }

  case '/': {
    token = newToken(SLASH, createSingleCharLiteral(lexer->currentChar));
    break;
  }

  case '<': {
    token = newToken(LT, createSingleCharLiteral(lexer->currentChar));
    break;
  }

  case '>': {
    token = newToken(GT, createSingleCharLiteral(lexer->currentChar));
    break;
  }

  case ',': {
    token = newToken(COMMA, createSingleCharLiteral(lexer->currentChar));
    break;
  }

  case ';': {
    token = newToken(SEMICOLON, createSingleCharLiteral(lexer->currentChar));
    break;
  }

  case ':': {
    token = newToken(COLON, createSingleCharLiteral(lexer->currentChar));
    break;
  }

  case '(': {
    token = newToken(LPAREN, createSingleCharLiteral(lexer->currentChar));
    break;
  }

  case ')': {
    token = newToken(RPAREN, createSingleCharLiteral(lexer->currentChar));
    break;
  }

  case '{': {
    token = newToken(LBRACE, createSingleCharLiteral(lexer->currentChar));
    break;
  }

  case '}': {
    token = newToken(RBRACE, createSingleCharLiteral(lexer->currentChar));
    break;
  }

  case '[': {
    token = newToken(LBRACKET, createSingleCharLiteral(lexer->currentChar));
    break;
  }

  case ']': {
    token = newToken(RBRACKET, createSingleCharLiteral(lexer->currentChar));
    break;
  }

  case '"': {
    token.type = STRING;
    token.literal = readString(lexer);
    break;
  }

  case 0: {
    token.literal = createSingleCharLiteral(0);
    token.type = EOF_TOK;
    break;
  }
  default: {
    if (isLetter(lexer->currentChar)) {
      token.literal = readIdentifier(lexer);
      token.type = lookupIdentifier(token.literal);
      return token;
    } else if (isDigit(lexer->currentChar)) {
      token.literal = readNumber(lexer);
      token.type = INT;
      return token;
    } else {
      token = newToken(ILLEGAL, createSingleCharLiteral(lexer->currentChar));
    }
    break;
  }
  }

  readChar(lexer);
  return token;
};
