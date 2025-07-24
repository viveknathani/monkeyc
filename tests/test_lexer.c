#include "../lexer/lexer.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void testNewLexer() {
  assert(strcmp(newLexer("let x = 5;")->input, "let x = 5;") == 0);
  printf("✅ testNewLexer passed\n");
}

void testNextToken() {
  const char *input = "let x = 5;\n"
                      "let y = 10;\n"
                      "let foobar = 838383;\n"
                      "let str = \"hello\";\n"
                      "if (x < y) { return x; }\n"
                      "10 == 10;\n"
                      "5 != 6;\n";

  struct {
    TokenType type;
    const char *literal;
  } tests[] = {{LET, "let"},      {IDENTIFIER, "x"},
               {ASSIGN, "="},     {INT, "5"},
               {SEMICOLON, ";"},

               {LET, "let"},      {IDENTIFIER, "y"},
               {ASSIGN, "="},     {INT, "10"},
               {SEMICOLON, ";"},

               {LET, "let"},      {IDENTIFIER, "foobar"},
               {ASSIGN, "="},     {INT, "838383"},
               {SEMICOLON, ";"},

               {LET, "let"},      {IDENTIFIER, "str"},
               {ASSIGN, "="},     {STRING, "hello"},
               {SEMICOLON, ";"},

               {IF, "if"},        {LPAREN, "("},
               {IDENTIFIER, "x"}, {LT, "<"},
               {IDENTIFIER, "y"}, {RPAREN, ")"},
               {LBRACE, "{"},     {RETURN, "return"},
               {IDENTIFIER, "x"}, {SEMICOLON, ";"},
               {RBRACE, "}"},

               {INT, "10"},       {EQ, "=="},
               {INT, "10"},       {SEMICOLON, ";"},

               {INT, "5"},        {NOT_EQ, "!="},
               {INT, "6"},        {SEMICOLON, ";"},

               {EOF_TOK, ""}};

  Lexer *lexer = newLexer((char *)input);

  for (int i = 0; strcmp(tests[i].type, EOF_TOK) != 0; i++) {
    Token tok = nextToken(lexer);
    assert(tok.type == tests[i].type);
    assert(strcmp(tok.literal, tests[i].literal) == 0);
    printf("✅ Token %d passed: type=%s, literal=%s\n", i, tok.type,
           tok.literal);
    free(tok.literal);
  }

  printf("✅ testNextToken passed\n");
}

int main() {
  testNewLexer();
  testNextToken();
  return 0;
}
