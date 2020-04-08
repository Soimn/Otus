Main goal: The goal of this programming language is to provide an alternative to C that is simple and supports 
           modern metaprogramming features

Lesser goals:
  - Adopt a syntax similar to Jai in case of the language dying and Jai taking over as the language of choice
    for my personal projects

  - Easy to parse (no/few ambiguities, negliable lookahead, consistent syntax) in order to help developers and tools 
    understand and reason about the code

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
  - When should string literals be checked?

  - How should types be inferred?

  - How should unicode strings and characters be represented in memory, or rather,
    how should UTF-8 strings and characters be expressed and manipulated?

Solved problems:
  P: If Pascal style pointer notation is adopted, how should the parser differentiate between taking a pointer to a 
     variable and a pointer type? Should they be equivalent?
  S: Whether an expression is evaluated as a type or as an ordinary expression is infered by context.
     The type evaluation context can be forced by using the #type directive.

  P: Should alleviating the semicolon after struct, enum and proc declarations be allowed as an 
     "exception to the rule"?
  S: After much thought it seemed like the benefit of keeping the syntax consistent in this case falls below the
     aesthetic value of not speewing semicolons everywhere.

Ideas:
  - Comments cause a lot of problems when outdated, is there a better way to "comment" code than the current 
    standard of leaving text that is ignored by the compiler? Should the compiler try to solve this? Should it 
    pass the comments to a metaprogram that can "compile and check" the comments, or should this problem be left 
    for text editors to solve?

  - Use arrays for SIMD like operations and support element-wise arithmetic
    (e.g. #assert([2]{1, 3} == [2]int{0, 1} + [2]{1, 2}))

  - Provide of specifying specific pointer sizes (e.g. force a pointer to be 64-bit on all platforms). Maybe 
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
  - int
  - uint
  - bool
  - float
  - typeid
  - I8, I16, I32, I64
  - U8, U16, U32, U64
  - F32, F64

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
  - foreign_library
  - run
  - if
  - char
  - strict
  - loose
  - packed
  - padded
  - no_discard
  - deprecated
  - align
  - import
  - scope_export
  - scope_file

Grammar (BNF):

<identifier> ::= (([_]+[A-Za-z0-9])|[A-Za-z])[A-Za-z0-9]*
<number>     ::= (0[xh][A-Fa-f0-9]+)|([0-9]+(.[0-9]+)?)
<string>     ::= "((\\\\)+|(\\")|[^"])*"
<blank>      ::= "_"
<note>       ::= "@" <identifier> ["(" [<expression> {"," <expression>}] ")"]

<compilation_unit> ::= {<import_directive> | <statement>}

<compiler_directives> ::= <import_directive> | <scope_directives>
<import_directive> ::= "#" "import" <string> [<identifier> | <blank>]
<scope_directives> ::= ("#" "scope_export") | ("#" "scope_file")
<expression_level_directives> ::= ("#" "foreign_library" <identifier>) | ("#" "inline" <primary_expression>)            |
                                  ("#" "no_inline" <primary_expression>) | ("#" "no_bounds_check" <primary_expression>) |
                                  ("#" "bounds_check" <primary_expression>) | ("#" "char" <string>)                     |
                                  ("#" "type" <unary_expression>) | ("#" "run" <primary_expression>)                    |
                                  ("#" "distinct" <primary_expression>)

<const_if_directive> ::= "#" "if" <expression> <statement> ["else" <statement>]
<assert_directive>   ::= "#" "assert" <expression>
<statement_level_directives> ::= ("#" "no_bounds_check") | ("#" "bounds_check")
<procedure_directives> ::= ("#" "inline") | ("#" "no_inline") | ("#" "foreign" <identifier>) | ("#" "no_discard") | ("#" "deprecated" <string>)
<structure_directives> ::= ("#" "strict") | ("#" "loose") | ("#" "packed") | ("#" "padded") | ("#" "align" <expression>) |
                           ("#" "union")
<enumeration_directives> ::= ("#" "strict") | ("#" "loose")



<declaration>          ::= <variable_declaration> | <constant_declaration> | <using_declaration>
<variable_declaration> ::= (<identifier> | <blank>) {"," (<identifier> | <blank>)} ":" (([<type>] "=" (<expression> | ("-" "-" "-"))) | (<type>)) ";"
<constant_declaration> ::= <identifier> ":" [<type>] ":" (<procedure> | <structure> | <enumeration> |
                                                          (<expression> ";"))
<using_declaration>    ::= "using" (<variable_declaration> | <constant_declaration> | <member_expression>) ";"

<procedure>              ::= <proc_type> {<procedure_directives> | <statement_directives>} <statement_block>
<procedure_args>         ::= ["using"] <procedure_arg_name> {"," <procedure_arg_name>} ":" (([<type>] "=" <expression>) | (<type>))
<procedure_arg_name>     ::= ["$" | ("$" "$")] (<identifier> | <blank>)
<procedure_return_value> ::= <type> | "(" [<identifier> ":"] <type> {[<identifier> ":"] <type>} ")"
<structure>              ::= "struct" ["(" [<structre_arg> {"," structure_arg}] ")"] {<structure_directives>} "{" [<structure_member> {"," <structure_member>}]"}"
<structure_args>         ::= ["$" | ("$" "$")] <identifier> ":" <type>
<structure_member>       ::= ["using"] (<identifier> | <blank>) ":" <type>
<enumeration>            ::= "enum" ["(" [<type>] ")"] {<enumeration_directives>} "{" [<identifier> ["=" <expression>] {"," <identifier ["=" <expression>]>}"}"


<statement> ::= <statement_level_directive> <statement>                                           |
                <statement_block>                                                                 |
                <declaration> <note>                                                              |
                <assert_directive>                                                                |
                <const_if_directive>                                                              |
                "if" <expression> <statement> ["else" <statement>]                                |
                "while" <expression> <statement>                                                  |
                "break" ";"                                                                       |
                "continue" ";"                                                                    |
                "return" [<identifier> "="] <expression>{"," [<identifier> "="] <expression>} ";" |
                <assignment_statement>                                                            |
                <expression> ";"                                                                  |
                ";"

<statement_block> ::= "{" {<statement>} "}"
<assignment_statement> ::= <unary_expression> ["+" | "-" | "*" | "/" | "%" | "&&" | "||" | "&" | "|" | "<<" | ">>"]"=" <expression> ";"


<type> ::= ("^" | ("[" "]") | ("[" <expression> "]") | ("[" "." "." "]")) <type> |
           <proc_type>                                                           |
           <structure>                                                           |
           <enumeration>                                                         |
           "typeof" "(" <type> ")"                                               |
           ["$"] <identifier>

<proc_type> ::= "proc" ["(" [<procedure_args> {"," <procedure_arg>}] ")"] ["-" ">" <procedure_return_value>]


<expression>             ::= <logical_or_expression>
<logical_or_expression>  ::= <logical_and_expression> {"|" "|" <logical_and_expression>}
<logical_and_expression> ::= <comparative_expression> {"&" "&" <comparative_expression>}
<comparative_expression> ::= <add_expression> [(("=" | "!" | "<" | ">")"=" | "<" ">") <add_expression>]
<add_expression>         ::= <mult_expression> {("+" | "-" | "|") <mult_expression>}
<mult_expression>        ::= <infix_call_expression> {("*" | "/" | "%" | "&" | "<<" | ">>") <infix_call_expression>}
<infix_call_expression>  ::= <unary_expression> {"'" <identifier> [<call_expression>] ["'" <unary_expression>]}
<unary_expression>       ::= ("+" | "-" | "^" | (<identifier> [<call_expression>] "'")) <unary_expression> |
                             <member_expression>
<member_expression>      ::= <primary_expression> {"." <primary_expression>}
<primary_expression>     ::= <expression_level_directives> | (<identifier> <postfix_expression>) | <number> | (<string> <postfix_expression>) |
                             (<procedure> <postfix_expression>) | <type> ["{"[[<identifier> "="] <expression> {"," [<identifier> "="] <expression>}]"}"]
<postfix_expression>     ::= {("[" ([<expression>] ":" [<expression>]) | <expression> "]") | <call_expression>
<call_expression>        ::= "(" [[<identifier> "="] <expression> {"," [<identifier> "="] <expression>}] ")"