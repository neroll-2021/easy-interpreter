# Easy Interpreter
[简体中文](./README_ZH.md)

an easy tree based interpreter for *nscript* in C++.

## nscript
*nscript* is a simple programming language with a syntax that is similar to C and Java.

It has only three basic types: int, float and boolean.

*nscript* support condition statement (if else) and loop statement (for, while) and function call.

You can use `input` and `println` to input or output data.

### Example
use *nscript* to compute the n-th Fibonacci number.
```
function fib(int x) : int {
    int a = 1;
    int b = 1;
    int c = 1;
    int i;
    for (i = 2; i < x; i = i + 1) {
        c = a + b;
        a = b;
        b = c;
    }
    return c;
}

int n = input(int);

println(fib(n));
```

### variable declaration
Variable declaration is similar to C and Java, the difference is that an uninitialized variable will have a default value. (int and float defaults to 0, boolean defaults to false)
```
int zero;
int i = 10;
float f = 5.2e-3;
```

### function
You can define a function like this
```
function add(int x, int y) : int {
    return x + y;
}
```
`function` is a keyword that indicates a function definition. Before the left brace is return type, which cannot be missing.

A function must return a value (cannot return a function), if you don't need a return value, you can simply return 0.

### loop
You can use `for` and `while` loop statement.
```
int sum = 0;
int i;
for (i = 1; i < 100 || i == 100; i = i + 1) {
    sum = sum + i;
}
println(sum);
```
It is very similar to C and Java.
Notes:
* loop index must define outside the loop statement.
* there is no 'less or equal' operator (<=), so as 'greater or equal'
* there is no self-increment operator, so as self-decrement operator

### if
```
function is_even(int x) : boolean {
    if (x % 2 == 0)
        return true;
    else {
        return false;
    }
}

println(is_even(input(int)));
```
It is similar to C and Java. If there is only one statement in the body of `if`, then the brace can be missing.

## Build
This project use [xmake](https://xmake.io/) to build.

Enter the project directory in the terminal, and then type following command:
```
xmake
```
Then the project will be built.

To run the program, just type
```
xmake run
```
then the program will read file 'script/script.txt' and execute code in it.

## How it works?
### Lexer
First lexer will read the file. It read character stream and output token stream. It record the type and location of every token.

### Parser
Then parser reads the token stream, it analyzes the token stream and builds a tree according to BNF notation. 

It also checks whether a variable or a function is defined, whether the funciton arguments match the definition of function, whether the operand is valid and so on.

Now the structure of program have been built up.

### Execute
To execute code, just traverse the tree and execute every statement.

## BNF Notation
the syntax of *nscript* refers a lot to C Programming Language. click [here](https://www.quut.com/c/ANSI-C-grammar-y.html) to get BNF notation of C.

You can find the BNF of *nscript* in 'include/script/detail/syntax_checker.h'. Most Left Recursion has been rewritten in Right Recursion to avoid infinite recursion.

## How Some Problems are Solved
The BNF of *nscript* is Right Recursive because Left Recursion will create infinity recursion.
To avoid infinity recursion, most Left Recursion has been rewritten in Right Recursion.

However, Right Recursion will create a right-leaning tree, which makes operators right associative. I use iteration instead of recursion to build syntax tree, so the tree would not be right-leaning.
