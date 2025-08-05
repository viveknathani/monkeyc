# monkeyc

Compiler for the Monkey programming language, written in C, based on the books [Writing an Interpreter in Go](https://interpreterbook.com/) and [Writing an Compiler in Go](https://compilerbook.com/) by Thorsten Ball.

## motivation and differences

I want to teach myself compilers. I figure this might be a nice way to start. It differs from the original Go implementation in the following ways:
1. This implementation does not support closures.
2. It does not have a garbage collector.
3. We compile the monkey source code into a self-contained executable!

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
