Main goal: The goal of this programming language is to provide an alternative to C that is "simple but powerful"

Lesser goals:
  - Adopt a syntax similar to Jai in case of the language dying and Jai taking over as the language of choice
    for my personal projects

  - Easy to parse (no/few ambiguities, negliable lookahead, consistent syntax) in order to help developers and
    tools understand and reason about the code

  - Provide a powerfull toolset (metaprogramming and language constructs) with minimal use of keywords, obscure
    syntax and other abominations

  - Invoke the "feeling of low-level programming"

  - Try to push metaprogramming "to a whole new level"

  - Provide support for visual code editors (e.g. compile AST instead of text)

Philosophy:
  - Only features which _cannot_ be implemented properly by a library should be a language construct

  - Every operation (e.g. integer addition) should either be well defined or throw a compile time error

  - External build tools are _not_ and should _never_ be necessary to compile larger projects

  - There should only be _one_ implementation of a _standard_ library

  - Being able to easily read and reason about a piece of code is more important than being able to write code
    quickly

Problems: (P: problem, I: important information, S: solution, ?: possible solution, # previous solutiuon)<br>
  P: How should importing work in the language, and should cyclic imports be allowed?<br>
  #: Cyclic imports will be allowed for packages, as importing a package will only declare a dependancy on that file.
     Cyclic loads of files is not allowed. Importing a directory will import the file in the directort of the same name.
     Loading a directory is also possible, as that will import all source files in that directory and all sub directories.
     All import paths are relative to the current file unless a specific mounting point is specified. Mounting
     points are defined in the build options, and are path snippets used as prefixes to any import path with
     the accompanying mounting point label as a prefix.<br>
  S: Imports are split in two: loading files and importing packages. Files are regular text files, while packages are
     files which have a package declaration on the first line, ignoring whitespace. Only files may be loaded and only
     packages may be imported. Loading works very similar to textual importing, and results in the loaded files' namespace
     being embedded within the loader's namespace. Importing will only create a link between the namespaces. Cyclic loads are
     therefore prohibited, however cyclic imports are perfectly legal. Trying to load a directory results in an error, whilst
     importing a directory will search the diretory for a file with the same name as the directory.

  P: How should procedure overloading work?<br>
  ?: Overloading should be implicit, since the programmer "headspace complexity" over explicit overloading is negligible and
     should be more familiar to programmers coming from other language

  P: In what order should global variables and constants be initialized?<br>
  ?: Globals and constants are initialized after their dependencies, respecting source order when possible,
     alphabetical otherwise

  P: What could the language provide to handle name collisions?<br>
  ?: Namespacing imports and file vs. export scope<br>
  #: Import statements which are able to selectively import declarations and namespace them<br>
  S: Imports that namespace and file vs export scope

  P: How should attributes to procedures, structs and other constructs be marked up?<br>
  ?: By compiler directives before the declaration<br>
  S: @attrtibute_name(args) or @[attribute_name_0(args), attribute_name_1(args), ...] for multiple, before the declaration

  P: Should macros be added to the language?

  P: Should the subscript operator work on pointers?<br>
  #: yes<br>
  ?: no? Maybe use intervals instead of pointers for this use case and implement pointer arithmetic as simple integer
     subtraction, i.e. no divide by size of the pointed type

  P: How far should metaprogramming be taken in this language?<br>
  ?: Jai-esque polymorphism, run directives, body_text, modify and mutable compilation

  P: How should unicode strings and characters be represented in memory, or rather,
     how should UTF-8 strings and characters be expressed and manipulated?<br>
  S: Strings are utf-8 encoded arrays of bytes, characters are utf-8 but always 4 bytes large.

  P: How should float to int conversions be handled, where the floating point number is larger than the largest
     integer type representable on a system?<br>
  ?: However LLVM does it?

  P: What should determine linking names, and should they be consistent?<br>
  ?: Name of the procedure followed by the type name of each argument, unless specified otherwise<br>
  S: "package_name.mangled_procedure_name" if not overridden

  P: Should there be a distinction between static arrays and intervals?<br>
  I: Vector types: V2, V3, V4 :: [2]f32, [3]f32, [4]f32;<br>
  S: Yes, because this allows static arrays to exist and be used without storing the pointer or length of the data
     on the stack and in structs.

  P: How should declarations within a procedure be handled?<br>
  I: Either every declaration must be moved out of the procedure body, or they should all remain.<br>
     Something in between will only result in confusion

Ideas:
  - Use arrays for SIMD like operations and support element-wise arithmetic
    (e.g. #assert({:[2]int: 1, 3} == {:[2]int: 0, 1} + {:[2]int: 1, 2}))

  - When the allocator pointer on a dynamic array is null, the array exists but cannot be freed
  
  - Provide a syntactical way of cheking if a value is included in a set (i.e. avoid token.kind == ... || token.kind == ... || token.kind == ...)
  
  - Should scope local closures be a part of the language as a syntactical convenience (but _NO_ capture, only elide syntactic pass by value/ref)
  
  - Turn the "for" loop into a "for each" loop and beef up the "while" loop to take the old "for" loops place
  
  - Move "inline" from an attribute to a directive or keyword, and add the option to inline a function at the call site and inline certain for loops

  - Allow use of the blank identifier to explicitly specify that a type should be inferred. e.g.
    x, y, z, w : int, _, float, _ = 0, 0, 0, 0;

  - Static arrays "only exist as data", length is stored in the type and the pointer to the data is stored in the symbol
    table

  - Provide Zig-esque optimization options: debug, release_safe, release_fast, release_small

  - Provide a way of explicitly restricting return values in "safe" mode (mainly for providing a an api contract for
    possible error return values)

Observations from using C to build the compiler:
  - Default function arguments and function overloading are well worth their mental overhead. They allow for
    hiding details which are unimportant in the current context, and they allow for greater flexibility when used
    in tandem with a well defined common API (e.g. switching storage data structures from a hash map to a bucket
    array does not require the usage code to change if both containers overload a common set of API functions).

  - Operator overloading is overrated, a procedure call is much clearer

Observations from talk on Zig (https://www.youtube.com/watch?v=Gv2I7qTux7g), (+: posetive, -: negative)
  + No distinction between constant if and regular if
  + Control over whether a value is known at compile time
  + The error handling system
  + Every standard library function that allocates memory needs to be supplied with an allocator
  + Per scope build mode
TODO: There should be some negatives here ^

Observations from other language:
  - The dot enum thingy (infer the type name from .MemberName) is actually usefull
  - The Odin where statement for structs and procedures seems like a better syntax than #modify

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
  - as
  - import
  - load
  - foreign

Builtin types:
  - int        // A 64-bit signed integer
  - uint       // A 64-bit unsigned integer
  - bool       // An 8-bit value that can either be false (0) or true (any value that is not 0)
  - float      // A 64-bit IEEE-754 floating point value

  - i8, i16, i32, i64 // Explicitly sized signed integer
  - u8, u16, u32, u64 // Explicitly sized unsigned integer
  - b8, b16, b32, b64 // Explicitly sized boolean
  - f32, f64          // Explicitly sized IEEE-754 float

  - rawptr // A 64/32-bit (depending on architecture) sized integer pointing to a specific address in memory
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
  - type_info_of
  - assert

Compiler directives:
// only global
  - scope_export
  - scope_file

// statement level
  - if
  - bounds_check
  - no_bounds_check
  - assert

// expression level
  - run
  - distinct

Attributes:
// Proc
  - inline
  - no_inline
  - foreign
//  - no_discard
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

Import packages & load files
- Importing a package will __link__ the current and the imported packages' namespaces
- Duplicate imports are deduplicated
- Only package files can be imported
- A file name equal to the directory name will be appended to an import path that leads to a directory
<br>
- Loading a file will __merge__ the current and the loaded files' namespaces
- Duplicate loads throw an error
- Only non-package files can be loaded
  
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

                                               Scratchpad

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Old Metaprogramming API

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Workspace_Open :: proc(workspace: ^Workspace, options: Workspace_Options, file_path: String) -> bool ---;

Workspace_Close :: proc(workspace: Workspace) ---;

Workspace_InspectNextDeclaration :: proc(workspace: Workspace, declaration: ^Declaration) -> bool ---;

Workspace_ModifyCurrentDeclaration :: proc(workspace: Workspace, declaration: ^Declaration) -> bool ---;

Workspace_InjectDeclaration :: proc(workspace: Workspace, package: string, declaration: ^Declaration) -> bool ---;

Workspace_InjectImport :: proc(workspace: Workspace, package: string, path: string, alias: string) -> bool ---;

Workspace_GenerateBinary :: proc(workspace: Workspace, options: Binary_Options) -> bool ---;


// TODO:
  - Find out how types should be represented and manipulated in the metaprogram
  - Find out how the metaprogram should refer to declarations
  - Maybe ditch the direct declaration based API with a message loop instead?
  - Should the internal and API representation of the AST be memory compatible?
  - Should body_text and modify be a part of the language and how should they be handled?

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////


New Metaprogramming API

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

OpenWorkspace

CloseWorkspace

FetchNextDeclaration

ResubmitDeclaration

SubmitDeclaration

// CommitDeclaration -- implicit

NumDeclarationsLeft -- different name or function?

GenerateBinary


// TODO:
  - Find out how types should be represented and manipulated in the metaprogram. Build metaprogram local type table and use type infos
  - Should the internal and API representation of the AST be memory compatible? NO
  - Should body_text and modify be a part of the language and how should they be handled?

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

entities : []Declaration;

for (decl: ^Declaration; FetchNextDeclaration(workspace, &decl))
{
	if (decl.name == "main" && NumDeclarationsLeft(workspace) != 0) ResubmitDeclaration(workspace, decl);
	else
	{
		if (decl.kind == Declaration_Struct)
		{
			if (HasTag(decl, "entity"))
			{
				if (!HasTag(decl, "no_serialization")
				{
					serialization_proc := ParseCode(...); // create some sort of serialization procedure

					Submitdeclaration(workspace, serialization_proc);
				}

				append(&entities, decl);
			}
		}

		else if (decl.kind == Declaration_Proc)
		{
			// do something with types
		}

		if (NumDeclarationsLeft(workspace, "entity") == 0)
		{
			// do something with all the entities
		}
	}
}