#ifndef TOKEN_H
#define TOKEN_H

typedef const char *TokenType;

typedef struct {
  TokenType type;
  char *literal;
} Token;

#define ILLEGAL "ILLEGAL"
#define EOF_TOK "EOF"

#define IDENTIFIER "IDENTIFIER"
#define INT "INT"

#define ASSIGN "="
#define PLUS "+"
#define MINUS "-"
#define BANG "!"
#define ASTERISK "*"
#define SLASH "/"

#define LT "<"
#define GT ">"

#define EQ "=="
#define NOT_EQ "!="

#define COMMA ","
#define SEMICOLON ";"
#define LPAREN "("
#define RPAREN ")"
#define LBRACE "{"
#define RBRACE "}"

#define FUNCTION "FUNCTION"
#define LET "LET"
#define TRUE_TOK "TRUE"
#define FALSE_TOK "FALSE"
#define IF "IF"
#define ELSE "ELSE"
#define RETURN "RETURN"

typedef struct {
  const char *keyword;
  TokenType type;
} KeywordMap;

static KeywordMap keywords[] = {
    {"fn", FUNCTION}, {"let", LET},   {"true", TRUE_TOK}, {"false", FALSE_TOK},
    {"if", IF},       {"else", ELSE}, {"return", RETURN},
};

#define KEYWORD_COUNT (sizeof(keywords) / sizeof(keywords[0]))

Token newToken(TokenType type, char *literal);

TokenType lookupIdentifier(const char *identifier);

#endif
