Main goal: The goal of this programming language is to provide an alternative to C that is "simple but powerful"

Lesser goals:
  - Adopt a syntax similar to Jai in case of the language dying and Jai taking over as the language of choice
    for my personal projects

  - Easy to parse (no/few ambiguities, negliable lookahead, consistent syntax) in order to help developers and
    tools understand and reason about the code

  - Provide a powerfull toolset (metaprogramming and language constructs) with minimal use of keywords, obscure 
    syntax or other abominations

  - Invoke the "feeling of low-level programming"

  - Try to push metaprogramming "to a whole new level"

  - Provide support for visual code editors (e.g. compile AST instead of text)

Philosophy:
  - Only features which _cannot_ be implemented by a library should be a language construct

  - Every operation (e.g. integer addition) should either be well defined or throw an error

  - Build systems are _not_ and should _never_ be necessary to compile larger projects

  - There should only be _one_ implementation of a _standard_ library

  - Being able to easily read and reason about a piece of code is more important than being able to write code 
    quickly

Problems: (P: problem, S: solution, ?: possible solution)
  P: How should importing work in the language, and should cyclic imports be allowed?
  ?: Cyclic imports will be allowed, as importing a file will only declare a dependancy on that file. Importing a 
     directory is also possible, as that will import all source files in that directory and all sub directories.
     All import paths are relative to the current file unless a specific mounting point is specified. Mounting
     points are defined in the build options, and are path snippets used as prefixes to any import path with
     the accompanying mounting point label as a prefix.

  P: How should procedure overloading work?
  S: Overloading should be explicit, since the programmer complexity over explicit overloading is negligible and
     should be more familiar to programmers coming from other languages

  P: In what order should global variables and constants be initialized?
  S: Globals and constants are initialized after their dependencies, respecting source order when possible, 
     alphabetical otherwise

  P: What could the language provide to handle name collisions?
  ?: Namespacing imports and file vs. export scope
  S: Import statements which are able to selectively import declarations and namespace them

  P: How should attributes to procedures, structs and other constructs be marked up?
  ?: By compiler directives before the declaration
  S: @attrtibute_name(args) or @[attribute_name_0(args), attribute_name_1(args), ...] for multiple

  P: Should macros be added to the language?

  P: Should the subscript operator work on pointers?

  P: How far should metaprogramming be taken in this language?
  ?: Jai-esque polymorphism, run directives, body_text, modify and mutable compilation

  P: How should unicode strings and characters be represented in memory, or rather,
     how should UTF-8 strings and characters be expressed and manipulated?

  P: How should float to int conversions be handled, where the floating point number is larger than the largest 
     integer type representable on a system?
  ?: However LLVM does it?

  P: What should determine linking names, and should they be consistent?
  ?: Name of the procedure followed by the type name of each argument, unless specified otherwise
  S: "package_name.mangled_procedure_name" if not overridden

