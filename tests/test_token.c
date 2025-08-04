#include "../token/token.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// test the newToken function with various token types
void testNewToken() {
  // test creating tokens with different types
  Token intToken = newToken(INT, "42");
  assert(strcmp(intToken.type, INT) == 0);
  assert(strcmp(intToken.literal, "42") == 0);
  
  Token identifierToken = newToken(IDENTIFIER, "myVar");
  assert(strcmp(identifierToken.type, IDENTIFIER) == 0);
  assert(strcmp(identifierToken.literal, "myVar") == 0);
  
  Token operatorToken = newToken(PLUS, "+");
  assert(strcmp(operatorToken.type, PLUS) == 0);
  assert(strcmp(operatorToken.literal, "+") == 0);
  
  // test with empty string literal
  Token emptyToken = newToken(EOF_TOK, "");
  assert(strcmp(emptyToken.type, EOF_TOK) == 0);
  assert(strcmp(emptyToken.literal, "") == 0);
  
  printf("✅ testNewToken passed\n");
}

// test keyword lookup functionality
void testLookupIdentifier() {
  // test all keywords
  assert(strcmp(lookupIdentifier("fn"), FUNCTION) == 0);
  assert(strcmp(lookupIdentifier("let"), LET) == 0);
  assert(strcmp(lookupIdentifier("true"), TRUE_TOK) == 0);
  assert(strcmp(lookupIdentifier("false"), FALSE_TOK) == 0);
  assert(strcmp(lookupIdentifier("if"), IF) == 0);
  assert(strcmp(lookupIdentifier("else"), ELSE) == 0);
  assert(strcmp(lookupIdentifier("return"), RETURN) == 0);
  
  // test non-keywords (should return IDENTIFIER)
  assert(strcmp(lookupIdentifier("foobar"), IDENTIFIER) == 0);
  assert(strcmp(lookupIdentifier("x"), IDENTIFIER) == 0);
  assert(strcmp(lookupIdentifier("returnx"), IDENTIFIER) == 0);
  assert(strcmp(lookupIdentifier("function"), IDENTIFIER) == 0);  // Not "fn"
  assert(strcmp(lookupIdentifier("True"), IDENTIFIER) == 0);      // Case sensitive
  
  printf("✅ testLookupIdentifier passed\n");
}

// test edge cases and error conditions
void testEdgeCases() {
  // test NULL input (should not crash and return IDENTIFIER)
  assert(strcmp(lookupIdentifier(NULL), IDENTIFIER) == 0);
  
  // test empty string
  assert(strcmp(lookupIdentifier(""), IDENTIFIER) == 0);
  
  // test case sensitivity
  assert(strcmp(lookupIdentifier("FN"), IDENTIFIER) == 0);
  assert(strcmp(lookupIdentifier("LET"), IDENTIFIER) == 0);
  assert(strcmp(lookupIdentifier("IF"), IDENTIFIER) == 0);
  
  // test partial matches
  assert(strcmp(lookupIdentifier("f"), IDENTIFIER) == 0);
  assert(strcmp(lookupIdentifier("fn_"), IDENTIFIER) == 0);
  assert(strcmp(lookupIdentifier("_fn"), IDENTIFIER) == 0);
  
  // test longer strings containing keywords
  assert(strcmp(lookupIdentifier("function"), IDENTIFIER) == 0);
  assert(strcmp(lookupIdentifier("letter"), IDENTIFIER) == 0);
  assert(strcmp(lookupIdentifier("ifelse"), IDENTIFIER) == 0);
  
  printf("✅ testEdgeCases passed\n");
}

// test token creation with various literal types
void testTokenLiterals() {
  // test with string literals
  Token stringToken = newToken(STRING, "\"hello world\"");
  assert(strcmp(stringToken.type, STRING) == 0);
  assert(strcmp(stringToken.literal, "\"hello world\"") == 0);
  
  // test with numeric literals
  Token negativeToken = newToken(INT, "-123");
  assert(strcmp(negativeToken.type, INT) == 0);
  assert(strcmp(negativeToken.literal, "-123") == 0);
  
  // test with special characters
  Token specialToken = newToken(IDENTIFIER, "var_name_123");
  assert(strcmp(specialToken.type, IDENTIFIER) == 0);
  assert(strcmp(specialToken.literal, "var_name_123") == 0);
  
  printf("✅ testTokenLiterals passed\n");
}

// test token cloning functionality
void testCloneToken() {
  // test cloning a simple token
  Token original = newToken(IDENTIFIER, "myVar");
  Token cloned = cloneToken(original);
  
  // verify the clone has the same type and literal content
  assert(strcmp(cloned.type, original.type) == 0);
  assert(strcmp(cloned.literal, original.literal) == 0);
  
  // verify they are different memory locations (deep copy)
  assert(cloned.literal != original.literal);
  
  // test cloning different token types
  Token intToken = newToken(INT, "42");
  Token clonedInt = cloneToken(intToken);
  assert(strcmp(clonedInt.type, INT) == 0);
  assert(strcmp(clonedInt.literal, "42") == 0);
  assert(clonedInt.literal != intToken.literal);
  
  // test cloning keyword token
  Token keywordToken = newToken(FUNCTION, "fn");
  Token clonedKeyword = cloneToken(keywordToken);
  assert(strcmp(clonedKeyword.type, FUNCTION) == 0);
  assert(strcmp(clonedKeyword.literal, "fn") == 0);
  assert(clonedKeyword.literal != keywordToken.literal);
  
  // test cloning empty string
  Token emptyToken = newToken(EOF_TOK, "");
  Token clonedEmpty = cloneToken(emptyToken);
  assert(strcmp(clonedEmpty.type, EOF_TOK) == 0);
  assert(strcmp(clonedEmpty.literal, "") == 0);
  assert(clonedEmpty.literal != emptyToken.literal);
  
  // cleanup cloned tokens (since they use strdup)
  free((char*)cloned.literal);
  free((char*)clonedInt.literal);
  free((char*)clonedKeyword.literal);
  free((char*)clonedEmpty.literal);
  
  printf("✅ testCloneToken passed\n");
}

int main() {
  printf("running token module tests...\n\n");
  
  testNewToken();
  testLookupIdentifier();
  testEdgeCases();
  testTokenLiterals();
  testCloneToken();
  
  printf("\n🎉 all token tests passed!\n");
  return 0;
}
