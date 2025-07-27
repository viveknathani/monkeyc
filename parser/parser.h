#ifndef PARSER_H
#define PARSER_H

#include "../ast/ast.h"
#include "../lexer/lexer.h"

typedef struct Parser Parser;
typedef Expression *(*PrefixParseFn)(Parser *);
typedef Expression *(*InfixParseFn)(Parser *, Expression *);

typedef struct {
  const char *tokenType;
  PrefixParseFn fn;
} PrefixEntry;

typedef struct {
  const char *tokenType;
  InfixParseFn fn;
} InfixEntry;

#define MAX_PARSE_FN_ENTRIES 64

struct Parser {
  Lexer *lexer;
  Token currentToken;
  Token peekToken;

  PrefixEntry prefixFns[MAX_PARSE_FN_ENTRIES];
  int prefixCount;

  InfixEntry infixFns[MAX_PARSE_FN_ENTRIES];
  int infixCount;

  char **errors;
  int errorCount;
};

Parser *newParser(Lexer *lexer);
Program *parseProgram(Parser *parser);
char **parserErrors(Parser *parser, int *count);
void registerPrefix(Parser *parser, const char *tokenType, PrefixParseFn fn);
void registerInfix(Parser *parser, const char *tokenType, InfixParseFn fn);
PrefixParseFn getPrefixFn(Parser *parser, const char *tokenType);
InfixParseFn getInfixFn(Parser *parser, const char *tokenType);
void nextTokenParser(Parser *parser);
int currentTokenIs(Parser *parser, const char *type);
int peekTokenIs(Parser *parser, const char *type);
int expectPeek(Parser *parser, const char *type);
void parserAddError(Parser *parser, const char *message);
int peekPrecedence(Parser *parser);
int currentPrecedence(Parser *parser);
Statement *parseStatement(Parser *parser);
Statement *parseLetStatement(Parser *parser);
Statement *parseReturnStatement(Parser *parser);
Statement *parseExpressionStatement(Parser *parser);
Expression *parseExpression(Parser *parser, int precedence);
Expression *parseIdentifier(Parser *parser);
Expression *parseIntegerLiteral(Parser *parser);
Expression *parseBoolean(Parser *parser);
Expression *parsePrefixExpression(Parser *parser);
Expression *parseInfixExpression(Parser *parser, Expression *left);
Expression *parseGroupedExpression(Parser *parser);
Expression *parseIfExpression(Parser *parser);
BlockStatement *parseBlockStatement(Parser *parser);
Expression *parseFunctionLiteral(Parser *parser);
Expression *parseCallExpression(Parser *parser, Expression *function);
Expression *parseStringLiteral(Parser *parser);
Expression *parseArrayLiteral(Parser *parser);
Expression *parseIndexExpression(Parser *parser, Expression *left);
Expression *parseHashLiteral(Parser *parser);
Expression **parseExpressionList(Parser *parser, const char *endToken,
                                 int *count);
void freeParser(Parser *parser);
void printProgram(Program *program);

#endif
