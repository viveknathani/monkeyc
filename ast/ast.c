#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _POSIX_C_SOURCE
char *strdup(const char *s) {
  size_t len = strlen(s) + 1;
  char *copy = malloc(len);
  if (copy)
    memcpy(copy, s, len);
  return copy;
}
#endif

Token cloneToken(Token original) {
  Token t;
  t.type = original.type;
  t.literal = strdup(original.literal);
  return t;
}

LetStatement *newLetStatement(Token token, Identifier *name,
                              Expression *value) {
  LetStatement *stmt = malloc(sizeof(LetStatement));
  stmt->token = token;
  stmt->name = name;
  stmt->value = value;
  return stmt;
}

Identifier *newIdentifier(Token token, const char *value) {
  Identifier *ident = malloc(sizeof(Identifier));
  ident->token = cloneToken(token);
  ident->value = strdup(value);
  return ident;
}

IntegerLiteral *newIntegerLiteral(Token token, long long value) {
  IntegerLiteral *il = malloc(sizeof(IntegerLiteral));
  il->token = cloneToken(token);
  il->value = value;
  return il;
}

BooleanLiteral *newBooleanLiteral(Token token, int value) {
  BooleanLiteral *bl = malloc(sizeof(BooleanLiteral));
  bl->token = cloneToken(token);
  bl->value = value;
  return bl;
}

ExpressionStatement *newExpressionStatement(Token token, Expression *expr) {
  ExpressionStatement *stmt = malloc(sizeof(ExpressionStatement));
  stmt->token = cloneToken(token);
  stmt->expression = expr;
  return stmt;
}

ReturnStatement *newReturnStatement(Token token, Expression *value) {
  ReturnStatement *stmt = malloc(sizeof(ReturnStatement));
  stmt->token = cloneToken(token);
  stmt->return_value = value;
  return stmt;
}

PrefixExpression *newPrefixExpression(Token token, const char *op,
                                      Expression *right) {
  PrefixExpression *pe = malloc(sizeof(PrefixExpression));
  pe->token = token;
  pe->op = strdup(op);
  pe->right = right;
  return pe;
}

InfixExpression *newInfixExpression(Token token, Expression *left,
                                    const char *op, Expression *right) {
  InfixExpression *ie = malloc(sizeof(InfixExpression));
  ie->token = token;
  ie->left = left;
  ie->op = strdup(op);
  ie->right = right;
  return ie;
}

HashLiteral *newHashLiteral(Token token, Expression **keys, Expression **values,
                            int count) {
  HashLiteral *hl = malloc(sizeof(HashLiteral));
  hl->token = token;
  hl->keys = keys;
  hl->values = values;
  hl->count = count;
  return hl;
}

Expression *wrapIntegerLiteral(IntegerLiteral *il) {
  Expression *expr = malloc(sizeof(Expression));
  expr->type = NODE_INTEGER_LITERAL;
  expr->integerLiteral = il;
  return expr;
}

Expression *wrapBooleanLiteral(BooleanLiteral *bl) {
  Expression *expr = malloc(sizeof(Expression));
  expr->type = NODE_BOOLEAN;
  expr->booleanLiteral = bl;
  return expr;
}

Expression *wrapIdentifier(Identifier *id) {
  Expression *expr = malloc(sizeof(Expression));
  expr->type = NODE_IDENTIFIER;
  expr->identifier = id;
  return expr;
}

Expression *wrapPrefixExpression(PrefixExpression *pe) {
  Expression *expr = malloc(sizeof(Expression));
  expr->type = NODE_PREFIX_EXPRESSION;
  expr->prefixExpression = pe;
  return expr;
}

Expression *wrapInfixExpression(InfixExpression *ie) {
  Expression *expr = malloc(sizeof(Expression));
  expr->type = NODE_INFIX_EXPRESSION;
  expr->infixExpression = ie;
  return expr;
}

Expression *wrapHashLiteral(HashLiteral *hl) {
  Expression *expr = malloc(sizeof(Expression));
  expr->type = NODE_HASH_LITERAL;
  expr->hashLiteral = hl;
  return expr;
}

