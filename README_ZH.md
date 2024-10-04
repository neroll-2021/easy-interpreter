# Easy Interpreter

一个基于语法树的简单脚本语言解释器，使用 C++23 实现

## nscript
*nscript* 是本项目设计出来的简单语言，语法和 C 与 Java 类似，只有三个基本类型：int, float, boolean

它支持条件判断语句（if, else）和循环语句（for, while），还支持函数调用。

可以使用 `input` 和 `println` 函数输入输出

### 例子
使用 *nscript* 计算第 n 项斐波那契数
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

### 变量声明
变量声明和 C 语言 与 Java 类似，一个区别是未初始化的变量会有默认值，int 和 float 默认为 0，boolean 默认为 false
```
int zero;
int i = 10;
float f = 5.2e-3;
```

### 函数
像这样定义一个函数
```
function add(int x, int y) : int {
    return x + y;
}
```
`function` 是用来定义函数的关键字，冒号和花括号前的类型名表示返回值类型，不能省略

函数必须返回一个值（不能返回函数），如果不需要返回值，return 0即可。

### 循环
可以使用 for 循环和 while 循环
```
int sum = 0;
int i;
for (i = 1; i < 100 || i == 100; i = i + 1) {
    sum = sum + i;
}
println(sum);
```
和 C 语言与 Java 非常相似。

注意：
* 循环变量必须在循环语句外定义
* 没有小于等于号（<=），也没有大于等于号（>=）
* 没有自增运算符（++），也没有自减运算符（--）

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
和 C 语言与 Java 类似。如果 if else 里只有一条语句，可以省略花括号

## 构建
本项目使用 [xmake](https://xmake.io/) 构建。

在命令行中进入当前项目的目录，输入以下指令并回车
```
xmake
```
xmake 会自动构建项目。

要运行项目，只需要输入以下命令并回车
```
xmake run
```
程序会读取 'script/script.txt' 中的代码并执行

# 工作流程
### 词法解析器（Lexer）
词法解析器读取文件。它读入字符流，输出词法单元（token）流，记录每个词法单元的类型和在源码中的位置（方便提示更具体的报错信息）

### 语法解析器（Parser）
语法解析器读入词法单元流，分析词法单元的结果并根据 BNF 范式构建语法树。

在这个过程中会检查符号是否定义、类型是否匹配，以及执行其他的一些静态检查。

语法解析器输出一个语法树。

### 执行
递归遍历语法树执行每一条语句。

## BNF 范式
*nscript* 的语法很大程度参考了 C 语言。[点击这里](https://www.quut.com/c/ANSI-C-grammar-y.html) 获取 C 语言的 BNF 范式。

## 一些问题的解决方式
*nscript* 的 BNF 范式是右递归的，因为本项目使用 LL(2) 语法解析器，而左递归会导致语法解析器无限递归。

为了避免无限递归，大部分左递归都被改写成了右递归。

然而，右递归会导致构建出来的语法树向右“倾斜”，从而导致运算符具有右结合性。因此在语法分析阶段不得不使用循环代替递归来构建语法树，使得语法树保持运算符原有的结合性。
