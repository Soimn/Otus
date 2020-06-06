Main goal: The goal of this programming language is to provide an alternative to C that is simple and supports 
           modern metaprogramming features

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

  - Coding style should _not_ be dictated by a tool or language

Problems:
  - How should types be inferred?

  - How should unicode strings and characters be represented in memory, or rather,
    how should UTF-8 strings and characters be expressed and manipulated?

Solved problems:
  P: If Pascal style pointer notation is adopted, how should the parser differentiate between taking a pointer to
     a variable and a pointer type? Should they be equivalent?
  S: Whether an expression is evaluated as a type or as an ordinary expression is infered by context.
     The type evaluation context can be forced by using the #type directive.

  P: Should alleviating the semicolon after struct, enum and proc declarations be allowed as an 
     "exception to the rule"?
  S: After much thought it seemed like the benefit of keeping the syntax consistent, in this case, falls below the
     aesthetic value of not speewing semicolons everywhere.

Ideas:
  - Comments cause a lot of problems when outdated, is there a better way to "comment" code than the current 
    standard of leaving text that is ignored by the compiler? Should the compiler try to solve this? Should it 
    pass the comments to a metaprogram that can "compile and check" the comments, or should this problem be left 
    for text editors to solve?

  - Use arrays for SIMD like operations and support element-wise arithmetic
    (e.g. #assert([2]int{1, 3} == [2]int{0, 1} + [2]int{1, 2}))

  - Provide a way of specifying specific pointer sizes (e.g. force a pointer to be 64-bit on all platforms). Maybe 
    extend this to all core data types? I8 - I64 and U8 - U64 could defined in terms of this.

Observations from using C to build the compiler:
  - Default function arguments and function overloading are well worth their mental overhead. They allow for 
    hiding details which are unimportant in the current context, and they allow for greater flexibility when used 
    in tandem with a well defined common API (e.g. switching storage data structures from a hash map to a bucket 
    array des not require the usage code to change if both containers overload a common set of API functions).

Keywords:
  - return
  - defer
  - using
  - if
  - else
  - while
  - break
  - continue
  - for // Will be added in the future
  - proc
  - struct
  - enum
  - true
  - false
  - nil

Builtin types:
  - int        // A 64-bit signed integer
  - uint       // A 64-bit unsigned integer
  - bool       // An 8-bit value that can either be false (0) or true (any value that is not 0)
  - float      // A 64-bit IEEE-754 floating point value

  - I8, I16, I32, I64 // Explicitly sized signed integer
  - U8, U16, U32, U64 // Explicitly sized unsigned integer
  - F32               // 32-bit IEEE-754 float
  - F64               // 64-bit IEEE-754 float

  - byte   // An 8-bit value
  - word   // A 64-bit value for 64-bit systems, else 32-bit
  - rawptr // A word sized integer pointing to a specific address in memory
  - typeid // A 32-bit value representing a specific type
  - any    // A typeid and a rawptr to the underlying data

Casting rules:
  - Aliased types can be both implicitly and explicitly casted to eachother
  - Distinct types cannot be casted to eachother
  - All pointers can be explicitly casted to any pointer type
  - rawptr can be explicitly, and implicitly, casted to any pointer type
  - A more specific type can be casted to a less specific type, both explicitly and implicitly. e.g:
    * I8, I16, I32 and I64 can all be casted to int
    * U8, U16, U32 and U64 can all be casted to uint
    * F32 and F64 can be casted to float
  - Signed integer types can be explicitly casted to unsigned, and vice versa
  - Any integer type can be explicitly casted to floating point, and vice versa
  - byte can be explicitly casted to any integer type
  - word can be explicitly casted to any integer type
  - typeid can be explicitly casted to any integer type
  - any can be casted to typeid and any pointer type

Casting behaviour:
  - The resulting value of an integer casted to a smaller width will be equal to the N least significant bits of 
    the integer, where N is the width of the target type
  - The result of a floating point value casted to an integer will be equal to the floored value (if positive, 
    ceiled if negative) if the integer is large enough to represent the value, otherwise the result will be equal 
    to nil

Builtin functions:
  - cast
  - transmute
  - sizeof
  - alignof
  - offsetof
  - typeof
  - assert

Compiler directives:
  - bounds_check
  - no_bounds_check
  - inline
  - no_inline
  - type
  - distinct
  - foreign
  - run
  - if
  - char
  - size
  - bit_field
  - union
  - strict
  - loose
  - packed
  - padded
  - no_discard
  - deprecated
  - align
  - scope_export
  - scope_file


Important information:
A compilation unit consists of import-, variable- and constant declarations. 

Import declarations signal the parser to append a file for compilation and imports the exported entities of that file. Import declarations can only appear at global scope and will only import what is exported from the target file. The compiler directives: "scope_export" and "scope_file", control which entities are exported. These directives control the state of a boolean