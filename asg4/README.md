# Symbols and Type Checking

## Overview
Builds off of the previous assignment. A symbol table is constructed and a symbols are inserted into the table everytime a symbol is scanned. When a reference is scanned, the reference is looked up in the symbol table for its corresponding information. The custom language includes the following symbols that is scanned by the added files of this assignment:

- Type names (void, int, string, struct, etc.)
- Field names (arrow operators, structs, etc.)
- Function and variable names
- Type categories (arrays, pointers, etc.)
- Polymorphism (only universal parametric polymorphism in arrays and structs)

There is also a type checker that ivolves a post-order depth-first traversal of the abstract syntax tree built in the previous assignment. The type checker follows a partial context-sensitive type checking grammar that includes checking compatible types, correct usage of operators(assignments, field selectors, etc), call parameters, etc.

## Credits
Institution: University of California, Santa Cruz<br/>
Course: Fundamentals of Compiler Design I<br/>
Professor: Wesley, Mackey<br/>
Student(s): Wai Chun Leung, Shineng Tang