Statement *wrapLetStatement(LetStatement *stmt) {
  Statement *s = malloc(sizeof(Statement));
  s->type = NODE_LET_STATEMENT;
  s->letStatement = stmt;
  return s;
}

Statement *wrapExpressionStatement(ExpressionStatement *stmt) {
  Statement *s = malloc(sizeof(Statement));
  s->type = NODE_EXPRESSION_STATEMENT;
  s->expressionStatement = stmt;
  return s;
}

Statement *wrapReturnStatement(ReturnStatement *stmt) {
  Statement *s = malloc(sizeof(Statement));
  s->type = NODE_RETURN_STATEMENT;
  s->returnStatement = stmt;
  return s;
}

char *identifierToString(Identifier *ident) { return strdup(ident->value); }

char *integerLiteralToString(IntegerLiteral *il) {
  return strdup(il->token.literal);
}

char *booleanLiteralToString(BooleanLiteral *bl) {
  return strdup(bl->token.literal);
}

char *prefixExpressionToString(PrefixExpression *pe) {
  char *rightStr = expressionToString(pe->right);
  size_t len = strlen(pe->op) + strlen(rightStr) + 4;
  char *out = malloc(len);
  snprintf(out, len, "(%s%s)", pe->op, rightStr);
  free(rightStr);
  return out;
}

char *infixExpressionToString(InfixExpression *ie) {
  char *leftStr = expressionToString(ie->left);
  char *rightStr = expressionToString(ie->right);
  size_t len = strlen(leftStr) + strlen(ie->op) + strlen(rightStr) + 6;
  char *out = malloc(len);
  snprintf(out, len, "(%s %s %s)", leftStr, ie->op, rightStr);
  free(leftStr);
  free(rightStr);
  return out;
}

char *hashLiteralToString(HashLiteral *hl) {
  size_t bufsize = 256;
  char *out = malloc(bufsize);
  out[0] = '\0';
  strcat(out, "{");

  for (int i = 0; i < hl->count; i++) {
    char *k = expressionToString(hl->keys[i]);
    char *v = expressionToString(hl->values[i]);
    size_t newlen = strlen(out) + strlen(k) + strlen(v) + 4;

    if (newlen >= bufsize) {
      bufsize *= 2;
      out = realloc(out, bufsize);
    }
    strcat(out, k);
    strcat(out, ":");
    strcat(out, v);
    if (i < hl->count - 1)
      strcat(out, ", ");
    free(k);
    free(v);
  }

  strcat(out, "}");
  return out;
}

char *expressionToString(Expression *expr) {
  if (!expr)
    return strdup("");

  printf("➡️ expressionToString: %s\n", expr->type);

  if (strcmp(expr->type, NODE_IDENTIFIER) == 0) {
    return identifierToString(expr->identifier);
  } else if (strcmp(expr->type, NODE_INTEGER_LITERAL) == 0) {
    return integerLiteralToString(expr->integerLiteral);
  } else if (strcmp(expr->type, NODE_BOOLEAN) == 0) {
    return booleanLiteralToString(expr->booleanLiteral);
  } else if (strcmp(expr->type, NODE_STRING_LITERAL) == 0) {
    return stringLiteralToString(expr->stringLiteral);
  } else if (strcmp(expr->type, NODE_PREFIX_EXPRESSION) == 0) {
    return prefixExpressionToString(expr->prefixExpression);
  } else if (strcmp(expr->type, NODE_INFIX_EXPRESSION) == 0) {
    return infixExpressionToString(expr->infixExpression);
  } else if (strcmp(expr->type, NODE_IF_EXPRESSION) == 0) {
    return ifExpressionToString(expr->ifExpression);
  } else if (strcmp(expr->type, NODE_FUNCTION_LITERAL) == 0) {
    return functionLiteralToString(expr->functionLiteral);
  } else if (strcmp(expr->type, NODE_CALL_EXPRESSION) == 0) {
    return callExpressionToString(expr->callExpression);
  } else if (strcmp(expr->type, NODE_ARRAY_LITERAL) == 0) {
    return arrayLiteralToString(expr->arrayLiteral);
  } else if (strcmp(expr->type, NODE_INDEX_EXPRESSION) == 0) {
    return indexExpressionToString(expr->indexExpression);
  } else if (strcmp(expr->type, NODE_HASH_LITERAL) == 0) {
    return hashLiteralToString(expr->hashLiteral);
  }

  return strdup("<expr unknown>");
}

