#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../ast/ast.h"
#include "../lexer/lexer.h"
#include "../token/token.h"

Token makeToken(const char *type, const char *literal) {
  Token tok;
  tok.type = (char *)type;
  tok.literal = strdup(literal);
  return tok;
}

void testLetStatement() {
  Token tok = makeToken(LET, "let");
  Token nameTok = makeToken(IDENTIFIER, "myVar");
  Token valTok = makeToken(INT, "5");

  Identifier *name = newIdentifier(nameTok, "myVar");
  IntegerLiteral *value = newIntegerLiteral(valTok, 5);
  LetStatement *letStmt = newLetStatement(tok, name, wrapIntegerLiteral(value));

  Program *program = malloc(sizeof(Program));
  program->type = NODE_PROGRAM;
  program->statementCount = 1;
  program->statements = malloc(sizeof(Statement *) * 1);
  program->statements[0] = wrapLetStatement(letStmt);

  char *out = programToString(program);
  printf("✅ LetStatement: %s\n", out);
  assert(strcmp(out, "let myVar = 5;") == 0);

  free(out);
  freeProgram(program);
}

void testReturnStatement() {
  Token tok = makeToken(RETURN, "return");
  Token valTok = makeToken(INT, "10");

  IntegerLiteral *value = newIntegerLiteral(valTok, 10);
  ReturnStatement *ret = newReturnStatement(tok, wrapIntegerLiteral(value));

  Program *program = malloc(sizeof(Program));
  program->type = NODE_PROGRAM;
  program->statementCount = 1;
  program->statements = malloc(sizeof(Statement *) * 1);
  program->statements[0] = wrapReturnStatement(ret);

  char *out = programToString(program);
  printf("✅ ReturnStatement: %s\n", out);
  assert(strcmp(out, "return 10;") == 0);

  free(out);
  freeProgram(program);
}

void testInfixExpression() {
  Token letTok = makeToken(LET, "let");
  Token nameTok = makeToken(IDENTIFIER, "result");
  Token intTokLeft = makeToken(INT, "1");
  Token intTokRight = makeToken(INT, "2");
  Token plusTok = makeToken(PLUS, "+");

  Expression *left = wrapIntegerLiteral(newIntegerLiteral(intTokLeft, 1));
  Expression *right = wrapIntegerLiteral(newIntegerLiteral(intTokRight, 2));
  InfixExpression *infix = newInfixExpression(plusTok, left, "+", right);

  LetStatement *stmt = newLetStatement(letTok, newIdentifier(nameTok, "result"),
                                       wrapInfixExpression(infix));

  Program *program = malloc(sizeof(Program));
  program->type = NODE_PROGRAM;
  program->statementCount = 1;
  program->statements = malloc(sizeof(Statement *));
  program->statements[0] = wrapLetStatement(stmt);

  char *out = programToString(program);
  printf("✅ InfixExpression: %s\n", out);
  assert(strcmp(out, "let result = (1 + 2);") == 0);

  free(out);
  freeProgram(program);
}

void testIfExpression() {
  Token ifTok = makeToken(IF, "if");
  Token ltTok = makeToken(LT, "<");
  Token xTok = makeToken(IDENTIFIER, "x");
  Token yTok = makeToken(IDENTIFIER, "y");

  Expression *x = wrapIdentifier(newIdentifier(xTok, "x"));
  Expression *y = wrapIdentifier(newIdentifier(yTok, "y"));
  InfixExpression *cond = newInfixExpression(ltTok, x, "<", y);

  Token returnTok = makeToken(RETURN, "return");
  Expression *xRet = wrapIdentifier(newIdentifier(xTok, "x"));
  ReturnStatement *ret = newReturnStatement(returnTok, xRet);
  Statement **stmts = malloc(sizeof(Statement *));
  stmts[0] = wrapReturnStatement(ret);
  BlockStatement *block = newBlockStatement(makeToken(LBRACE, "{"), stmts, 1);

  IfExpression *ifExpr =
      newIfExpression(ifTok, wrapInfixExpression(cond), block, NULL);
  ExpressionStatement *exprStmt =
      newExpressionStatement(ifTok, wrapIfExpression(ifExpr));

  Program *program = malloc(sizeof(Program));
  program->type = NODE_PROGRAM;
  program->statementCount = 1;
  program->statements = malloc(sizeof(Statement *));
  program->statements[0] = wrapExpressionStatement(exprStmt);

  char *out = programToString(program);
  printf("✅ IfExpression: %s\n", out);
  assert(strcmp(out, "if(x < y)return x;") == 0);

  free(out);
  freeProgram(program);
}

