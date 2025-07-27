#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PREC_LOWEST 1
#define PREC_EQUALS 2
#define PREC_LESSGREATER 3
#define PREC_SUM 4
#define PREC_PRODUCT 5
#define PREC_PREFIX 6
#define PREC_CALL 7
#define PREC_INDEX 8

typedef struct {
  const char *tokenType;
  int precedence;
} PrecedenceEntry;

static PrecedenceEntry precedenceTable[] = {
    {EQ, PREC_EQUALS},      {NOT_EQ, PREC_EQUALS},    {LT, PREC_LESSGREATER},
    {GT, PREC_LESSGREATER}, {PLUS, PREC_SUM},         {MINUS, PREC_SUM},
    {SLASH, PREC_PRODUCT},  {ASTERISK, PREC_PRODUCT}, {LPAREN, PREC_CALL},
    {LBRACKET, PREC_INDEX},
};

Parser *newParser(Lexer *lexer) {
  Parser *parser = malloc(sizeof(Parser));
  parser->lexer = lexer;
  parser->errors = NULL;
  parser->errorCount = 0;
  parser->prefixCount = 0;
  parser->infixCount = 0;

  registerPrefix(parser, IDENTIFIER, parseIdentifier);
  registerPrefix(parser, INT, parseIntegerLiteral);
  registerPrefix(parser, TRUE_TOK, parseBoolean);
  registerPrefix(parser, FALSE_TOK, parseBoolean);
  registerPrefix(parser, BANG, parsePrefixExpression);
  registerPrefix(parser, MINUS, parsePrefixExpression);
  registerPrefix(parser, LPAREN, parseGroupedExpression);
  registerPrefix(parser, IF, parseIfExpression);
  registerPrefix(parser, FUNCTION, parseFunctionLiteral);
  registerPrefix(parser, STRING, parseStringLiteral);
  registerPrefix(parser, LBRACKET, parseArrayLiteral);
  registerPrefix(parser, LBRACE, parseHashLiteral);
  registerInfix(parser, PLUS, parseInfixExpression);
  registerInfix(parser, MINUS, parseInfixExpression);
  registerInfix(parser, SLASH, parseInfixExpression);
  registerInfix(parser, ASTERISK, parseInfixExpression);
  registerInfix(parser, EQ, parseInfixExpression);
  registerInfix(parser, NOT_EQ, parseInfixExpression);
  registerInfix(parser, LT, parseInfixExpression);
  registerInfix(parser, GT, parseInfixExpression);
  registerInfix(parser, LPAREN, parseCallExpression);
  registerInfix(parser, LBRACKET, parseIndexExpression);

  nextTokenParser(parser);
  nextTokenParser(parser);
  return parser;
}

void freeParser(Parser *parser) {
  for (int i = 0; i < parser->errorCount; i++)
    free(parser->errors[i]);
  free(parser->errors);
  free(parser);
}

char **parserErrors(Parser *parser, int *count) {
  *count = parser->errorCount;
  return parser->errors;
}

void parserAddError(Parser *parser, const char *message) {
  parser->errors =
      realloc(parser->errors, sizeof(char *) * (parser->errorCount + 1));
  parser->errors[parser->errorCount++] = strdup(message);
}

void registerPrefix(Parser *parser, const char *tokenType, PrefixParseFn fn) {
  parser->prefixFns[parser->prefixCount++] = (PrefixEntry){tokenType, fn};
}

void registerInfix(Parser *parser, const char *tokenType, InfixParseFn fn) {
  parser->infixFns[parser->infixCount++] = (InfixEntry){tokenType, fn};
}

PrefixParseFn getPrefixFn(Parser *parser, const char *tokenType) {
  for (int i = 0; i < parser->prefixCount; i++) {
    if (strcmp(parser->prefixFns[i].tokenType, tokenType) == 0)
      return parser->prefixFns[i].fn;
  }
  return NULL;
}

InfixParseFn getInfixFn(Parser *parser, const char *tokenType) {
  for (int i = 0; i < parser->infixCount; i++) {
    if (strcmp(parser->infixFns[i].tokenType, tokenType) == 0)
      return parser->infixFns[i].fn;
  }
  return NULL;
}

