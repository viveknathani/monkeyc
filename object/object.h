#ifndef OBJECT_H
#define OBJECT_H

#include "../ast/ast.h"
#include "../opcode/opcode.h"
#include <stdbool.h>
#include <stdint.h>

#define IntegerObj "Integer"
#define BooleanObj "Boolean"
#define NullObj "Null"
#define ReturnValueObj "ReturnValue"
#define ErrorObj "Error"
#define FunctionObj "Function"
#define StringObj "String"
#define BuiltinObj "Builtin"
#define ArrayObj "Array"
#define HashObj "Hash"
#define CompiledFunctionObj "CompiledFunction"
#define BuiltinFuncNameLen "len"
#define BuiltinFuncNameFirst "first"
#define BuiltinFuncNameLast "last"
#define BuiltinFuncNameRest "rest"
#define BuiltinFuncNamePush "push"
#define BuiltinFuncNamePuts "puts"

typedef char *ObjectType;
typedef struct Object Object;
typedef struct Integer Integer;
typedef struct Boolean Boolean;
typedef struct Null Null;
typedef struct ReturnValue ReturnValue;
typedef struct Error Error;
typedef struct Function Function;
typedef struct String String;
typedef struct Builtin Builtin;
typedef struct Array Array;
typedef struct Hash Hash;
typedef struct HashKey HashKey;
typedef struct CompiledFunction CompiledFunction;
typedef struct Environment Environment;
typedef struct EnvironmentTableEntry EnvironmentTableEntry;
typedef struct BuiltinEntry BuiltinEntry;
typedef Object *(*BuiltinFunction)(Object **args, int argCount);

struct Object {
  ObjectType type;
  union {
    Integer *integer;
    Boolean *boolean;
    Null *null;
    ReturnValue *returnValue;
    Error *error;
    Function *function;
    String *string;
    Builtin *builtin;
    Array *array;
    Hash *hash;
    CompiledFunction *compiledFunction;
  };
};

typedef struct {
  Object key;
  Object value;
} HashPair;

struct HashKey {
  ObjectType type;
  uint64_t value;
};

typedef struct {
  HashKey key;
  HashPair hashPair;
} HashTableEntry;

struct Hash {
  // an array at the moment, to be replaced by an actual hash table
  // implementation at the end
  HashTableEntry *pairs;
};

struct Integer {
  int64_t value;
};

struct String {
  char *value;
};

struct Boolean {
  bool value;
};

struct Null {};

struct ReturnValue {
  Object value;
};

struct Error {
  char *message;
};

struct Function {
  Identifier *parameters;
  BlockStatement *body;
  Environment *environment;
};

struct Array {
  Object *elements;
  int count;
};

struct CompiledFunction {
  Instructions instructions;
  int numLocals;
  int numParameters;
};

struct EnvironmentTableEntry {
  char *key;
  Object value;
};

struct Environment {
  Environment *outer;
  EnvironmentTableEntry *store;
  int storeCount;
  int storeCapacity;
};

struct Builtin {
  BuiltinFunction function;
};

struct BuiltinEntry {
  char *name;
  Builtin *function;
};

extern BuiltinEntry builtins[];

char *inspect(Object *object);
HashKey getHashKey(Object *object);
Environment *newEnviroment();
Environment *newEnclosedEnvironment(Environment *environment);
Object *getFromEnvrionment(Environment *environment, char *name);
Object *setFromEnvrionment(Environment *environment, char *name, Object value);
Object *newError(char *message);
Builtin *getBuiltinByName(char *name);

#endif
