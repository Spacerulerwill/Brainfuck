# Brainfuck
An interpreter for the esoteric programming language Brainfuck written in C

# Building
I haven't bothered with any makefiles or cmake files as this project is a single C file. So compile it with whatever your favourite compiler is. As long as you use **C99 or greater.** For example, with gcc:
```c
gcc brainfuck.c -o brainfuck -std=c99 // Must be greater than C99
```  

# Usage
```c
./brainfuck path/to/file tapesize
``` 