void nextTokenParser(Parser *parser) {
  parser->currentToken = parser->peekToken;
  parser->peekToken = nextToken(parser->lexer);
}

int currentTokenIs(Parser *parser, const char *type) {
  return strcmp(parser->currentToken.type, type) == 0;
}

int peekTokenIs(Parser *parser, const char *type) {
  return strcmp(parser->peekToken.type, type) == 0;
}

int expectPeek(Parser *parser, const char *type) {
  if (peekTokenIs(parser, type)) {
    nextTokenParser(parser);
    return 1;
  } else {
    char msg[128];
    snprintf(msg, sizeof(msg), "expected next token to be %s, got %s instead",
             type, parser->peekToken.type);
    parserAddError(parser, msg);
    return 0;
  }
}

int peekPrecedence(Parser *parser) {
  for (size_t i = 0; i < sizeof(precedenceTable) / sizeof(PrecedenceEntry);
       i++) {
    if (strcmp(parser->peekToken.type, precedenceTable[i].tokenType) == 0)
      return precedenceTable[i].precedence;
  }
  return PREC_LOWEST;
}

int currentPrecedence(Parser *parser) {
  for (size_t i = 0; i < sizeof(precedenceTable) / sizeof(PrecedenceEntry);
       i++) {
    if (strcmp(parser->currentToken.type, precedenceTable[i].tokenType) == 0)
      return precedenceTable[i].precedence;
  }
  return PREC_LOWEST;
}

Program *parseProgram(Parser *parser) {
  Program *program = malloc(sizeof(Program));
  program->type = NODE_PROGRAM;
  program->statementCount = 0;
  program->statements = NULL;

  while (!currentTokenIs(parser, EOF_TOK)) {
    Statement *stmt = parseStatement(parser);
    if (stmt != NULL) {
      program->statements =
          realloc(program->statements,
                  sizeof(Statement *) * (program->statementCount + 1));
      program->statements[program->statementCount++] = stmt;
    }
    nextTokenParser(parser);
  }

  return program;
}

Statement *parseStatement(Parser *parser) {
  if (currentTokenIs(parser, LET)) {
    return parseLetStatement(parser);
  } else if (currentTokenIs(parser, RETURN)) {
    return parseReturnStatement(parser);
  } else {
    return parseExpressionStatement(parser);
  }
}

Statement *parseLetStatement(Parser *parser) {
  Token letToken = parser->currentToken;

  if (!expectPeek(parser, IDENTIFIER)) {
    return NULL;
  }

  Identifier *name =
      newIdentifier(parser->currentToken, parser->currentToken.literal);

  if (!expectPeek(parser, ASSIGN)) {
    return NULL;
  }

  nextTokenParser(parser);
  Expression *value = parseExpression(parser, PREC_LOWEST);

  if (peekTokenIs(parser, SEMICOLON)) {
    nextTokenParser(parser);
  }

  return wrapLetStatement(newLetStatement(letToken, name, value));
}

Statement *parseReturnStatement(Parser *parser) {
  Token returnToken = parser->currentToken;

  nextTokenParser(parser);
  Expression *returnValue = parseExpression(parser, PREC_LOWEST);

  if (peekTokenIs(parser, SEMICOLON)) {
    nextTokenParser(parser);
  }

  return wrapReturnStatement(newReturnStatement(returnToken, returnValue));
}

Statement *parseExpressionStatement(Parser *parser) {
  Token token = parser->currentToken;
  Expression *expression = parseExpression(parser, PREC_LOWEST);

  if (peekTokenIs(parser, SEMICOLON)) {
    nextTokenParser(parser);
  }

  return wrapExpressionStatement(newExpressionStatement(token, expression));
}

Expression *parseExpression(Parser *parser, int precedence) {
  PrefixParseFn prefixFn = getPrefixFn(parser, parser->currentToken.type);
  if (prefixFn == NULL) {
    char msg[128];
    snprintf(msg, sizeof(msg), "no prefix parse function for %s found",
             parser->currentToken.type);
    parserAddError(parser, msg);
    return NULL;
  }

  Expression *leftExp = prefixFn(parser);

  while (!peekTokenIs(parser, SEMICOLON) &&
         precedence < peekPrecedence(parser)) {
    InfixParseFn infixFn = getInfixFn(parser, parser->peekToken.type);
    if (infixFn == NULL) {
      return leftExp;
    }

    nextTokenParser(parser);
    leftExp = infixFn(parser, leftExp);
  }

  return leftExp;
}

