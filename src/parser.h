struct Parser_State
{
    Comp_State* comp_state;
    Lexer lexer;
};

// top scope
// - imported scope list
// - decls
//   - ast node or scope

// scope
// - imported scopes
// - decls
// - ast nodes


// What is allowed at top scope?
// * declarations
// * const if

// Undeclared identifiers must be tolerated in the type checking stage and only logged, such that the same piece 
// of code may be rechecked once ample information is provided

// Const if
// The scope of a const if disappears when it has been evaluated

^Name is ambiguous
How to resolve polymorphic lambdas


NAME : TYPE : VALUE
NAME : TYPE = VALUE

PTR^ = VAR
PTR  = ^VAR

cast(TYPE) EXPR

// Behaviour
proc { ... }
proc(...) -> ... { ... }

// Data
struct { ... }
struct(...) { ... }