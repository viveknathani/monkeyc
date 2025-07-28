#include "object.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// === Environment Implementation ===

#define INITIAL_ENV_CAPACITY 8

Environment *newEnvironment() {
  Environment *env = malloc(sizeof(Environment));
  env->outer = NULL;
  env->store = malloc(sizeof(EnvironmentTableEntry) * INITIAL_ENV_CAPACITY);
  env->storeCount = 0;
  env->storeCapacity = INITIAL_ENV_CAPACITY;
  return env;
}

Environment *newEnclosedEnvironment(Environment *outer) {
  Environment *env = newEnvironment();
  env->outer = outer;
  return env;
}

Object *getFromEnvironment(Environment *env, char *name) {
  for (int i = 0; i < env->storeCount; i++) {
    if (strcmp(env->store[i].key, name) == 0) {
      return &env->store[i].value;
    }
  }
  if (env->outer) {
    return getFromEnvironment(env->outer, name);
  }
  return NULL;
}

Object *setInEnvironment(Environment *env, char *name, Object value) {
  for (int i = 0; i < env->storeCount; i++) {
    if (strcmp(env->store[i].key, name) == 0) {
      env->store[i].value = value;
      return &env->store[i].value;
    }
  }
  if (env->storeCount >= env->storeCapacity) {
    env->storeCapacity *= 2;
    env->store =
        realloc(env->store, sizeof(EnvironmentTableEntry) * env->storeCapacity);
  }
  env->store[env->storeCount].key = strdup(name);
  env->store[env->storeCount].value = value;
  env->storeCount++;
  return &env->store[env->storeCount - 1].value;
}

// === Object inspection ===

char *inspect(Object *obj) {
  char buf[512];
  if (strcmp(obj->type, IntegerObj) == 0) {
    snprintf(buf, sizeof(buf), "%lld", (long long)obj->integer->value);
  } else if (strcmp(obj->type, BooleanObj) == 0) {
    snprintf(buf, sizeof(buf), "%s", obj->boolean->value ? "true" : "false");
  } else if (strcmp(obj->type, NullObj) == 0) {
    snprintf(buf, sizeof(buf), "null");
  } else if (strcmp(obj->type, StringObj) == 0) {
    snprintf(buf, sizeof(buf), "%s", obj->string->value);
  } else if (strcmp(obj->type, ReturnValueObj) == 0) {
    return inspect(&obj->returnValue->value);
  } else if (strcmp(obj->type, ErrorObj) == 0) {
    snprintf(buf, sizeof(buf), "ERROR: %s", obj->error->message);
  } else {
    snprintf(buf, sizeof(buf), "<unknown>");
  }
  return strdup(buf);
}

Object *newError(char *msg) {
  Object *err = malloc(sizeof(Object));
  err->type = ErrorObj;
  err->error = malloc(sizeof(Error));
  err->error->message = strdup(msg);
  return err;
}

#define PUSH_EDGES                                                             \
  if (args[0]->type != ArrayObj)                                               \
  return newError("argument to `%s` not ARRAY; got %s", builtin_puts->name)

Object *builtin_len(Object **args, int argCount) {
  if (argCount != 1)
    return newError("wrong number of arguments.want=1");
  if (strcmp(args[0]->type, StringObj) == 0) {
    Object *rv = malloc(sizeof(Object));
    rv->type = IntegerObj;
    rv->integer = malloc(sizeof(Integer));
    rv->integer->value = strlen(args[0]->string->value);
    return rv;
  }
  if (strcmp(args[0]->type, ArrayObj) == 0) {
    Object *rv = malloc(sizeof(Object));
    rv->type = IntegerObj;
    rv->integer = malloc(sizeof(Integer));
    rv->integer->value = args[0]->array->count;
    return rv;
  }
  return newError("argument to `len` not supported");
}

Object *builtin_first(Object **args, int argCount) {
  if (argCount != 1 || strcmp(args[0]->type, ArrayObj) != 0) {
    return newError("wrong arguments to `first`");
  }
  if (args[0]->array->count > 0) {
    return &args[0]->array->elements[0];
  }
  Object *nullObj = malloc(sizeof(Object));
  nullObj->type = NullObj;
  nullObj->null = malloc(sizeof(Null));
  return nullObj;
}