Expression *parseIdentifier(Parser *parser) {
  return wrapIdentifier(
      newIdentifier(parser->currentToken, parser->currentToken.literal));
}

Expression *parseIntegerLiteral(Parser *parser) {
  long long value = atoi(parser->currentToken.literal);
  return wrapIntegerLiteral(newIntegerLiteral(parser->currentToken, value));
}

Expression *parsePrefixExpression(Parser *parser) {
  Token token = parser->currentToken;
  char *op = strdup(parser->currentToken.literal);

  nextTokenParser(parser);
  Expression *right = parseExpression(parser, PREC_PREFIX);

  return wrapPrefixExpression(newPrefixExpression(token, op, right));
}

Expression *parseInfixExpression(Parser *parser, Expression *left) {
  Token token = parser->currentToken;
  char *op = strdup(parser->currentToken.literal);
  int precedence = currentPrecedence(parser);

  nextTokenParser(parser);
  Expression *right = parseExpression(parser, precedence);

  return wrapInfixExpression(newInfixExpression(token, left, op, right));
}

Expression *parseGroupedExpression(Parser *parser) {
  nextTokenParser(parser);
  Expression *exp = parseExpression(parser, PREC_LOWEST);

  if (!expectPeek(parser, RPAREN)) {
    return NULL;
  }

  return exp;
}

Expression *parseIfExpression(Parser *parser) {
  Token token = parser->currentToken;

  if (!expectPeek(parser, LPAREN)) {
    return NULL;
  }

  nextTokenParser(parser);
  Expression *condition = parseExpression(parser, PREC_LOWEST);

  if (!expectPeek(parser, RPAREN)) {
    return NULL;
  }

  if (!expectPeek(parser, LBRACE)) {
    return NULL;
  }

  BlockStatement *consequence = parseBlockStatement(parser);
  BlockStatement *alternative = NULL;

  if (peekTokenIs(parser, ELSE)) {
    nextTokenParser(parser);

    if (!expectPeek(parser, LBRACE)) {
      return NULL;
    }

    alternative = parseBlockStatement(parser);
  }

  return wrapIfExpression(
      newIfExpression(token, condition, consequence, alternative));
}

BlockStatement *parseBlockStatement(Parser *parser) {
  Token token = parser->currentToken;
  Statement **statements = NULL;
  int count = 0;

  nextTokenParser(parser);

  while (!currentTokenIs(parser, RBRACE) && !currentTokenIs(parser, EOF_TOK)) {
    Statement *stmt = parseStatement(parser);
    if (stmt != NULL) {
      statements = realloc(statements, sizeof(Statement *) * (count + 1));
      statements[count++] = stmt;
    }
    nextTokenParser(parser);
  }

  return newBlockStatement(token, statements, count);
}

Expression *parseFunctionLiteral(Parser *parser) {
  Token token = parser->currentToken;

  if (!expectPeek(parser, LPAREN)) {
    return NULL;
  }

  Identifier **parameters = NULL;
  int count = 0;

  if (peekTokenIs(parser, RPAREN)) {
    nextTokenParser(parser);
  } else {
    nextTokenParser(parser);

    parameters = realloc(parameters, sizeof(Identifier *) * (count + 1));
    parameters[count++] =
        newIdentifier(parser->currentToken, parser->currentToken.literal);

    while (peekTokenIs(parser, COMMA)) {
      nextTokenParser(parser);
      nextTokenParser(parser);
      parameters = realloc(parameters, sizeof(Identifier *) * (count + 1));
      parameters[count++] =
          newIdentifier(parser->currentToken, parser->currentToken.literal);
    }

    if (!expectPeek(parser, RPAREN)) {
      free(parameters);
      return NULL;
    }
  }

  if (!expectPeek(parser, LBRACE)) {
    free(parameters);
    return NULL;
  }

  BlockStatement *body = parseBlockStatement(parser);

  return wrapFunctionLiteral(
      newFunctionLiteral(token, parameters, count, body));
}

