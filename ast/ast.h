#ifndef AST_H
#define AST_H

#include "../token/token.h"

#define NODE_PROGRAM "PROGRAM"
#define NODE_LET_STATEMENT "LET_STATEMENT"
#define NODE_RETURN_STATEMENT "RETURN_STATEMENT"
#define NODE_EXPRESSION_STATEMENT "EXPRESSION_STATEMENT"
#define NODE_BLOCK_STATEMENT "BLOCK_STATEMENT"
#define NODE_IDENTIFIER "IDENTIFIER"
#define NODE_INTEGER_LITERAL "INTEGER_LITERAL"
#define NODE_BOOLEAN "BOOLEAN"
#define NODE_PREFIX_EXPRESSION "PREFIX_EXPRESSION"
#define NODE_INFIX_EXPRESSION "INFIX_EXPRESSION"
#define NODE_IF_EXPRESSION "IF_EXPRESSION"
#define NODE_FUNCTION_LITERAL "FUNCTION_LITERAL"
#define NODE_CALL_EXPRESSION "CALL_EXPRESSION"
#define NODE_STRING_LITERAL "STRING_LITERAL"
#define NODE_ARRAY_LITERAL "ARRAY_LITERAL"
#define NODE_INDEX_EXPRESSION "INDEX_EXPRESSION"
#define NODE_HASH_LITERAL "HASH_LITERAL"

typedef struct Statement Statement;
typedef struct Expression Expression;
typedef struct LetStatement LetStatement;
typedef struct ReturnStatement ReturnStatement;
typedef struct ExpressionStatement ExpressionStatement;
typedef struct BlockStatement BlockStatement;
typedef struct Identifier Identifier;
typedef struct IntegerLiteral IntegerLiteral;
typedef struct BooleanLiteral BooleanLiteral;
typedef struct PrefixExpression PrefixExpression;
typedef struct InfixExpression InfixExpression;
typedef struct IfExpression IfExpression;
typedef struct FunctionLiteral FunctionLiteral;
typedef struct CallExpression CallExpression;
typedef struct StringLiteral StringLiteral;
typedef struct ArrayLiteral ArrayLiteral;
typedef struct IndexExpression IndexExpression;
typedef struct HashLiteral HashLiteral;

struct Statement {
  char *type;
  union {
    LetStatement *letStatement;
    ReturnStatement *returnStatement;
    ExpressionStatement *expressionStatement;
    BlockStatement *blockStatement;
  };
};

struct Expression {
  char *type;
  union {
    Identifier *identifier;
    IntegerLiteral *integerLiteral;
    BooleanLiteral *booleanLiteral;
    PrefixExpression *prefixExpression;
    InfixExpression *infixExpression;
    IfExpression *ifExpression;
    FunctionLiteral *functionLiteral;
    CallExpression *callExpression;
    StringLiteral *stringLiteral;
    ArrayLiteral *arrayLiteral;
    IndexExpression *indexExpression;
    HashLiteral *hashLiteral;
  };
};

typedef struct {
  char *type;
  Statement **statements;
  int statementCount;
} Program;

struct LetStatement {
  Token token;
  Identifier *name;
  Expression *value;
};

struct ReturnStatement {
  Token token;
  Expression *return_value;
};

struct ExpressionStatement {
  Token token;
  Expression *expression;
};

struct BlockStatement {
  Token token;
  Statement **statements;
  int count;
};

struct Identifier {
  Token token;
  char *value;
};

struct IntegerLiteral {
  Token token;
  long long value;
};

struct BooleanLiteral {
  Token token;
  int value;
};

struct PrefixExpression {
  Token token;
  char *op;
  Expression *right;
};

struct InfixExpression {
  Token token;
  Expression *left;
  char *op;
  Expression *right;
};

struct IfExpression {
  Token token;
  Expression *condition;
  BlockStatement *consequence;
  BlockStatement *alternative;
};

struct FunctionLiteral {
  Token token;
  Identifier **parameters;
  int param_count;
  BlockStatement *body;
};

struct CallExpression {
  Token token;
  Expression *function;
  Expression **arguments;
  int arg_count;
};