Object *builtin_last(Object **args, int argCount) {
  if (argCount != 1 || strcmp(args[0]->type, ArrayObj) != 0) {
    return newError("wrong arguments to `last`");
  }
  int cnt = args[0]->array->count;
  if (cnt > 0) {
    return &args[0]->array->elements[cnt - 1];
  }
  Object *nullObj = malloc(sizeof(Object));
  nullObj->type = NullObj;
  nullObj->null = malloc(sizeof(Null));
  return nullObj;
}

Object *builtin_rest(Object **args, int argCount) {
  if (argCount != 1 || strcmp(args[0]->type, ArrayObj) != 0) {
    return newError("wrong arguments to `rest`");
  }
  int cnt = args[0]->array->count;
  if (cnt <= 1) {
    Object *nullObj = malloc(sizeof(Object));
    nullObj->type = NullObj;
    nullObj->null = malloc(sizeof(Null));
    return nullObj;
  }
  Object *newArr = malloc(sizeof(Object));
  newArr->type = ArrayObj;
  newArr->array = malloc(sizeof(Array));
  newArr->array->count = cnt - 1;
  newArr->array->elements = malloc(sizeof(Object) * (cnt - 1));
  memcpy(newArr->array->elements, &args[0]->array->elements[1],
         sizeof(Object) * (cnt - 1));
  return newArr;
}

Object *builtin_push(Object **args, int argCount) {
  if (argCount != 2 || strcmp(args[0]->type, ArrayObj) != 0) {
    return newError("wrong arguments to `push`");
  }
  int cnt = args[0]->array->count;
  Object *newArr = malloc(sizeof(Object));
  newArr->type = ArrayObj;
  newArr->array = malloc(sizeof(Array));
  newArr->array->count = cnt + 1;
  newArr->array->elements = malloc(sizeof(Object) * (cnt + 1));
  memcpy(newArr->array->elements, &args[0]->array->elements[0],
         sizeof(Object) * cnt);
  newArr->array->elements[cnt] = *args[1];
  return newArr;
}

Object *builtin_puts(Object **args, int argCount) {
  for (int i = 0; i < argCount; i++) {
    char *s = inspect(args[i]);
    printf("%s\n", s);
    free(s);
  }
  Object *nullObj = malloc(sizeof(Object));
  nullObj->type = NullObj;
  nullObj->null = malloc(sizeof(Null));
  return nullObj;
}

BuiltinEntry builtins[] = {
    {BuiltinFuncNameLen, &(Builtin){.function = builtin_len}},
    {BuiltinFuncNameFirst, &(Builtin){.function = builtin_first}},
    {BuiltinFuncNameLast, &(Builtin){.function = builtin_last}},
    {BuiltinFuncNameRest, &(Builtin){.function = builtin_rest}},
    {BuiltinFuncNamePush, &(Builtin){.function = builtin_push}},
    {BuiltinFuncNamePuts, &(Builtin){.function = builtin_puts}},
};

const int builtinsCount = sizeof(builtins) / sizeof(BuiltinEntry);

Builtin *getBuiltinByName(char *name) {
  for (int i = 0; i < builtinsCount; i++) {
    if (strcmp(builtins[i].name, name) == 0) {
      return builtins[i].function;
    }
  }
  return NULL;
}

uint64_t fnv1aHash(const char *str) {
  uint64_t hash = 14695981039346656037ULL;
  while (*str) {
    hash ^= (unsigned char)(*str++);
    hash *= 1099511628211ULL;
  }
  return hash;
}

HashKey getHashKey(Object *object) {
  HashKey key;
  key.type = object->type;

  if (strcmp(object->type, IntegerObj) == 0) {
    key.value = (uint64_t)object->integer->value;
  } else if (strcmp(object->type, BooleanObj) == 0) {
    key.value = object->boolean->value ? 1 : 0;
  } else if (strcmp(object->type, StringObj) == 0) {
    key.value = fnv1aHash(object->string->value);
  } else {
    fprintf(stderr, "Unhashable type: %s\n", object->type);
    exit(1);
  }

  return key;
}
