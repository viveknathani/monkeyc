#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../ast/ast.h"
#include "../lexer/lexer.h"
#include "../parser/parser.h"

void testParseEmptyProgram() {
  Lexer *lexer = newLexer("");
  Parser *parser = newParser(lexer);

  Program *program = parseProgram(parser);
  assert(program != NULL);
  assert(program->statementCount == 0);

  printf("✅ testParseEmptyProgram passed\n");

  freeProgram(program);
  freeParser(parser);
  free(lexer);
}

void testParseLetStatementBasic() {
  const char *input = "let x = 5;";
  Lexer *lexer = newLexer((char *)input);
  Parser *parser = newParser(lexer);

  Program *program = parseProgram(parser);
  assert(program != NULL);
  assert(program->statementCount == 1);

  Statement *stmt = program->statements[0];
  assert(strcmp(stmt->type, NODE_LET_STATEMENT) == 0);

  LetStatement *letStmt = stmt->letStatement;
  assert(strcmp(letStmt->name->value, "x") == 0);

  printf("✅ testParseLetStatementBasic passed\n");

  freeProgram(program);
  freeParser(parser);
  free(lexer);
}

void testPrintComplexProgram() {
  const char *input = "let getAge = fn(user) {\n"
                      "  return user[\"age\"];\n"
                      "};\n"
                      "let person = {\"name\": \"Alice\", \"age\": 30};\n"
                      "let age = getAge(person);";

  Lexer *lexer = newLexer((char *)input);
  Parser *parser = newParser(lexer);

  Program *program = parseProgram(parser);
  assert(program != NULL);
  printf("\n🏁 now attempting to print...\n\n");

  printf("🗂️ Source program: \n%s\n\n", input);

  printf("🌲 AST: \n");
  printProgram(program);
}

int main() {
  testParseEmptyProgram();
  testParseLetStatementBasic();
  testPrintComplexProgram();
  printf("✅ All parser tests passed\n");
  return 0;
}