struct StringLiteral {
  Token token;
  char *value;
};

struct ArrayLiteral {
  Token token;
  Expression **elements;
  int count;
};

struct IndexExpression {
  Token token;
  Expression *left;
  Expression *index;
};

struct HashLiteral {
  Token token;
  Expression **keys;
  Expression **values;
  int count;
};
Token cloneToken(Token original);
LetStatement *newLetStatement(Token token, Identifier *name, Expression *value);
ReturnStatement *newReturnStatement(Token token, Expression *value);
ExpressionStatement *newExpressionStatement(Token token, Expression *expr);
BlockStatement *newBlockStatement(Token token, Statement **statements,
                                  int count);
IfExpression *newIfExpression(Token token, Expression *cond,
                              BlockStatement *cons, BlockStatement *alt);
FunctionLiteral *newFunctionLiteral(Token token, Identifier **params, int count,
                                    BlockStatement *body);
CallExpression *newCallExpression(Token token, Expression *func,
                                  Expression **args, int count);
StringLiteral *newStringLiteral(Token token, const char *val);
ArrayLiteral *newArrayLiteral(Token token, Expression **elements, int count);
IndexExpression *newIndexExpression(Token token, Expression *left,
                                    Expression *index);
HashLiteral *newHashLiteral(Token token, Expression **keys, Expression **values,
                            int count);
Identifier *newIdentifier(Token token, const char *value);
IntegerLiteral *newIntegerLiteral(Token token, long long value);
BooleanLiteral *newBooleanLiteral(Token token, int value);
PrefixExpression *newPrefixExpression(Token token, const char *op,
                                      Expression *right);
InfixExpression *newInfixExpression(Token token, Expression *left,
                                    const char *op, Expression *right);
Expression *wrapIdentifier(Identifier *id);
Expression *wrapIntegerLiteral(IntegerLiteral *il);
Expression *wrapBooleanLiteral(BooleanLiteral *bl);
Expression *wrapPrefixExpression(PrefixExpression *pe);
Expression *wrapInfixExpression(InfixExpression *ie);
Expression *wrapIfExpression(IfExpression *ifExpr);
Expression *wrapFunctionLiteral(FunctionLiteral *fl);
Expression *wrapCallExpression(CallExpression *ce);
Expression *wrapStringLiteral(StringLiteral *sl);
Expression *wrapArrayLiteral(ArrayLiteral *al);
Expression *wrapIndexExpression(IndexExpression *ie);
Expression *wrapHashLiteral(HashLiteral *hl);
Statement *wrapLetStatement(LetStatement *stmt);
Statement *wrapReturnStatement(ReturnStatement *stmt);
Statement *wrapExpressionStatement(ExpressionStatement *stmt);
Statement *wrapBlockStatement(BlockStatement *block);
char *identifierToString(Identifier *ident);
char *integerLiteralToString(IntegerLiteral *il);
char *booleanLiteralToString(BooleanLiteral *bl);
char *stringLiteralToString(StringLiteral *sl);
char *prefixExpressionToString(PrefixExpression *pe);
char *infixExpressionToString(InfixExpression *ie);
char *ifExpressionToString(IfExpression *ie);
char *functionLiteralToString(FunctionLiteral *fl);
char *callExpressionToString(CallExpression *ce);
char *arrayLiteralToString(ArrayLiteral *al);
char *indexExpressionToString(IndexExpression *ie);
char *hashLiteralToString(HashLiteral *hl);
char *blockStatementToString(BlockStatement *b);
char *expressionToString(Expression *expr);
char *statementToString(Statement *stmt);
char *programToString(Program *program);
void freeExpression(Expression *expr);
void freeStatement(Statement *stmt);
void freeProgram(Program *program);
void freeBlockStatement(BlockStatement *b);
void freeIfExpression(IfExpression *ie);
void freeFunctionLiteral(FunctionLiteral *fl);
void freeCallExpression(CallExpression *ce);
void freeStringLiteral(StringLiteral *s);
void freeArrayLiteral(ArrayLiteral *al);
void freeIndexExpression(IndexExpression *ie);

#endif