char *statementToString(Statement *stmt) {
  if (!stmt)
    return strdup("");

  if (strcmp(stmt->type, NODE_LET_STATEMENT) == 0) {
    LetStatement *ls = stmt->letStatement;
    char *name_str = identifierToString(ls->name);
    char *val_str = expressionToString(ls->value);

    size_t len = strlen("let ") + strlen(name_str) + 3 + strlen(val_str) + 2;
    char *out = malloc(len);
    snprintf(out, len, "let %s = %s;", name_str, val_str);
    free(name_str);
    free(val_str);
    return out;

  } else if (strcmp(stmt->type, NODE_EXPRESSION_STATEMENT) == 0) {
    return expressionToString(stmt->expressionStatement->expression);
  } else if (strcmp(stmt->type, NODE_RETURN_STATEMENT) == 0) {
    ReturnStatement *rs = stmt->returnStatement;
    char *val_str = expressionToString(rs->return_value);
    size_t len = strlen("return ") + strlen(val_str) + 2;
    char *out = malloc(len);
    snprintf(out, len, "return %s;", val_str);
    free(val_str);
    return out;
  }

  return strdup("<stmt unknown>");
}

char *programToString(Program *program) {
  size_t capacity = 1024;
  char *out = malloc(capacity);
  out[0] = '\0';

  for (int i = 0; i < program->statementCount; i++) {
    char *stmt_str = statementToString(program->statements[i]);
    size_t new_len = strlen(out) + strlen(stmt_str) + 2;

    if (new_len >= capacity) {
      capacity *= 2;
      out = realloc(out, capacity);
    }
    strcat(out, stmt_str);
    free(stmt_str);
  }

  return out;
}

// ===== BLOCK STATEMENT =====
BlockStatement *newBlockStatement(Token token, Statement **stmts, int count) {
  BlockStatement *b = malloc(sizeof(BlockStatement));
  b->token = token;
  b->statements = stmts;
  b->count = count;
  return b;
}

Statement *wrapBlockStatement(BlockStatement *b) {
  Statement *s = malloc(sizeof(Statement));
  s->type = NODE_BLOCK_STATEMENT;
  s->blockStatement = b;
  return s;
}

char *blockStatementToString(BlockStatement *b) {
  size_t size = 2;
  char *out = malloc(size);
  out[0] = '\0';

  for (int i = 0; i < b->count; i++) {
    char *stmtStr = statementToString(b->statements[i]);
    size += strlen(stmtStr);
    out = realloc(out, size);
    strcat(out, stmtStr);
    free(stmtStr);
  }

  return out;
}

void freeBlockStatement(BlockStatement *b) {
  for (int i = 0; i < b->count; i++) {
    freeStatement(b->statements[i]);
  }
  free(b->statements);
  free(b);
}

// ===== IF EXPRESSION =====
IfExpression *newIfExpression(Token token, Expression *cond,
                              BlockStatement *cons, BlockStatement *alt) {
  IfExpression *ifExpr = malloc(sizeof(IfExpression));
  ifExpr->token = token;
  ifExpr->condition = cond;
  ifExpr->consequence = cons;
  ifExpr->alternative = alt;
  return ifExpr;
}

Expression *wrapIfExpression(IfExpression *ifExpr) {
  Expression *e = malloc(sizeof(Expression));
  e->type = NODE_IF_EXPRESSION;
  e->ifExpression = ifExpr;
  return e;
}

char *ifExpressionToString(IfExpression *ie) {
  char *cond = expressionToString(ie->condition);
  char *cons = blockStatementToString(ie->consequence);
  char *alt = ie->alternative ? blockStatementToString(ie->alternative) : NULL;

  size_t size = strlen("if") + strlen(cond) + strlen(cons) + 16;
  if (alt)
    size += strlen("else") + strlen(alt);
  char *out = malloc(size);
  strcpy(out, "if");
  strcat(out, cond);
  strcat(out, cons);
  if (alt) {
    strcat(out, "else");
    strcat(out, alt);
    free(alt);
  }

  free(cond);
  free(cons);
  return out;
}

