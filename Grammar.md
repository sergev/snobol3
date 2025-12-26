# Snobol III Grammar

This document describes the grammar of the Snobol III dialect implemented in this interpreter. The grammar is derived from the parser implementation in `sno2.c`, lexical analyzer in `sno1.c`, and execution semantics in `sno3.c` and `sno4.c`.

## Table of Contents

1. [Lexical Structure](#lexical-structure)
2. [Expressions](#expressions)
3. [Patterns](#patterns)
4. [Statements](#statements)
5. [Program Structure](#program-structure)
6. [Special Constructs](#special-constructs)
7. [Differences from Standard Snobol III](#differences-from-standard-snobol-iii)

---

## Lexical Structure

### Character Classification

Characters are classified into the following categories (as defined in `char_class()` in `sno1.c`):

- **Delimiters**: `(`, `)`, `,`, `=`
- **Operators**: `+`, `-`, `*`, `/`
- **Whitespace**: space (` `), tab (`\t`)
- **String delimiters**: single quote (`'`), double quote (`"`)
- **Special characters**: `$` (pattern immediate value)
- **Other characters**: letters, digits, and other symbols that form identifiers

### Tokens

The lexical analyzer (`compon()` in `sno2.c`) recognizes the following token types:

#### 1. Identifiers
- **Syntax**: A sequence of characters that are not delimiters, operators, or whitespace
- **Examples**: `x`, `var1`, `myFunction`, `abc123`
- **Usage**: Variable names, function names, labels

#### 2. String Literals
- **Syntax**: `"string"` or `'string'`
- **Rules**:
  - Either single or double quotes may be used
  - The same quote character must be used to open and close the string
  - Empty strings are allowed: `""` or `''`
- **Examples**: `"hello"`, `'world'`, `""`

#### 3. Whitespace
- **Syntax**: One or more spaces or tabs
- **Significance**: Whitespace is significant and used for string concatenation
- **Note**: Whitespace must separate certain operators (`*` and `/`) from operands

#### 4. Operators
- `+` - Addition
- `-` - Subtraction
- `*` - Multiplication (must be followed by space for exponentiation)
- `/` - Division (must be followed by space for pattern alternation)
- `**` - Exponentiation (asterisk followed by space)
- `//` - Pattern alternation (slash followed by space)

#### 5. Delimiters
- `(` - Left parenthesis (grouping, function calls)
- `)` - Right parenthesis
- `,` - Comma (argument separator, goto targets)
- `=` - Equals (assignment operator)

#### 6. Special Characters
- `$` - Pattern immediate value operator

### Keywords

The following are reserved keywords (initialized in `main.c`):
- `define` - Function definition
- `end` - End of program marker
- `start` - Program entry point label
- `return` - Return from function
- `freturn` - Failure return from function
- `f` - Failure goto marker
- `s` - Success goto marker

### Built-in Symbols

- `syspit` - System input function (reads a line from input)
- `syspot` - System output variable (writes to output)

---

## Expressions

Expressions are parsed using operator precedence parsing (Shunting Yard algorithm) in the `expr()` function.

### Operator Precedence

Operators have the following precedence (higher numbers = higher precedence):

1. **Exponentiation** (`**`) - Precedence 10
2. **Pattern Alternation** (`//`) - Precedence 11
3. **Multiplication** (`*`) - Precedence 1
4. **Division** (`/`) - Precedence 2
5. **Addition** (`+`) - Precedence 8
6. **Subtraction** (`-`) - Precedence 9
7. **Concatenation** (implicit via whitespace) - Precedence 7

**Note**: Parentheses for arithmetic are not needed due to normal precedence rules. However, the arithmetic operators `/` and `*` must be set off by space to distinguish them from pattern alternation (`//`) and exponentiation (`**`).

### Expression Components

#### 1. Variables
- **Syntax**: `identifier`
- **Examples**: `x`, `result`, `myVar`
- **Semantics**: References a variable's value

#### 2. String Literals
- **Syntax**: `"string"` or `'string'`
- **Examples**: `"hello"`, `'world'`

#### 3. Arithmetic Operations
- **Addition**: `expr1 + expr2`
- **Subtraction**: `expr1 - expr2`
- **Multiplication**: `expr1 * expr2` (note: space required around `*`)
- **Division**: `expr1 / expr2` (note: space required around `/`)
- **Exponentiation**: `expr1 ** expr2` (asterisk followed by space)

#### 4. String Concatenation
- **Syntax**: `expr1 expr2` (whitespace between expressions)
- **Semantics**: Concatenates two strings
- **Example**: `"hello" "world"` produces `"helloworld"`

#### 5. Function Calls
- **Syntax**: `identifier(arg1, arg2, ...)`
- **Rules**:
  - Function name must be an identifier
  - Arguments are comma-separated expressions
  - Empty argument list: `identifier()`
- **Examples**: `f()`, `add(x, y)`, `process("input")`

#### 6. Pattern Immediate Values
- **Syntax**: `$identifier`
- **Semantics**: Returns a reference to the variable (for pattern matching)
- **Example**: `$x` returns a reference to variable `x`

### Expression Examples

```
x + y * z          # Addition and multiplication
a ** b              # Exponentiation
"hello" "world"     # String concatenation
f(x, y)             # Function call
$var                # Pattern immediate value
```

---

## Patterns

Patterns are used in pattern matching statements. They are parsed by the `match()` function in `sno2.c`.

### Pattern Components

#### 1. Simple Patterns
- **Variables**: `identifier` - matches the variable's value
- **String Literals**: `"string"` or `'string'` - matches the literal string
- **Pattern Immediate**: `$identifier` - matches the variable's value dynamically

#### 2. Pattern Alternation
- **Syntax**: `*left*right*`
- **Semantics**: Matches either the left pattern or the right pattern
- **Example**: `*"a"*"b"*` matches either `"a"` or `"b"`

#### 3. Balanced Patterns
- **Syntax**: `*(left*right)*`
- **Semantics**: Matches a balanced pattern (handles nested parentheses)
- **Example**: `*("a"*"b")*` matches a balanced pattern

#### 4. Pattern Concatenation
- **Syntax**: Patterns separated by whitespace
- **Semantics**: Matches patterns in sequence
- **Example**: `"hello" "world"` matches `"hello"` followed by `"world"`

#### 5. Pattern with Length Constraint
- **Syntax**: `*left*right* = length`
- **Semantics**: Matches with a specific length constraint
- **Note**: The right side of the alternation can specify a length

### Pattern Matching Semantics

Pattern matching uses backtracking (implemented in `search()` in `sno3.c`):
- Patterns are matched left-to-right
- On failure, the matcher backtracks to try alternatives
- Balanced patterns maintain parenthesis balance during matching
- Unbalanced patterns extend one character at a time

### Pattern Examples

```
"hello"                    # Simple literal pattern
x                          # Variable pattern
$var                       # Pattern immediate
*"a"*"b"*                  # Alternation pattern
*("a"*"b")*                # Balanced alternation
"hello" "world"             # Concatenated patterns
```

---

## Statements

Statements are compiled by the `compile()` function in `sno2.c`. Each statement has the following structure:

```
[label] statement-body [goto-clause]
```

### Statement Structure

#### 1. Label (Optional)
- **Syntax**: `identifier` followed by whitespace
- **Rules**:
  - All labels except `define` must have a non-empty statement
  - The label `end` marks the end of the program
  - The label `start` marks the program entry point
- **Example**: `loop x = x + 1`

#### 2. Statement Body

The statement body can be one of:

##### a) Simple Expression Statement
- **Syntax**: `expression`
- **Semantics**: Evaluates the expression and discards the result
- **Example**: `x + y`

##### b) Assignment Statement
- **Syntax**: `variable = expression`
- **Rules**:
  - The right side must be non-empty
  - The variable must be a valid identifier
- **Example**: `x = "hello"`, `result = a + b`

##### c) Pattern Matching Statement
- **Syntax**: `subject pattern`
- **Semantics**: Matches the pattern against the subject
- **Example**: `str "hello"`, `x pattern`

##### d) Pattern Matching with Assignment
- **Syntax**: `subject pattern = replacement`
- **Semantics**: Matches pattern in subject and replaces with replacement
- **Example**: `str "old" = "new"`, `x pattern = value`

##### e) Function Definition
- **Syntax**: `define name(params) body`
- **Rules**:
  - `define` is a special label (not a regular label)
  - Parameters are comma-separated identifiers
  - Empty parameter list: `define name() body`
  - Function body is a single statement
- **Examples**:
  ```
  define f()
      return
  define add(a, b)
      return a + b
  ```

#### 3. Goto Clause (Optional)

The goto clause controls program flow after statement execution:

##### a) Simple Goto
- **Syntax**: `, label`
- **Semantics**: Transfers control to the label
- **Example**: `x = x + 1, loop`

##### b) Success/Failure Goto
- **Syntax**: `, (success, failure)`
- **Semantics**: Transfers to `success` on success, `failure` on failure
- **Example**: `str pattern, (ok, fail)`

##### c) Success Goto Only
- **Syntax**: `, s(label)`
- **Semantics**: Transfers to label only on success
- **Example**: `str pattern, s(ok)`

##### d) Failure Goto Only
- **Syntax**: `, f(label)`
- **Semantics**: Transfers to label only on failure
- **Example**: `str pattern, f(fail)`

### Statement Examples

```
# Simple assignment
x = "hello"

# Assignment with goto
x = x + 1, loop

# Pattern matching
str "hello", (found, notfound)

# Pattern matching with assignment
str "old" = "new", (done, error)

# Function definition
define factorial(n)
    n = 0, (base, recurse)
base
    return 1
recurse
    return n * factorial(n - 1)

# Labeled statement
start
    x = syspit()
    x "end", (done, start)
done
    syspot = x
end
```

---

## Program Structure

### Program Compilation

1. **Input Source**: Programs are read from a file (if specified) and then from standard input
2. **Compilation**: All input through a statement containing the label `end` is considered program and is compiled
3. **Post-compilation**: Input after `end` is available to `syspit`

### Entry Point

- If `start` is a label in the program, execution begins there
- If not, execution begins with the first executable statement
- `define` statements are not executable (they are compile-time declarations)

### Execution Flow

1. Program is compiled into a linked list of statements
2. Execution starts at the entry point (or first statement)
3. Statements execute sequentially unless redirected by goto
4. Execution continues until a `return` statement or end of program

### Function Definitions

- Functions are defined at compile time using `define`
- Functions cannot be defined at runtime
- Functions have parameters (no automatic variables other than parameters)
- Functions are called using `identifier(arg1, arg2, ...)`
- Functions return values using `return expression`
- Functions can return failure using `freturn`

### Program Example

```
define add(a, b)
    return a + b

start
    x = syspit()
    x "end", (done, start)
done
    y = add(x, "!")
    syspot = y
end
```

---

## Special Constructs

### Built-in Symbols

#### `syspit`
- **Type**: System function (type 3)
- **Semantics**: Reads a line from input
- **Returns**: String node containing the line, or `NULL` on EOF
- **Usage**: `x = syspit()`

#### `syspot`
- **Type**: System variable (type 4)
- **Semantics**: Output variable - writing to it outputs the value
- **Usage**: `syspot = "hello"` (outputs "hello" followed by newline)

#### `return`
- **Type**: Special label
- **Semantics**: Returns from function with success
- **Usage**: `return expression`

#### `freturn`
- **Type**: Special label
- **Semantics**: Returns from function with failure (sets `rfail` flag)
- **Usage**: `freturn`

### Variable Assignment Semantics

- Variables are dynamically typed (strings)
- Uninitialized variables are initialized to empty string on first use
- Assignment replaces the variable's value
- Variables can be assigned from expressions, function calls, or pattern matches

### Pattern Matching Semantics

Pattern matching uses backtracking:
- Patterns are matched against subject strings
- On success, matched portions can be assigned to variables
- On failure, control can transfer to a failure label
- Pattern matching can replace matched portions with new values

### Function Call Semantics

1. Function parameters are bound to argument values
2. Old parameter values are saved
3. Function body executes
4. Return value is captured
5. Parameter values are restored
6. Return value is returned to caller

---

## Differences from Standard Snobol III

This implementation differs from standard Snobol III in the following ways (as documented in `README.md`):

1. **No unanchored searches**: Use `**` for unanchored search or `*x* b = x c` for unanchored assignment
2. **No back referencing**: Back references are not supported
3. **Function declaration**: Functions are declared at compile time using `define` label
4. **Labels**: All labels except `define` must have non-empty statements
5. **Start label**: `start` label determines entry point; otherwise first statement executes
6. **No builtin functions**: Only `syspit` and `syspot` are available
7. **Arithmetic precedence**: Normal precedence applies; `/` and `*` must be set off by space
8. **Assignments**: Right side of assignments must be non-empty
9. **String quotes**: Either `'` or `"` may be used
10. **Pseudo-variables**: `sysppt` is not available

---

## Grammar Summary (BNF-like)

```
program          ::= statement* end-statement

statement        ::= [label] statement-body [goto-clause]
label            ::= identifier
statement-body   ::= expression-stmt
                  |  assignment-stmt
                  |  pattern-match-stmt
                  |  pattern-assign-stmt
                  |  function-def

expression-stmt  ::= expression
assignment-stmt  ::= variable "=" expression
pattern-match-stmt ::= expression pattern
pattern-assign-stmt ::= expression pattern "=" expression
function-def     ::= "define" identifier "(" [param-list] ")" statement

goto-clause      ::= "," identifier
                  |  "," "(" identifier "," identifier ")"
                  |  "," "s" "(" identifier ")"
                  |  "," "f" "(" identifier ")"

expression       ::= term (("+" | "-") term)*
term             ::= factor (("*" | "/") factor)*
factor           ::= primary ("**" primary)*
primary          ::= identifier
                  |  string-literal
                  |  "$" identifier
                  |  identifier "(" [arg-list] ")"
                  |  "(" expression ")"
                  |  primary primary  // concatenation

pattern          ::= pattern-elem+
pattern-elem     ::= identifier
                  |  string-literal
                  |  "$" identifier
                  |  "*" pattern "*" pattern "*"
                  |  "*" "(" pattern "*" pattern "*" ")"

param-list       ::= identifier ("," identifier)*
arg-list         ::= expression ("," expression)*

identifier       ::= [a-zA-Z0-9_]+
string-literal   ::= '"' [^"]* '"' | "'" [^']* "'"
variable         ::= identifier
```

---

## Implementation Notes

- **Parser**: Expression parsing uses Shunting Yard algorithm (`expr()` in `sno2.c`)
- **Pattern Matching**: Backtracking algorithm (`search()` in `sno3.c`)
- **Execution**: Postfix evaluation for expressions (`eval()` in `sno4.c`)
- **Memory**: String representation uses linked lists of character nodes
- **Symbol Table**: Hash table-like structure for variable lookup (`look()` in `sno1.c`)

---

## References

- Snobol III manual (JACM; Vol. 11 No. 1; Jan 1964; pp 21)
- [Unix V4 Manual](https://dspinellis.github.io/unix-v4man/v4man.pdf)
- [Snobol III Paper](https://dl.acm.org/doi/epdf/10.1145/321203.321207)

