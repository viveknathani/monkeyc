#include "symbol.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_STORE_CAPACITY 8
#define INITIAL_FREE_CAPACITY 4

SymbolTable *newSymbolTable() {
  SymbolTable *table = malloc(sizeof(SymbolTable));
  table->outer = NULL;
  table->store = malloc(sizeof(SymbolStoreEntry) * INITIAL_STORE_CAPACITY);
  table->storeCount = 0;
  table->storeCapacity = INITIAL_STORE_CAPACITY;
  table->freeSymbols = malloc(sizeof(Symbol) * INITIAL_FREE_CAPACITY);
  table->freeSymbolCount = 0;
  table->freeSymbolCapacity = INITIAL_FREE_CAPACITY;
  table->numDefinitions = 0;
  return table;
}

SymbolTable *newEnclosedSymbolTable(SymbolTable *outer) {
  SymbolTable *table = newSymbolTable();
  table->outer = outer;
  return table;
}

Symbol define(SymbolTable *symbolTable, char *name) {
  Symbol symbol;
  symbol.name = strdup(name);
  symbol.scope = symbolTable->outer ? LocalScope : GlobalScope;
  symbol.index = symbolTable->numDefinitions++;

  if (symbolTable->storeCount >= symbolTable->storeCapacity) {
    symbolTable->storeCapacity *= 2;
    symbolTable->store =
        realloc(symbolTable->store,
                sizeof(SymbolStoreEntry) * symbolTable->storeCapacity);
  }

  symbolTable->store[symbolTable->storeCount].key = strdup(name);
  symbolTable->store[symbolTable->storeCount].value = symbol;
  symbolTable->storeCount++;

  return symbol;
}

Symbol defineBuiltin(SymbolTable *symbolTable, char *name, int index) {
  Symbol symbol;
  symbol.name = strdup(name);
  symbol.scope = BuiltinScope;
  symbol.index = index;

  if (symbolTable->storeCount >= symbolTable->storeCapacity) {
    symbolTable->storeCapacity *= 2;
    symbolTable->store =
        realloc(symbolTable->store,
                sizeof(SymbolStoreEntry) * symbolTable->storeCapacity);
  }

  symbolTable->store[symbolTable->storeCount].key = strdup(name);
  symbolTable->store[symbolTable->storeCount].value = symbol;
  symbolTable->storeCount++;

  return symbol;
}

int resolve(SymbolTable *symbolTable, char *name, Symbol *out) {
  for (int i = 0; i < symbolTable->storeCount; i++) {
    if (strcmp(symbolTable->store[i].key, name) == 0) {
      *out = symbolTable->store[i].value;
      return 0;
    }
  }
  if (symbolTable->outer != NULL) {
    // First check if the symbol exists directly in the immediate outer scope
    for (int i = 0; i < symbolTable->outer->storeCount; i++) {
      if (strcmp(symbolTable->outer->store[i].key, name) == 0) {
        Symbol directSymbol = symbolTable->outer->store[i].value;
        if (strcmp(directSymbol.scope, GlobalScope) == 0 ||
            strcmp(directSymbol.scope, BuiltinScope) == 0) {
          *out = directSymbol;
          return 0;
        }
      }
    }
    
    // If not found directly, resolve recursively
    Symbol outerSymbol;
    if (resolve(symbolTable->outer, name, &outerSymbol) == 0) {
      if (strcmp(outerSymbol.scope, BuiltinScope) == 0) {
        *out = outerSymbol;
        return 0;
      }
      // must promote to free
      Symbol freeSym = defineFree(symbolTable, outerSymbol);
      *out = freeSym;
      return 0;
    }
  }
  return -1; // not found
}

Symbol defineFree(SymbolTable *symbolTable, Symbol original) {
  Symbol sym;
  sym.name = strdup(original.name);
  sym.scope = FreeScope;
  sym.index = symbolTable->freeSymbolCount;

  if (symbolTable->freeSymbolCount >= symbolTable->freeSymbolCapacity) {
    symbolTable->freeSymbolCapacity *= 2;
    symbolTable->freeSymbols =
        realloc(symbolTable->freeSymbols,
                sizeof(Symbol) * symbolTable->freeSymbolCapacity);
  }

  Symbol originalCopy;
  originalCopy.name = strdup(original.name);
  originalCopy.scope = original.scope;
  originalCopy.index = original.index;
  symbolTable->freeSymbols[symbolTable->freeSymbolCount++] = originalCopy;

  return sym;
}

void freeSymbolTable(SymbolTable *table) {
  for (int i = 0; i < table->storeCount; i++) {
    free(table->store[i].key);
    free(table->store[i].value.name);
  }
  free(table->store);
  for (int i = 0; i < table->freeSymbolCount; i++) {
    free(table->freeSymbols[i].name);
  }
  free(table->freeSymbols);
  free(table);
}