void testFunctionLiteral() {
  Token fnTok = makeToken(FUNCTION, "fn");
  Token xTok = makeToken(IDENTIFIER, "x");
  Token yTok = makeToken(IDENTIFIER, "y");

  Identifier **params = malloc(sizeof(Identifier *) * 2);
  params[0] = newIdentifier(xTok, "x");
  params[1] = newIdentifier(yTok, "y");

  Token retTok = makeToken(RETURN, "return");
  InfixExpression *sum =
      newInfixExpression(makeToken(PLUS, "+"), wrapIdentifier(params[0]), "+",
                         wrapIdentifier(params[1]));

  ReturnStatement *ret = newReturnStatement(retTok, wrapInfixExpression(sum));
  Statement **stmts = malloc(sizeof(Statement *));
  stmts[0] = wrapReturnStatement(ret);
  BlockStatement *body = newBlockStatement(makeToken(LBRACE, "{"), stmts, 1);

  FunctionLiteral *fn = newFunctionLiteral(fnTok, params, 2, body);
  ExpressionStatement *exprStmt =
      newExpressionStatement(fnTok, wrapFunctionLiteral(fn));

  Program *program = malloc(sizeof(Program));
  program->type = NODE_PROGRAM;
  program->statementCount = 1;
  program->statements = malloc(sizeof(Statement *));
  program->statements[0] = wrapExpressionStatement(exprStmt);

  char *out = programToString(program);
  printf("✅ FunctionLiteral: %s\n", out);
  assert(strcmp(out, "fn(x, y)return (x + y);") == 0);

  free(out);
  freeProgram(program);
}

void testFullProgramAst() {
  const char *source = "let add = fn(x, y) {\n"
                       "  if (x < y) {\n"
                       "    return x;\n"
                       "  } else {\n"
                       "    return y;\n"
                       "  }\n"
                       "};\n"
                       "add(5, 10);";

  Lexer *lexer = newLexer((char *)source);
  Token tokens[512];
  int count = 0;

  while (1) {
    Token tok = nextToken(lexer);
    tokens[count++] = tok;
    if (strcmp(tok.type, EOF_TOK) == 0)
      break;
  }

  Identifier *param1 = newIdentifier(cloneToken(tokens[5]), tokens[5].literal);
  Identifier *param2 = newIdentifier(cloneToken(tokens[7]), tokens[7].literal);
  Identifier **params = malloc(sizeof(Identifier *) * 2);
  params[0] = param1;
  params[1] = param2;

  Expression *left =
      wrapIdentifier(newIdentifier(cloneToken(tokens[12]), tokens[12].literal));
  Expression *right =
      wrapIdentifier(newIdentifier(cloneToken(tokens[14]), tokens[14].literal));
  InfixExpression *cond =
      newInfixExpression(cloneToken(tokens[13]), left, "<", right);

  Expression *retVal1 =
      wrapIdentifier(newIdentifier(cloneToken(tokens[19]), tokens[19].literal));
  ReturnStatement *ret1 = newReturnStatement(cloneToken(tokens[18]), retVal1);
  Statement **cons = malloc(sizeof(Statement *));
  cons[0] = wrapReturnStatement(ret1);
  BlockStatement *consequence =
      newBlockStatement(cloneToken(tokens[17]), cons, 1);

  Expression *retVal2 =
      wrapIdentifier(newIdentifier(cloneToken(tokens[26]), tokens[26].literal));
  ReturnStatement *ret2 = newReturnStatement(cloneToken(tokens[25]), retVal2);
  Statement **alts = malloc(sizeof(Statement *));
  alts[0] = wrapReturnStatement(ret2);
  BlockStatement *alternative =
      newBlockStatement(cloneToken(tokens[24]), alts, 1);

  IfExpression *ifExpr =
      newIfExpression(cloneToken(tokens[10]), wrapInfixExpression(cond),
                      consequence, alternative);
  Statement **bodyStmts = malloc(sizeof(Statement *));
  bodyStmts[0] = wrapExpressionStatement(
      newExpressionStatement(cloneToken(tokens[10]), wrapIfExpression(ifExpr)));
  BlockStatement *fnBody =
      newBlockStatement(cloneToken(tokens[9]), bodyStmts, 1);

  FunctionLiteral *fn =
      newFunctionLiteral(cloneToken(tokens[3]), params, 2, fnBody);

  Identifier *addIdent =
      newIdentifier(cloneToken(tokens[1]), tokens[1].literal);
  LetStatement *let =
      newLetStatement(cloneToken(tokens[0]), addIdent, wrapFunctionLiteral(fn));

  Token calleeTok = cloneToken(tokens[29]);
  Expression *callee =
      wrapIdentifier(newIdentifier(calleeTok, calleeTok.literal));

  Expression **args = malloc(sizeof(Expression *) * 2);
  args[0] = wrapIntegerLiteral(
      newIntegerLiteral(cloneToken(tokens[31]), atoi(tokens[31].literal)));
  args[1] = wrapIntegerLiteral(
      newIntegerLiteral(cloneToken(tokens[33]), atoi(tokens[33].literal)));
  CallExpression *call =
      newCallExpression(cloneToken(tokens[29]), callee, args, 2);
  ExpressionStatement *callStmt =
      newExpressionStatement(cloneToken(tokens[29]), wrapCallExpression(call));

  Program *program = malloc(sizeof(Program));
  program->type = NODE_PROGRAM;
  program->statementCount = 2;
  program->statements = malloc(sizeof(Statement *) * 2);
  program->statements[0] = wrapLetStatement(let);
  program->statements[1] = wrapExpressionStatement(callStmt);

  char *out = programToString(program);
  printf("✅ Full Program AST: %s\n", out);
  assert(strstr(out, "let add = fn(x, y)") != NULL);
  assert(strstr(out, "add(5, 10)") != NULL);

  free(out);
  freeProgram(program);
}

int main() {
  testLetStatement();
  testReturnStatement();
  testInfixExpression();
  testIfExpression();
  testFunctionLiteral();
  testFullProgramAst();
  printf("✅ All AST tests passed\n");
  return 0;
}