Expression *parseCallExpression(Parser *parser, Expression *function) {
  Token token = parser->currentToken;

  Expression **args = NULL;
  int count = 0;

  if (peekTokenIs(parser, RPAREN)) {
    nextTokenParser(parser);
  } else {
    nextTokenParser(parser);

    args = realloc(args, sizeof(Expression *) * (count + 1));
    args[count++] = parseExpression(parser, PREC_LOWEST);

    while (peekTokenIs(parser, COMMA)) {
      nextTokenParser(parser);
      nextTokenParser(parser);
      args = realloc(args, sizeof(Expression *) * (count + 1));
      args[count++] = parseExpression(parser, PREC_LOWEST);
    }

    if (!expectPeek(parser, RPAREN)) {
      free(args);
      return NULL;
    }
  }

  return wrapCallExpression(newCallExpression(token, function, args, count));
}

Expression *parseStringLiteral(Parser *parser) {
  return wrapStringLiteral(
      newStringLiteral(parser->currentToken, parser->currentToken.literal));
}

Expression *parseArrayLiteral(Parser *parser) {
  Token token = parser->currentToken;

  int count = 0;
  Expression **elements = parseExpressionList(parser, RBRACKET, &count);
  return wrapArrayLiteral(newArrayLiteral(token, elements, count));
}

Expression *parseHashLiteral(Parser *parser) {
  Token token = parser->currentToken;

  Expression **keys = NULL;
  Expression **values = NULL;
  int count = 0;

  while (!peekTokenIs(parser, RBRACE) && !peekTokenIs(parser, EOF_TOK)) {
    nextTokenParser(parser);
    Expression *key = parseExpression(parser, PREC_LOWEST);

    if (!expectPeek(parser, COLON)) {
      return NULL;
    }

    nextTokenParser(parser);
    Expression *value = parseExpression(parser, PREC_LOWEST);

    keys = realloc(keys, sizeof(Expression *) * (count + 1));
    values = realloc(values, sizeof(Expression *) * (count + 1));
    keys[count] = key;
    values[count] = value;
    count++;

    if (!peekTokenIs(parser, RBRACE) && !expectPeek(parser, COMMA)) {
      return NULL;
    }
  }

  if (!expectPeek(parser, RBRACE)) {
    return NULL;
  }

  return wrapHashLiteral(newHashLiteral(token, keys, values, count));
}
Expression *parseIndexExpression(Parser *parser, Expression *left) {
  Token token = parser->currentToken;

  nextTokenParser(parser);
  Expression *index = parseExpression(parser, PREC_LOWEST);

  if (!expectPeek(parser, RBRACKET)) {
    return NULL;
  }

  return wrapIndexExpression(newIndexExpression(token, left, index));
}

Expression *parseBoolean(Parser *parser) {
  int value = strcmp(parser->currentToken.type, TRUE_TOK) == 0 ? 1 : 0;
  return wrapBooleanLiteral(newBooleanLiteral(parser->currentToken, value));
}

Expression **parseExpressionList(Parser *parser, const char *endToken,
                                 int *count) {
  Expression **args = NULL;
  *count = 0;

  if (peekTokenIs(parser, endToken)) {
    nextTokenParser(parser);
    return args;
  }

  nextTokenParser(parser);
  args = realloc(args, sizeof(Expression *) * (*count + 1));
  args[(*count)++] = parseExpression(parser, PREC_LOWEST);

  while (peekTokenIs(parser, COMMA)) {
    nextTokenParser(parser);
    nextTokenParser(parser);

    args = realloc(args, sizeof(Expression *) * (*count + 1));
    args[(*count)++] = parseExpression(parser, PREC_LOWEST);
  }

  if (!expectPeek(parser, endToken)) {
    return NULL;
  }

  return args;
}

void printExpression(Expression *expr, int level, int isLast,
                     int parentLevels[]);
void printStatement(Statement *stmt, int level, int isLast, int parentLevels[]);
void printBlockStatement(BlockStatement *block, int level, int isLast, int parentLevels[]);