Ideas:
  - Comments cause a lot of problems when outdated, is there a better way to "comment" code than the current 
    standard of leaving text that is ignored by the compiler? Should the compiler try to solve this? Should it 
    pass the comments to a metaprogram that can "compile and check" the comments, or should this problem be left 
    for text editors to solve?

  - Use arrays for SIMD like operations and support element-wise arithmetic
    (e.g. #assert([2]int{1, 3} == [2]int{0, 1} + [2]int{1, 2}))

  - When the allocator pointer on a dynamic array is null, the array exists but cannot be freed

Observations from using C to build the compiler:
  - Default function arguments and function overloading are well worth their mental overhead. They allow for 
    hiding details which are unimportant in the current context, and they allow for greater flexibility when used 
    in tandem with a well defined common API (e.g. switching storage data structures from a hash map to a bucket 
    array des not require the usage code to change if both containers overload a common set of API functions).

  - Operator overloading is overrated, a procedure call is much clearer

Keywords:
  - return
  - defer
  - using
  - if
  - else
  - break
  - continue
  - for
  - proc
  - struct
  - union
  - enum
  - true
  - false

Builtin types:
  - int        // A 64-bit signed integer
  - uint       // A 64-bit unsigned integer
  - bool       // An 8-bit value that can either be false (0) or true (any value that is not 0)
  - float      // A 64-bit IEEE-754 floating point value

  - I8, I16, I32, I64 // Explicitly sized signed integer
  - U8, U16, U32, U64 // Explicitly sized unsigned integer
  - B8, B16, B32, B64 // Explicitly sized boolean
  - F32               // 32-bit IEEE-754 float
  - F64               // 64-bit IEEE-754 float

  - rawptr // A word sized integer pointing to a specific address in memory
  - typeid // A 32-bit value representing a specific type
  - any    // A typeid and a rawptr to the underlying data

Casting rules:
  - Aliased types can be both implicitly and explicitly converted to the base type or another alias of said type
  - Distinct types cannot be casted
  - All pointers can be explicitly converted to any pointer type
  - rawptr can be explicitly, and implicitly, converted to any pointer type
  - Fixed width integer, bool and floating point types can be converted to int, bool and float respectively, both 
    explicitly and implicitly
  - Signed integer types can be explicitly casted to unsigned, and vice versa
  - Any integer type can be explicitly casted to floating point, and vice versa
  - Any boolean type can be explicitly casted to any integer type, and vice versa
  - Any boolean type can be explicitly casted to any floating point type, and vice versa
  - typeid can be explicitly casted to any integer type, and vice versa
  - any can be casted to typeid and any pointer type, both explicitly and implicitly

Casting behaviour:
  - The resulting value of an integer casted to a smaller width will be equal to the N least significant bits of 
    the integer, where N is the width of the target type
//  - The result of a floating point value casted to an integer will be equal to the N least significant bits of 
//    the truncated value of the floating point value
  - The result of an integer value casted to floating point will be equal to the integer value represented as a
    IEEE-754 binary floating point value

Builtin functions:
  - sizeof
  - alignof
  - offsetof
  - typeof
  - assert

Compiler directives:
// only global
  - scope_export
  - scope_file

// statement level
  - if
  - bounds_check
  - no_bounds_check
  - overflow_check
  - no_overflow_check
  - assert

// expression level
  - run
  - type
  - codepoint

Attributes:
// Proc
  - inline
  - no_inline
  - foreign
  - no_discard
  - deprecated
  - distinct

Control structures:
 - if
 - else
 - for

Allowed at global scope:
  - import
  - load
  - scope_export, scope_file and if directives
  - declarations

Declarations:
  - procedure declaration
  - struct declaration
  - union declaration
  - enum declaration
  - variable declaration
  - constant declaration

Statements:
  - declaration
  - if
  - else
  - for
  - break
  - continue
  - defer
  - return
  - using
  - assignment
  - block
  - expression

Expressions:
  - literals
  - operations (add, sub, call, deref)
  - types

TODO:
 - provide implicit procedure overloading
 - provide a terser casting syntax
 - remove *fix procedure calling
 - separate the concept of procedure, struct and enum declarations from constants
 - remove nil (it causes too much confusion, checking for 0 is a lot clearer)
 - revert back to C style pointer syntax
 - #run directives should not be allowed in global scope
 - Add ternary

// for loops
for (i := 0; i < 10; i += 1) { ... }

for (i, j := 0, 1; i < 10; i, j += 1) { ... }

i := 0;
for (i < 10; i += 1) { ... }
for (i < 10) { ... }

for (i := 0; i < 10; i += 1)
{
    loop_j: for (true)
    {
        for (k := 0; k < 10; k += 1)
        {
            if (k == 3 && j == 5) continue loop_j;
            else if (i == 2 && j == 5) break loop_j;
            else                       break;
        }
    }
}

label_name: (for | if | else | {})

// struct- and other literals

// <already_existing_type> {args}
// type is not necessary when inside another type literal
int{2};
[?]int{1, 2, 3, 4, 5};
[3]int{0, 1, 2};
[]int{pointer, length};
[..]int{pointer, length, capacity}

// illegal: struct { x: int, y: int} {1, 2};
// type does not already "exist"

Struct :: struct { x: int, S: s };
S :: struct { y: int, z: int };

Struct{1, {2, 3}};
// illegal: {1, {2, 3}};
Struct{1, S{2 ,3}};
// illegal: {1, S{2, 3}};



Libraries draft

import         - imports a package under a different namespace
foreign import - imports a foreign lirbary
load           - loads a source file or directory adds it to the loader's namespace

import "file.os" as name;

load "file.os";
load "dir";

foreign import "ucrt.lib";
foreign import "ucrt.lib" as URCT;

File types
  - name.os - Otus source file

A package file begins with a package declaration and can contain source code, load files and import other packages

Importing a directory will import a file with the same name within that directory
Loading a directory will load all files within that directory

Example package
// math.os
package Math;

load "vector.os";
load "algebra";

import "SuperMathPackage" as SMP;

Cos :: proc(n : float) -> float { ... }
Sin :: proc(n : float) -> float { ... }
Tan :: proc(n : float) -> float { ... }

// Main program
import "math.os";

main :: proc
{
    Math.Cos(50.0);

    using Math;
    Cos(50.0);

    cosine :: Cos;
    cosine(50.0);
}