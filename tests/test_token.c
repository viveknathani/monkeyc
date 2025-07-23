#include "../token/token.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void testLookupIdentifier() {
  assert(strcmp(lookupIdentifier("fn"), FUNCTION) == 0);
  assert(strcmp(lookupIdentifier("let"), LET) == 0);
  assert(strcmp(lookupIdentifier("true"), TRUE_TOK) == 0);
  assert(strcmp(lookupIdentifier("false"), FALSE_TOK) == 0);
  assert(strcmp(lookupIdentifier("if"), IF) == 0);
  assert(strcmp(lookupIdentifier("else"), ELSE) == 0);
  assert(strcmp(lookupIdentifier("return"), RETURN) == 0);
  assert(strcmp(lookupIdentifier("foobar"), IDENTIFIER) == 0);
  assert(strcmp(lookupIdentifier("x"), IDENTIFIER) == 0);
  assert(strcmp(lookupIdentifier("returnx"), IDENTIFIER) == 0);

  printf("✅ testLookupIdentifier passed\n");
}

int main() {
  testLookupIdentifier();
  return 0;
}