void printProgram(Program *program) {
  printf("Program\n");
  int parentLevels[32] = {0};

  for (int i = 0; i < program->statementCount; i++) {
    int isLast = (i == program->statementCount - 1);
    printStatement(program->statements[i], 1, isLast, parentLevels);
  }
}

static void printIndent(int level, int isLast, int parentLevels[]) {
  for (int i = 0; i < level - 1; i++) {
    if (parentLevels[i]) {
      printf("│   ");
    } else {
      printf("    ");
    }
  }
  if (level > 0) {
    if (isLast) {
      printf("└── ");
    } else {
      printf("├── ");
    }
  }
}

void printBlockStatement(BlockStatement *block, int level, int isLast, int parentLevels[]) {
  if (!block) return;

  printIndent(level, isLast, parentLevels);
  printf("BlockStatement\n");

  for (int i = 0; i < block->count; i++) {
    int blockIsLast = (i == block->count - 1);
    printStatement(block->statements[i], level + 1, blockIsLast, parentLevels);
  }
}

void printStatement(Statement *stmt, int level, int isLast,
                    int parentLevels[]) {
  if (!stmt)
    return;

  parentLevels[level - 1] = !isLast;

  if (strcmp(stmt->type, NODE_LET_STATEMENT) == 0) {
    printIndent(level, isLast, parentLevels);
    printf("LetStatement\n");

    printIndent(level + 1, 0, parentLevels);
    printf("Identifier: %s\n", stmt->letStatement->name->value);

    if (stmt->letStatement->value) {
      printExpression(stmt->letStatement->value, level + 1, 1, parentLevels);
    }
  } else if (strcmp(stmt->type, NODE_RETURN_STATEMENT) == 0) {
    printIndent(level, isLast, parentLevels);
    printf("ReturnStatement\n");
    if (stmt->returnStatement->return_value) {
      printExpression(stmt->returnStatement->return_value, level + 1, 1,
                      parentLevels);
    }
  } else if (strcmp(stmt->type, NODE_EXPRESSION_STATEMENT) == 0) {
    printIndent(level, isLast, parentLevels);
    printf("ExpressionStatement\n");
    if (stmt->expressionStatement->expression) {
      printExpression(stmt->expressionStatement->expression, level + 1, 1,
                      parentLevels);
    }
  } else if (strcmp(stmt->type, NODE_BLOCK_STATEMENT) == 0) {
    printIndent(level, isLast, parentLevels);
    printf("BlockStatement\n");
    for (int i = 0; i < stmt->blockStatement->count; i++) {
      int blockIsLast = (i == stmt->blockStatement->count - 1);
      printStatement(stmt->blockStatement->statements[i], level + 1,
                     blockIsLast, parentLevels);
    }
  }
}

