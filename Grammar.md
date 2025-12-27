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
- **Significance**:
  - Whitespace is significant and used for string concatenation
  - **Required** at the start of statement bodies (even without labels)
  - **Required** between labels and statement bodies on the same line
- **Note**:
  - Whitespace must separate certain operators (`*` and `/`) from operands
  - **Newlines are NOT whitespace** - they end statements and are handled by the line-oriented input system

#### 4. Operators
- `+` - Addition
- `-` - Subtraction
- `*` - Multiplication (or unanchored search when followed by non-space)
- `/` - Division (or goto target when followed by non-space)

#### 5. Delimiters
- `(` - Left parenthesis (grouping, function calls)
- `)` - Right parenthesis
- `,` - Comma (argument separator)
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

1. **Multiplication** (`*`) - Precedence 1
2. **Division** (`/`) - Precedence 2
3. **Addition** (`+`) - Precedence 8
4. **Subtraction** (`-`) - Precedence 9
5. **Concatenation** (implicit via whitespace) - Precedence 7

**Note**: Parentheses for arithmetic are not needed due to normal precedence rules. However, arithmetic operators `*` and `/` must be set off by space to distinguish it from other usage.

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

**Important**: This is a **line-oriented language**. Each statement must be on a **single line**. Statements cannot span multiple lines.

#### 1. Label (Optional)
- **Syntax**: `identifier` followed by **whitespace (space or tab) on the same line**
- **Rules**:
  - Labels **must be on the same line** as the statement body
  - There **must be whitespace** (space or tab) between the label and the statement body
  - All labels except `define` must have a non-empty statement
  - The label `end` marks the end of the program
  - The label `start` marks the program entry point
  - Newlines are **not** whitespace - they end the statement
- **Example**: `loop x = x + 1` (label and statement on same line)
- **Invalid**:
  ```
  loop
      x = x + 1
  ```
  (label on separate line - this would be parsed as two statements)

#### 2. Statement Body

**Important**: The statement body **must start with whitespace** (space or tab), even if there is no label. This is enforced by the parser (see line 448-449 in `sno2.c`).

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
- **Syntax**: `define name(params)` on one line, followed by function body statement on next line
- **Rules**:
  - `define` is a special label (not a regular label)
  - The `define` statement itself must end with end-of-line after the closing parenthesis
  - Parameters are comma-separated identifiers
  - Empty parameter list: `define name()`
  - Function body is a single statement on the **next line** (this is the only case where a statement can follow a label on a separate line)
- **Examples**:
  ```
  define f()
      return
  define add(a, b)
      return a + b
  ```
  Note: The function body statement must still start with whitespace on its line.

#### 3. Goto Clause (Optional)

The goto clause controls program flow after statement execution:

##### a) Simple Goto
- **Syntax**: `/(label)`
- **Semantics**: Transfers control to the label
- **Example**: `x = x + 1   /(loop)`

##### b) Success/Failure Goto
- **Syntax**: `/s(success)f(failure)`
- **Semantics**: Transfers to `success` on success, `failure` on failure
- **Example**: `str pattern     /s(ok)f(fail)`

##### c) Success Goto Only
- **Syntax**: `/s(label)`
- **Semantics**: Transfers to label only on success
- **Example**: `str pattern     /s(ok)`

##### d) Failure Goto Only
- **Syntax**: `/f(label)`
- **Semantics**: Transfers to label only on failure
- **Example**: `str pattern     /f(fail)`

### Statement Examples

**Note**: All examples show statements on single lines. Whitespace at the start of statement bodies is required.

```
# Simple assignment (must start with whitespace)
    x = "hello"

# Assignment with goto
    x = x + 1   /(loop)

# Pattern matching
    str "hello"     /s(found)f(notfound)

# Pattern matching with assignment
    str "old" = "new"   /s(done)f(error)

# Function definition (define on one line, body on next)
define factorial(n)
    n = 0           /s(base)f(recurse)
base
    return 1
recurse
    return n * factorial(n - 1)

# Labeled statements (label and body on same line)
start   x = syspit()
        x "end"         /s(done)f(start)
done    syspot = x
end
```

**Important Formatting Rules**:
- Each statement is on a single line
- Labels must be on the same line as their statement body
- Statement bodies must start with whitespace (space or tab)
- The only exception is function definitions: `define name(params)` ends a line, and the function body is on the next line

---

## Program Structure

### Program Compilation

1. **Input Source**: Programs are read from a file (if specified) and then from standard input
2. **Line-oriented Processing**: The parser reads one line at a time using `syspit()` (see `getc_char()` in `sno1.c`)
   - Each line is processed as a complete statement
   - When a line ends, `getc_char()` returns NULL (type 0), signaling end of statement
   - Newlines are not treated as whitespace - they terminate statements
3. **Compilation**: All input through a statement containing the label `end` is considered program and is compiled
4. **Post-compilation**: Input after `end` is available to `syspit`

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
define  add(a, b)
        return a + b
start   x = syspit()
        x "end"         /s(done)f(start)
done    y = add(x, "!")
        syspot = y
end
```

**Note**: In this example:
- `define add(a, b)` is on one line (ends with newline after closing paren)
- Function body `return a + b` is on the next line (starts with whitespace)
- `start    x = syspit()` shows label and statement on same line with whitespace between
- All other statements start with whitespace

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
4. **Labels**: All labels except `define` must have non-empty statements on the same line
5. **Line-oriented**: Each statement must be on a single line; labels must be on the same line as their statement body
6. **Whitespace requirement**: Statement bodies must start with whitespace (space or tab)
7. **Start label**: `start` label determines entry point; otherwise first statement executes
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

goto-clause      ::= "/" "(" identifier ")"
                  |  "/" "s" "(" identifier ")" "f" "(" identifier ")"
                  |  "/" "s" "(" identifier ")"
                  |  "/" "f" "(" identifier ")"

expression       ::= term (("+" | "-") term)*
term             ::= factor (("*" | "/") factor)*
factor           ::= primary
primary          ::= identifier
                  |  string-literal
                  |  "$" identifier
                  |  identifier "(" [arg-list] ")"
                  |  "(" expression ")"
                  |  primary primary        // concatenation

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