void freeIfExpression(IfExpression *ie) {
  freeExpression(ie->condition);
  freeBlockStatement(ie->consequence);
  if (ie->alternative)
    freeBlockStatement(ie->alternative);
  free(ie);
}

FunctionLiteral *newFunctionLiteral(Token token, Identifier **params, int count,
                                    BlockStatement *body) {
  FunctionLiteral *f = malloc(sizeof(FunctionLiteral));
  f->token = token;
  f->parameters = params;
  f->param_count = count;
  f->body = body;
  return f;
}

Expression *wrapFunctionLiteral(FunctionLiteral *f) {
  Expression *e = malloc(sizeof(Expression));
  e->type = NODE_FUNCTION_LITERAL;
  e->functionLiteral = f;
  return e;
}

char *functionLiteralToString(FunctionLiteral *fl) {
  size_t size = 64;
  char *out = malloc(size);
  strcpy(out, fl->token.literal);
  strcat(out, "(");

  for (int i = 0; i < fl->param_count; i++) {
    char *paramStr = identifierToString(fl->parameters[i]);
    size += strlen(paramStr) + 2;
    out = realloc(out, size);
    strcat(out, paramStr);
    if (i < fl->param_count - 1)
      strcat(out, ", ");
    free(paramStr);
  }

  strcat(out, ")");
  char *bodyStr = blockStatementToString(fl->body);
  size += strlen(bodyStr);
  out = realloc(out, size);
  strcat(out, bodyStr);
  free(bodyStr);
  return out;
}

void freeFunctionLiteral(FunctionLiteral *fl) {
  for (int i = 0; i < fl->param_count; i++) {
    free(fl->parameters[i]);
  }
  free(fl->parameters);
  freeBlockStatement(fl->body);
  free(fl);
}

// ===== CALL EXPRESSION =====
CallExpression *newCallExpression(Token token, Expression *func,
                                  Expression **args, int count) {
  CallExpression *c = malloc(sizeof(CallExpression));
  c->token = token;
  c->function = func;
  c->arguments = args;
  c->arg_count = count;
  return c;
}

Expression *wrapCallExpression(CallExpression *c) {
  Expression *e = malloc(sizeof(Expression));
  e->type = NODE_CALL_EXPRESSION;
  e->callExpression = c;
  return e;
}

char *callExpressionToString(CallExpression *ce) {
  char *funcStr = expressionToString(ce->function);

  size_t bufsize = strlen(funcStr) + 64;
  char *out = malloc(bufsize);
  strcpy(out, funcStr);
  strcat(out, "(");
  free(funcStr);

  for (int i = 0; i < ce->arg_count; i++) {
    char *argStr = expressionToString(ce->arguments[i]);

    size_t newlen = strlen(out) + strlen(argStr) + 4;
    if (newlen >= bufsize) {
      bufsize = newlen * 2;
      out = realloc(out, bufsize);
    }

    strcat(out, argStr);
    if (i < ce->arg_count - 1)
      strcat(out, ", ");
    free(argStr);
  }

  strcat(out, ")");
  return out;
}

void freeCallExpression(CallExpression *ce) {
  freeExpression(ce->function);
  for (int i = 0; i < ce->arg_count; i++) {
    freeExpression(ce->arguments[i]);
  }
  free(ce->arguments);
  free(ce);
}

// ===== STRING LITERAL =====
StringLiteral *newStringLiteral(Token token, const char *val) {
  StringLiteral *s = malloc(sizeof(StringLiteral));
  s->token = token;
  s->value = strdup(val);
  return s;
}

Expression *wrapStringLiteral(StringLiteral *s) {
  Expression *e = malloc(sizeof(Expression));
  e->type = NODE_STRING_LITERAL;
  e->stringLiteral = s;
  return e;
}

char *stringLiteralToString(StringLiteral *s) { return strdup(s->value); }

void freeStringLiteral(StringLiteral *s) {
  free(s->value);
  free(s);
}

// ===== ARRAY LITERAL =====
ArrayLiteral *newArrayLiteral(Token token, Expression **elements, int count) {
  ArrayLiteral *a = malloc(sizeof(ArrayLiteral));
  a->token = token;
  a->elements = elements;
  a->count = count;
  return a;
}

