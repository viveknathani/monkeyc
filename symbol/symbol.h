#ifndef SYMBOL_H
#define SYMBOL_H

typedef char *SymbolScope;

#define GlobalScope "Global"
#define LocalScope "Local"
#define BuiltinScope "Builtin"
#define FreeScope "Free"

typedef struct {
  char *name;
  SymbolScope scope;
  int index;
} Symbol;

typedef struct {
  char *key;
  Symbol value;
} SymbolStoreEntry;

typedef struct SymbolTable SymbolTable;

struct SymbolTable {
  SymbolTable *outer;

  SymbolStoreEntry *store;
  int storeCount;
  int storeCapacity;

  int numDefinitions;

  Symbol *freeSymbols;
  int freeSymbolCount;
  int freeSymbolCapacity;
};

SymbolTable *newSymbolTable();
SymbolTable *newEnclosedSymbolTable(SymbolTable *outer);
Symbol define(SymbolTable *symbolTable, char *name);
Symbol defineBuiltin(SymbolTable *symbolTable, char *name, int index);
int resolve(SymbolTable *symbolTable, char *name, Symbol *out);
Symbol defineFree(SymbolTable *symbolTable, Symbol original);
void freeSymbolTable(SymbolTable *table);

#endif
