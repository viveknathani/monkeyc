#include "object.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// === Environment Implementation ===
// environments use linear search for simplicity - good for small scopes

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

// lookup variable in environment chain (current -> outer)
Object *getFromEnvironment(Environment *env, char *name) {
  // search current scope first
  for (int i = 0; i < env->storeCount; i++) {
    if (strcmp(env->store[i].key, name) == 0) {
      return &env->store[i].value;
    }
  }
  // recurse to outer scope if not found
  if (env->outer) {
    return getFromEnvironment(env->outer, name);
  }
  return NULL;
}

// set variable in environment (updates existing or creates new)
Object *setInEnvironment(Environment *env, char *name, Object value) {
  // check if variable already exists - update in place
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
  } else if (strcmp(obj->type, ArrayObj) == 0) {
    // Build a simple comma-separated list of element representations.
    strcpy(buf, "[");
    for (int i = 0; i < obj->array->count; i++) {
      char *elemRepr = inspect(&obj->array->elements[i]);
      strncat(buf, elemRepr, sizeof(buf) - strlen(buf) - 1);
      free(elemRepr);
      if (i < obj->array->count - 1) {
        strncat(buf, ", ", sizeof(buf) - strlen(buf) - 1);
      }
    }
    strncat(buf, "]", sizeof(buf) - strlen(buf) - 1);
  } else if (strcmp(obj->type, HashObj) == 0) {
    // print hash size using new hash table implementation
    snprintf(buf, sizeof(buf), "<hash with %d entries>",
             obj->hash->size); // TODO: pretty-print contents if needed
  } else if (strcmp(obj->type, CompiledFunctionObj) == 0) {
    snprintf(buf, sizeof(buf), "<compiled fn at %p>",
             (void *)obj->compiledFunction);
  } else if (strcmp(obj->type, BuiltinObj) == 0) {
    snprintf(buf, sizeof(buf), "<builtin fn>");
  } else if (strcmp(obj->type, ReturnValueObj) == 0) {
    return inspect(&obj->returnValue->value);
  } else if (strcmp(obj->type, ErrorObj) == 0) {
    snprintf(buf, sizeof(buf), "ERROR: %s", obj->error->message);
  } else {
    snprintf(buf, sizeof(buf), "<unknown %s>", obj->type);
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

// builtin function registry - maps names to function pointers
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

// hash table implementation with O(1) average lookup
#define INITIAL_BUCKET_COUNT 16
#define LOAD_FACTOR_THRESHOLD 0.75

// create new hash table
Hash *newHash() {
  Hash *hash = malloc(sizeof(Hash));
  hash->bucketCount = INITIAL_BUCKET_COUNT;
  hash->size = 0;
  hash->capacity = (int)(INITIAL_BUCKET_COUNT * LOAD_FACTOR_THRESHOLD);
  hash->buckets = calloc(hash->bucketCount, sizeof(HashEntry*));
  return hash;
}

// free hash table and all entries
void freeHash(Hash *hash) {
  if (!hash) return;
  
  for (int i = 0; i < hash->bucketCount; i++) {
    HashEntry *entry = hash->buckets[i];
    while (entry) {
      HashEntry *next = entry->next;
      free(entry);
      entry = next;
    }
  }
  free(hash->buckets);
  free(hash);
}

// check if two keys are equal
int hashKeysEqual(Object *key1, Object *key2) {
  if (strcmp(key1->type, key2->type) != 0) {
    return 0;
  }
  
  if (strcmp(key1->type, IntegerObj) == 0) {
    return key1->integer->value == key2->integer->value;
  } else if (strcmp(key1->type, BooleanObj) == 0) {
    return key1->boolean->value == key2->boolean->value;
  } else if (strcmp(key1->type, StringObj) == 0) {
    return strcmp(key1->string->value, key2->string->value) == 0;
  }
  
  return 0;
}

// resize hash table when load factor exceeds threshold
void hashResize(Hash *hash) {
  int oldBucketCount = hash->bucketCount;
  HashEntry **oldBuckets = hash->buckets;
  
  // double the bucket count
  hash->bucketCount *= 2;
  hash->capacity = (int)(hash->bucketCount * LOAD_FACTOR_THRESHOLD);
  hash->buckets = calloc(hash->bucketCount, sizeof(HashEntry*));
  hash->size = 0;
  
  // rehash all existing entries
  for (int i = 0; i < oldBucketCount; i++) {
    HashEntry *entry = oldBuckets[i];
    while (entry) {
      HashEntry *next = entry->next;
      hashSet(hash, &entry->key, &entry->value);
      free(entry);
      entry = next;
    }
  }
  
  free(oldBuckets);
}

// set key-value pair in hash table
int hashSet(Hash *hash, Object *key, Object *value) {
  if (hash->size >= hash->capacity) {
    hashResize(hash);
  }
  
  HashKey hashKey = getHashKey(key);
  int bucketIndex = hashKey.value & (hash->bucketCount - 1);  // fast modulo for power of 2
  
  HashEntry *entry = hash->buckets[bucketIndex];
  
  // check if key already exists
  while (entry) {
    if (hashKeysEqual(&entry->key, key)) {
      entry->value = *value;  // update existing value
      return 0;
    }
    entry = entry->next;
  }
  
  // create new entry
  HashEntry *newEntry = malloc(sizeof(HashEntry));
  newEntry->key = *key;
  newEntry->value = *value;
  newEntry->next = hash->buckets[bucketIndex];
  hash->buckets[bucketIndex] = newEntry;
  hash->size++;
  
  return 0;
}

// get value from hash table
Object *hashGet(Hash *hash, Object *key) {
  HashKey hashKey = getHashKey(key);
  int bucketIndex = hashKey.value & (hash->bucketCount - 1);  // fast modulo for power of 2
  
  HashEntry *entry = hash->buckets[bucketIndex];
  
  while (entry) {
    if (hashKeysEqual(&entry->key, key)) {
      return &entry->value;
    }
    entry = entry->next;
  }
  
  return NULL;  // key not found
}

void printObject(Object *object) {
  if (object == NULL) {
    printf("[NULL] (null)\n");
    return;
  }
  printf("[%s] ", object->type);
  if (strcmp(object->type, IntegerObj) == 0) {
    printf("%lld\n", (long long)object->integer->value);
  } else if (strcmp(object->type, BooleanObj) == 0) {
    printf("%s\n", object->boolean->value ? "true" : "false");
  } else if (strcmp(object->type, NullObj) == 0) {
    printf("null\n");
  } else if (strcmp(object->type, StringObj) == 0) {
    printf("\"%s\"\n", object->string->value);
  } else if (strcmp(object->type, ArrayObj) == 0) {
    printf("array[%d]\n", object->array->count);
  } else if (strcmp(object->type, HashObj) == 0) {
    // print hash object pointer and size (no pairs field)
    printf("hash@%p (size=%d)\n", (void *)object->hash, object->hash->size); // TODO: pretty-print contents if needed
  } else if (strcmp(object->type, CompiledFunctionObj) == 0) {
    printf("compiled_fn@%p\n", (void *)object->compiledFunction);
  } else if (strcmp(object->type, BuiltinObj) == 0) {
    printf("<builtin>\n");
  } else if (strcmp(object->type, ReturnValueObj) == 0) {
    printf("(return) ");
    printObject(&object->returnValue->value);
  } else if (strcmp(object->type, ErrorObj) == 0) {
    printf("ERROR: %s\n", object->error->message);
  } else {
    char *repr = inspect(object);
    printf("%s\n", repr);
    free(repr);
  }
}