void printExpression(Expression *expr, int level, int isLast,
                     int parentLevels[]) {
  if (!expr)
    return;

  parentLevels[level - 1] = !isLast;

  if (strcmp(expr->type, NODE_IDENTIFIER) == 0) {
    printIndent(level, isLast, parentLevels);
    printf("Identifier: %s\n", expr->identifier->value);
  } else if (strcmp(expr->type, NODE_INTEGER_LITERAL) == 0) {
    printIndent(level, isLast, parentLevels);
    printf("IntegerLiteral: %lld\n", expr->integerLiteral->value);
  } else if (strcmp(expr->type, NODE_STRING_LITERAL) == 0) {
    printIndent(level, isLast, parentLevels);
    printf("StringLiteral: \"%s\"\n", expr->stringLiteral->value);
  } else if (strcmp(expr->type, NODE_BOOLEAN) == 0) {
    printIndent(level, isLast, parentLevels);
    printf("Boolean: %s\n", expr->booleanLiteral->value ? "true" : "false");
  } else if (strcmp(expr->type, NODE_PREFIX_EXPRESSION) == 0) {
    printIndent(level, isLast, parentLevels);
    printf("PrefixExpression\n");

    printIndent(level + 1, 0, parentLevels);
    printf("Operator: %s\n", expr->prefixExpression->op);

    printExpression(expr->prefixExpression->right, level + 1, 1, parentLevels);
  } else if (strcmp(expr->type, NODE_INFIX_EXPRESSION) == 0) {
    printIndent(level, isLast, parentLevels);
    printf("InfixExpression\n");

    printExpression(expr->infixExpression->left, level + 1, 0, parentLevels);

    printIndent(level + 1, 0, parentLevels);
    printf("Operator: %s\n", expr->infixExpression->op);

    printExpression(expr->infixExpression->right, level + 1, 1, parentLevels);
  } else if (strcmp(expr->type, NODE_IF_EXPRESSION) == 0) {
    printIndent(level, isLast, parentLevels);
    printf("IfExpression\n");

    printIndent(level + 1, 0, parentLevels);
    printf("Condition:\n");
    printExpression(expr->ifExpression->condition, level + 2, 1, parentLevels);

    printIndent(level + 1, expr->ifExpression->alternative ? 0 : 1,
                parentLevels);
    printf("Consequence:\n");
    printBlockStatement(expr->ifExpression->consequence, level + 2, 1,
                       parentLevels);

    if (expr->ifExpression->alternative) {
      printIndent(level + 1, 1, parentLevels);
      printf("Alternative:\n");
      printBlockStatement(expr->ifExpression->alternative, level + 2, 1,
                         parentLevels);
    }
  } else if (strcmp(expr->type, NODE_FUNCTION_LITERAL) == 0) {
    printIndent(level, isLast, parentLevels);
    printf("FunctionLiteral\n");

    if (expr->functionLiteral->param_count > 0) {
      printIndent(level + 1, 0, parentLevels);
      printf("Parameters:\n");
      for (int i = 0; i < expr->functionLiteral->param_count; i++) {
        int paramIsLast = (i == expr->functionLiteral->param_count - 1);
        printIndent(level + 2, paramIsLast, parentLevels);
        printf("Identifier: %s\n", expr->functionLiteral->parameters[i]->value);
      }
    }

    printIndent(level + 1, 1, parentLevels);
    printf("Body:\n");
    printBlockStatement(expr->functionLiteral->body, level + 2, 1, parentLevels);
  } else if (strcmp(expr->type, NODE_CALL_EXPRESSION) == 0) {
    printIndent(level, isLast, parentLevels);
    printf("CallExpression\n");

    printIndent(level + 1, expr->callExpression->arg_count == 0 ? 1 : 0,
                parentLevels);
    printf("Function:\n");
    printExpression(expr->callExpression->function, level + 2, 1, parentLevels);

    if (expr->callExpression->arg_count > 0) {
      printIndent(level + 1, 1, parentLevels);
      printf("Arguments:\n");
      for (int i = 0; i < expr->callExpression->arg_count; i++) {
        int argIsLast = (i == expr->callExpression->arg_count - 1);
        printExpression(expr->callExpression->arguments[i], level + 2,
                        argIsLast, parentLevels);
      }
    }
  } else if (strcmp(expr->type, NODE_ARRAY_LITERAL) == 0) {
    printIndent(level, isLast, parentLevels);
    printf("ArrayLiteral\n");
    for (int i = 0; i < expr->arrayLiteral->count; i++) {
      int arrayIsLast = (i == expr->arrayLiteral->count - 1);
      printExpression(expr->arrayLiteral->elements[i], level + 1, arrayIsLast,
                      parentLevels);
    }
  } else if (strcmp(expr->type, NODE_INDEX_EXPRESSION) == 0) {
    printIndent(level, isLast, parentLevels);
    printf("IndexExpression\n");
    printExpression(expr->indexExpression->left, level + 1, 0, parentLevels);
    printExpression(expr->indexExpression->index, level + 1, 1, parentLevels);
  } else if (strcmp(expr->type, NODE_HASH_LITERAL) == 0) {
    printIndent(level, isLast, parentLevels);
    printf("HashLiteral\n");
    for (int i = 0; i < expr->hashLiteral->count; i++) {
      int pairIsLast = (i == expr->hashLiteral->count - 1);

      printIndent(level + 1, pairIsLast, parentLevels);
      printf("KeyValuePair\n");

      printExpression(expr->hashLiteral->keys[i], level + 2, 0, parentLevels);
      printExpression(expr->hashLiteral->values[i], level + 2, 1, parentLevels);
    }
  }
}
