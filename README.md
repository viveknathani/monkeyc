# monkeyc

Compiler for the Monkey programming language, written in C, based on the books [Writing an Interpreter in Go](https://interpreterbook.com/) and [Writing an Compiler in Go](https://compilerbook.com/) by Thorsten Ball.

## motivation and differences

I want to teach myself compilers. I figure this might be a nice way to start. It differs from the original Go implementation in the following ways:
1. This implementation does not support closures.
2. This has a garbage collector. The original implementation does not need one because it is written in Go.

## building

You need the following tools installed:
- clang
- make

Then, run:

```
make
```

## license

[MIT](./LICENSE)
