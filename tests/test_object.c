#include "../object/object.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void testIntegerObject() {
  Integer value = {.value = 123};
  Object obj = {.type = IntegerObj, .integer = &value};

  char *buf = inspect(&obj);
  assert(strcmp(buf, "123") == 0);
}

void testBooleanObject() {
  Boolean valueTrue = {.value = true};
  Boolean valueFalse = {.value = false};
  Object objTrue = {.type = BooleanObj, .boolean = &valueTrue};
  Object objFalse = {.type = BooleanObj, .boolean = &valueFalse};

  char *buft = inspect(&objTrue);
  assert(strcmp(buft, "true") == 0);

  char *buff = inspect(&objFalse);
  assert(strcmp(buff, "false") == 0);
}

void testNullObject() {
  Null nullVal = {};
  Object obj = {.type = NullObj, .null = &nullVal};

  char *buf = inspect(&obj);
  assert(strcmp(buf, "null") == 0);
}

void testStringObject() {
  String value = {.value = "hello"};
  Object obj = {.type = StringObj, .string = &value};

  char *buf = inspect(&obj);
  assert(strcmp(buf, "hello") == 0);
}

void testErrorObject() {
  Error err = {.message = "Something went wrong"};
  Object obj = {.type = ErrorObj, .error = &err};

  char *buf = inspect(&obj);
  assert(strcmp(buf, "ERROR: Something went wrong") == 0);
}

void testHashKeyEquality() {
  Integer val1 = {.value = 999};
  Integer val2 = {.value = 999};
  Object obj1 = {.type = IntegerObj, .integer = &val1};
  Object obj2 = {.type = IntegerObj, .integer = &val2};

  HashKey key1 = getHashKey(&obj1);
  HashKey key2 = getHashKey(&obj2);

  assert(strcmp(key1.type, key2.type) == 0);
  assert(key1.value == key2.value);
}

void testStringHashKey() {
  String s1 = {.value = "foobar"};
  String s2 = {.value = "foobar"};
  Object o1 = {.type = StringObj, .string = &s1};
  Object o2 = {.type = StringObj, .string = &s2};

  HashKey k1 = getHashKey(&o1);
  HashKey k2 = getHashKey(&o2);

  assert(strcmp(k1.type, k2.type) == 0);
  assert(k1.value == k2.value);
}

int main() {
  testIntegerObject();
  testBooleanObject();
  testNullObject();
  testStringObject();
  testErrorObject();
  testHashKeyEquality();
  testStringHashKey();

  printf("All object tests passed!\n");
  return 0;
}
