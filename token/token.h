#ifndef TOKEN_H
#define TOKEN_H

typedef const char *TokenType;

// represents a lexical token with its type and literal value
// note: the literal field points to memory that should be managed by the caller
// this structure does not own the literal string memory
typedef struct {
  TokenType type;
  const char *literal;  // Made const to clarify immutability
} Token;

// Special tokens
#define ILLEGAL "ILLEGAL"
#define EOF_TOK "EOF"

// Literals
#define IDENTIFIER "IDENTIFIER"
#define INT "INT"
#define STRING "STRING"

// Operators
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

// Delimiters
#define COMMA ","
#define SEMICOLON ";"
#define COLON ":"
#define LPAREN "("
#define RPAREN ")"
#define LBRACE "{"
#define RBRACE "}"
#define LBRACKET "["
#define RBRACKET "]"

// Keywords
#define FUNCTION "FUNCTION"
#define LET "LET"
#define TRUE_TOK "TRUE"
#define FALSE_TOK "FALSE"
#define IF "IF"
#define ELSE "ELSE"
#define RETURN "RETURN"

// creates a new token with the given type and literal value
// type: the token type (should be one of the defined constants)
// literal: the literal string value (caller retains ownership)
// returns: a new Token structure
// note: this function does not allocate memory for the literal string
// the caller is responsible for ensuring the literal remains valid for the lifetime of the token
Token newToken(TokenType type, const char *literal);

// looks up an identifier to determine if it's a keyword
// identifier: the identifier string to look up (must not be NULL)
// returns: the corresponding keyword token type, or IDENTIFIER if not a keyword
TokenType lookupIdentifier(const char *identifier);

// creates a deep copy of a token with duplicated literal string
// original: the token to clone
// returns: a new token with copied literal (caller must free the literal)
Token cloneToken(Token original);

#endif
