#include "../symbol/symbol.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void testDefineAndResolveGlobal() {
  SymbolTable *global = newSymbolTable();

  Symbol a = define(global, "a");
  assert(strcmp(a.name, "a") == 0 && strcmp(a.scope, GlobalScope) == 0 &&
         a.index == 0);

  Symbol b = define(global, "b");
  assert(strcmp(b.name, "b") == 0 && strcmp(b.scope, GlobalScope) == 0 &&
         b.index == 1);

  Symbol out;
  int ok = resolve(global, "a", &out);
  assert(ok == 0 && out.index == 0 && strcmp(out.scope, GlobalScope) == 0);

  ok = resolve(global, "b", &out);
  assert(ok == 0 && out.index == 1 && strcmp(out.scope, GlobalScope) == 0);

  freeSymbolTable(global);
  printf("✅ testDefineAndResolveGlobal passed\n");
}

void testNestedLocalScopes() {
  SymbolTable *global = newSymbolTable();
  define(global, "a");
  SymbolTable *local = newEnclosedSymbolTable(global);

  Symbol b = define(local, "b");
  assert(strcmp(b.name, "b") == 0 && strcmp(b.scope, LocalScope) == 0 &&
         b.index == 0);

  Symbol out;
  int ok = resolve(local, "a", &out);
  assert(ok == 0 && strcmp(out.scope, GlobalScope) == 0);

  ok = resolve(local, "b", &out);
  assert(ok == 0 && strcmp(out.scope, LocalScope) == 0);

  freeSymbolTable(local);
  freeSymbolTable(global);
  printf("✅ testNestedLocalScopes passed\n");
}

void testDefineBuiltinAndResolving() {
  SymbolTable *global = newSymbolTable();
  Symbol builtin = defineBuiltin(global, "len", 42);

  assert(strcmp(builtin.name, "len") == 0 &&
         strcmp(builtin.scope, BuiltinScope) == 0 && builtin.index == 42);

  Symbol out;
  int ok = resolve(global, "len", &out);
  assert(ok == 0 && strcmp(out.scope, BuiltinScope) == 0 && out.index == 42);

  freeSymbolTable(global);
  printf("✅ testDefineBuiltinAndResolving passed\n");
}

void testFreeSymbols() {
  SymbolTable *global = newSymbolTable();
  define(global, "a");
  define(global, "b");

  SymbolTable *first = newEnclosedSymbolTable(global);
  define(first, "c");

  SymbolTable *second = newEnclosedSymbolTable(first);
  define(second, "d");

  // resolve for free var from second scope
  Symbol out;
  int ok = resolve(second, "a", &out);
  assert(ok == 0 && strcmp(out.scope, FreeScope) == 0 && out.index == 0);

  ok = resolve(second, "b", &out);
  assert(ok == 0 && strcmp(out.scope, FreeScope) == 0 && out.index == 1);

  // freeSymbols count
  assert(second->freeSymbolCount == 2);

  freeSymbolTable(second);
  freeSymbolTable(first);
  freeSymbolTable(global);
  printf("✅ testFreeSymbols passed\n");
}

void testShadowing() {
  SymbolTable *global = newSymbolTable();
  define(global, "a");
  SymbolTable *local = newEnclosedSymbolTable(global);

  Symbol shadow = define(local, "a");
  assert(strcmp(shadow.scope, LocalScope) == 0 && shadow.index == 0);

  Symbol out;
  int ok = resolve(local, "a", &out);
  assert(ok == 0 && strcmp(out.scope, LocalScope) == 0 &&
         out.index == shadow.index);

  freeSymbolTable(local);
  freeSymbolTable(global);
  printf("✅ testShadowing passed\n");
}

int main() {
  testDefineAndResolveGlobal();
  testNestedLocalScopes();
  testDefineBuiltinAndResolving();
  testFreeSymbols();
  testShadowing();
  printf("\n✅ all symbol-table tests passed!\n");
  return 0;
}
