#include "token.h"
#include <stddef.h>
#include <string.h>

Token newToken(TokenType type, char *literal) {
  Token token;
  token.type = type;
  token.literal = literal;
  return token;
}

TokenType lookupIdentifier(const char *identifier) {
  for (size_t i = 0; i < KEYWORD_COUNT; ++i) {
    if (strcmp(identifier, keywords[i].keyword) == 0) {
      return keywords[i].type;
    }
  }

  return IDENTIFIER;
}