Expression *wrapArrayLiteral(ArrayLiteral *a) {
  Expression *e = malloc(sizeof(Expression));
  e->type = NODE_ARRAY_LITERAL;
  e->arrayLiteral = a;
  return e;
}

char *arrayLiteralToString(ArrayLiteral *al) {
  size_t size = 3;
  char *out = malloc(size);
  strcpy(out, "[");

  for (int i = 0; i < al->count; i++) {
    char *elem = expressionToString(al->elements[i]);
    size += strlen(elem) + 2;
    out = realloc(out, size);
    strcat(out, elem);
    if (i < al->count - 1)
      strcat(out, ", ");
    free(elem);
  }

  strcat(out, "]");
  return out;
}

void freeArrayLiteral(ArrayLiteral *al) {
  for (int i = 0; i < al->count; i++) {
    freeExpression(al->elements[i]);
  }
  free(al->elements);
  free(al);
}

IndexExpression *newIndexExpression(Token token, Expression *left,
                                    Expression *index) {
  IndexExpression *ie = malloc(sizeof(IndexExpression));
  ie->token = token;
  ie->left = left;
  ie->index = index;
  return ie;
}

Expression *wrapIndexExpression(IndexExpression *ie) {
  Expression *e = malloc(sizeof(Expression));
  e->type = NODE_INDEX_EXPRESSION;
  e->indexExpression = ie;
  return e;
}

char *indexExpressionToString(IndexExpression *ie) {
  char *leftStr = expressionToString(ie->left);
  char *indexStr = expressionToString(ie->index);
  size_t size = strlen(leftStr) + strlen(indexStr) + 4;

  char *out = malloc(size);
  snprintf(out, size, "(%s[%s])", leftStr, indexStr);

  free(leftStr);
  free(indexStr);
  return out;
}

void freeIndexExpression(IndexExpression *ie) {
  freeExpression(ie->left);
  freeExpression(ie->index);
  free(ie);
}

void freeExpression(Expression *expr) {
  if (!expr)
    return;

  if (strcmp(expr->type, NODE_IDENTIFIER) == 0) {
    free(expr->identifier->value);
    free(expr->identifier);
  } else if (strcmp(expr->type, NODE_INTEGER_LITERAL) == 0) {
    free(expr->integerLiteral);
  } else if (strcmp(expr->type, NODE_PREFIX_EXPRESSION) == 0) {
    free(expr->prefixExpression->op);
    freeExpression(expr->prefixExpression->right);
    free(expr->prefixExpression);
  } else if (strcmp(expr->type, NODE_INFIX_EXPRESSION) == 0) {
    freeExpression(expr->infixExpression->left);
    free(expr->infixExpression->op);
    freeExpression(expr->infixExpression->right);
    free(expr->infixExpression);
  } else if (strcmp(expr->type, NODE_HASH_LITERAL) == 0) {
    for (int i = 0; i < expr->hashLiteral->count; i++) {
      freeExpression(expr->hashLiteral->keys[i]);
      freeExpression(expr->hashLiteral->values[i]);
    }
    free(expr->hashLiteral->keys);
    free(expr->hashLiteral->values);
    free(expr->hashLiteral);
  }

  free(expr);
}

void freeStatement(Statement *stmt) {
  if (!stmt)
    return;

  if (strcmp(stmt->type, NODE_LET_STATEMENT) == 0) {
    free(stmt->letStatement->name->value);
    free(stmt->letStatement->name);
    freeExpression(stmt->letStatement->value);
    free(stmt->letStatement);
  } else if (strcmp(stmt->type, NODE_EXPRESSION_STATEMENT) == 0) {
    freeExpression(stmt->expressionStatement->expression);
    free(stmt->expressionStatement);
  } else if (strcmp(stmt->type, NODE_RETURN_STATEMENT) == 0) {
    freeExpression(stmt->returnStatement->return_value);
    free(stmt->returnStatement);
  }

  free(stmt);
}

void freeProgram(Program *program) {
  for (int i = 0; i < program->statementCount; i++) {
    freeStatement(program->statements[i]);
  }
  free(program->statements);
  free(program);
}
