Main goal: The goal of this programming language is to provide an alternative to C that is "simple but powerful"

Specific goals:
  - Easy to parse (no/few ambiguities, negliable lookahead, consistent syntax) in order to help developers and
    tools understand and reason about the code

  - Provide a powerfull toolset (metaprogramming and language constructs) with minimal use of keywords, obscure
    syntax and other abominations

  - The language should "feel" low-level

  - Provide powerful metaprogramming that is well defined

Problems: (P: problem, I: important information, S: solution, ?: possible solution, # previous solutiuon)<br>
  P: How should importing work in the language, and should cyclic imports be allowed?<br>
  #: Cyclic imports will be allowed for packages, as importing a package will only declare a dependancy on that file.
     Cyclic loads of files is not allowed. Importing a directory will import the file in the directort of the same name.
     Loading a directory is also possible, as that will import all source files in that directory and all sub directories.
     All import paths are relative to the current file unless a specific mounting point is specified. Mounting
     points are defined in the build options, and are path snippets used as prefixes to any import path with
     the accompanying mounting point label as a prefix.<br>
  #: Imports are split in two: loading files and importing packages. Files are regular text files, while packages are
     files which have a package declaration on the first line, ignoring whitespace. Only files may be loaded and only
     packages may be imported. Loading works very similar to textual importing, and results in the loaded files' namespace
     being embedded within the loader's namespace. Importing will only create a link between the namespaces. Cyclic loads are
     therefore prohibited, however cyclic imports are perfectly legal. Trying to load a directory results in an error, whilst
     importing a directory will search the diretory for a file with the same name as the directory.<br>
  #: A program consists of several modules with a single entry point. A module is a bundle of source files under the same
     namespace and with the same default link name prefix. Modules can load source files, adding them to the module, and
     import other modules, creating a reference to the other module and import foreign libraries, c style shared libraries.
     Duplicate and cyclic loads are illegal. Duplicate imports are deduplicated and cyclic imports are perfectly legal. The
     path to the file to be loaded, or the module to be imported, may be prefixed with a path prefix, in the form
     "path_prefix:path/to/file/or/module". This is used for absolute paths. All paths are assumed to be relative to the
     current file, if no path prefix is specified.<br>
  #: A program consists of several modules with a single entry point. A module is a collection of source files within the same
     directory, under the same namespace. A module has a public and private namespace and can import other modules' public
     namespace. Importing is done via import statements, which specifies a path to the target module's directory. Importing is
     not recursive, which means that importing the superdirectory of a modules' directory does not import said module.
     Additionally, the importee of a module does not import the modules imported by said module, unless these imports are
     exported by the module. Cyclic- and duplicate imports are perfectly legal, as imports are only a declaration of some
     "handle" to another namespace.

  P: How should procedure overloading work?<br>
  #: Overloading should be implicit, since the programmer "headspace complexity" over explicit overloading is negligible and
     should be more familiar to programmers coming from other language<br>
  I: Implicit overloading may lead to a lot of pipeline stalling, since references to overloads which are
	 not a "perfect" (perfect meaning no implicit casts) may need reevaluation later in compilation, which prevents
	 every procedure with one or more non-perfect reference from leaving type checking, leading to a lot of potential
	 stalls<br>
  #: Explicit overloads seem to be a must<br>
  I: Explicit overloads still have the problem with pipeline stalling, and have proven to be harder to use, since most
     languages use implicit overloads (making explicit overloads feel unnatural to anyone migrating from almost any
     other language), and a lot of functionality is harder to implement with explicit overloading, when shadowing is
	 not allowed.<br>
  S: Implicit overloading


  P: In what order should global variables and constants be initialized?<br>
  ?: Globals and constants are initialized after their dependencies, respecting source order when possible,
     alphabetical otherwise

  P: What could the language provide to handle name collisions?<br>
  ?: Namespacing imports and file vs. export scope<br>
  #: Import statements which are able to selectively import declarations and namespace them<br>
  #: Imports that namespace and file vs export scope<br>
  S: Imports that namespace and public vs. private module scopes, linking names prefixed with package name by default

  P: How should attributes to procedures, structs and other constructs be marked up?<br>
  ?: By compiler directives before the declaration<br>
  #: @attrtibute_name(args) or @[attribute_name_0(args), attribute_name_1(args), ...] for multiple, before the declaration<br>
  #: @attribute_name(args), multiple attributes are separated by whitespace, can be placed before and after(before terminator) any declaration<br>
  S: @attribute_name(args), multiple attributes are separated by whitespace, can be placed before any* statement or expression

  P: Should macros be added to the language?<br>
  ?: yes? Similar to Jai macros

  P: Should the subscript operator work on pointers?<br>
  #: yes<br>
  ?: no? Maybe use intervals instead of pointers for this use case and implement pointer arithmetic as simple integer
     arithmetic, i.e. no divide by size of the pointed type<br>
  #: No, and pointer arithemtic is treated as integer arithmetic<br>
  S: yes, pointer arithmetic is treated as integer arithmetic, but pointer subscripting works in the same style as C

  P: How should unicode strings and characters be represented in memory, or rather,
     how should UTF-8 strings and characters be expressed and manipulated?<br>
  S: Strings are utf-8 encoded arrays of bytes, characters are utf-8 but always 4 bytes large.

  P: How should float to int conversions be handled, where the floating point number is larger than the largest
     integer type representable on a system?<br>
  ?: However LLVM does it?

  P: What should determine linking names, and should they be consistent?<br>
  ?: Name of the procedure followed by the type name of each argument, unless specified otherwise<br>
  #: "package_name.mangled_procedure_name" if not overridden<br>
  S: "module_name.mangled_procedure_name" if not overridden

  P: Should there be a distinction between static arrays and intervals?<br>
  I: Vector types: V2, V3, V4 :: [2]f32, [3]f32, [4]f32;<br>
  S: Yes, because this allows static arrays to exist and be used without storing the pointer or length of the data
     on the stack and in structs.

  P: How should declarations within a procedure be handled?<br>
  I: Either move every declaration out of the procedure body, or handle them inplace.<br>
     A mix of both will only result in confusion

  P: Struct, and maybe array, literal syntax<br>
  #: {:type: field0, field1} for structs, {:[X]: elem0, elem1, elem2} for arrays, where X is either an integer or ? (infer size)<br>
  ?: {:type: field0, field1} for structs, [:type: elem0, elem1, elem2] for arrays<br>
  ?: type{field0, field1} for structs, type[elem0, elem1, elem2] for arrays<br>
  ?: type{field0, field1} for structs, [X]type[elem0, elem1, elem2] for arrays, where X is either an integer or ? (infer size)
  S: {type: field0, field1, field_name = field2} for structs, [type: elem0, elem1, index0..index1 = elem2, index0..<index1 = elem3] for arrays

  P: Should there be types for 8- and 16-bit floats?<br>

  P: Should constant ifs be a directive or statement?<br>
  I: Constant ifs need "transparent" scopes, support only a condition and may appear in global scope<br>
  #: Constant ifs should be a directive
  S: constant ifs are rebranded as when statements

Ideas (-: idea, x: scratched idea):
  - Use arrays for SIMD like operations and support element-wise arithmetic

  - When the allocator pointer on a dynamic array is null, the array exists but cannot be freed
  
  x Provide a syntactical way of cheking if a value is included in a set (i.e. avoid token.kind == ... || token.kind == ... || token.kind == ...)<br>
	It may be possible to use array literals for this, but only if the compiler can "unroll" them.
  - allow for operator overloading that works on the ast. This allows x in [a, b, c] to be conditionally unrolled to x == a || x == b || x == c
  
  x Allow use of the blank identifier to explicitly specify that a type should be inferred. e.g.
    x, y, z, w : int, _, float, _ = 0, 0, 0, 0;

  - Provide Zig-esque optimization options: debug, release_safe, release_fast, release_small

  - "dynamic bake" directive that "bakes" arguments into a procedure call that are not constant, so as long as they are
	in scope, these arguments will be implicitly passed to the procedures. Essentially a syntactic closure

  - a beefier version of the attribute which can also be used by itself as an expression or declaration

Builtin types:
  - int               // A maximally sized signed integer
  - uint              // A 64-bit unsigned integer
  - float             // A 64-bit IEEE-754 floating point value
  - bool              // An 8-bit value that can either be false (0) or true (any value that is not 0)

  - i8, i16, i32, i64 // Explicitly sized signed integer
  - u8, u16, u32, u64 // Explicitly sized unsigned integer
  - f32, f64          // Explicitly sized IEEE-754 float

  - rawptr            // A 64/32-bit (depending on architecture) sized integer pointing to a specific address in memory
  - typeid            // A 32-bit value representing a specific type
  - any               // A typeid and a rawptr to the underlying data

Casting rules:
  - distinct types do not cast
  - any pointer type can be explicitly casted to any other pointer type and can be implicitly casted to and from rawptr
  - "typeid" can be explicitly casted to and from any integer and float type
  - "any" can be explicitly casted to any pointer type and any type can be implicitly casted to "any"
  - any specific base type implicitly casts to an unspecific type of the the same category<br>
    i.e. i8, i16, i32, i64 implicitly cast to int, u8..64 cast to uint, f32 and f64 cast to float
  - any integer and float type can be explicitly casted to any other integer or float type
  - any base type can be explicitly casted to bool and implicitly in certain contexts
  - bool can be explicitly casted to any integer, float and pointer type


Keywords:
  - return
  - defer
  - using
  - if
  - when
  - else
  - break
  - continue
  - for
  - while
  - proc
  - where
  - struct
  - union
  - enum
  - true
  - false
  - import
  - foreign
  - package

Control structures:
 - if    // if init; condition
 - when  // when init; condition
 - else
 - for   // for symbols in symbol
 - while // while init; condition; step

Declarations:
  - package declaration
  - import declaration
  - variable declaration
  - constant declaration

Statements:
  - scope
  - declaration
  - expression
  - if
  - else
  - for
  - while
  - break
  - continue
  - defer
  - return
  - using
  - assignment

Expressions:
  - literals
  - operations
  - types