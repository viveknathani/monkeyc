#include "token.h"
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

// internal structure for keyword mapping
typedef struct {
  const char *keyword;
  TokenType type;
} KeywordMap;

// static array of keyword mappings
// this array maps MonkeyC language keywords to their corresponding token types
// the array is searched linearly, which is efficient for the small number of keywords
static const KeywordMap keywords[] = {
    {"fn", FUNCTION}, 
    {"let", LET},   
    {"true", TRUE_TOK}, 
    {"false", FALSE_TOK},
    {"if", IF},       
    {"else", ELSE}, 
    {"return", RETURN},
};

#define KEYWORD_COUNT (sizeof(keywords) / sizeof(keywords[0]))

Token newToken(TokenType type, const char *literal) {
  Token token;
  token.type = type;
  token.literal = literal;
  return token;
}

TokenType lookupIdentifier(const char *identifier) {
  // input validation
  if (identifier == NULL) {
    return IDENTIFIER;
  }
  
  // linear search through keywords array
  for (size_t i = 0; i < KEYWORD_COUNT; ++i) {
    if (strcmp(identifier, keywords[i].keyword) == 0) {
      return keywords[i].type;
    }
  }

  return IDENTIFIER;
}

Token cloneToken(Token original) {
  Token t;
  t.type = original.type;
  t.literal = strdup(original.literal);
  return t;
}
