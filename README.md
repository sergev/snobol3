# SNO - Snobol III Interpreter

SNO is a Snobol III interpreter (with slight differences) originally from Unix 4 tape by Ken Thompson. This implementation has been modernized to compile with modern C compilers.

## Description

SNO is a compiler and interpreter for Snobol III. It obtains input from a file (if specified) and standard input. All input through a statement containing the label `end` is considered program and is compiled. The rest is available to `syspit`.

## Building

This project uses CMake for building. To build:

```bash
mkdir build
cd build
cmake ..
make
```

The executable `sno` will be created in the build directory.

### Requirements

- CMake 3.10 or later
- A C11-compatible C compiler (GCC, Clang, etc.)

## Usage

```bash
sno [file]
```

If a file is specified, SNO reads from that file first, then from standard input. If no file is specified, SNO reads only from standard input.

## Differences from Snobol III

SNO differs from standard Snobol III in the following ways:

1. **No unanchored searches**: To get the same effect:
   - `a **b` - unanchored search for b
   - `a *x* b = x c` - unanchored assignment

2. **No back referencing**:
   ```snobol
   x="abc"
   a *x* x  ; is an unanchored search for 'abc'
   ```

3. **Function declaration**: Function declaration is done at compile time by the use of the label `define`. Thus there is no ability to define functions at run time and the use of the name `define` is preempted. There is also no provision for automatic variables other than the parameters. For example:
   ```snobol
   define f()
   ```
   or
   ```snobol
   define f(a,b,c)
   ```

4. **Labels**: All labels except `define` (even `end`) must have a non-empty statement.

5. **Start label**: If `start` is a label in the program, program execution will start there. If not, execution begins with the first executable statement. `define` is not an executable statement.

6. **No builtin functions**: There are no builtin functions.

7. **Arithmetic precedence**: Parentheses for arithmetic are not needed. Normal precedence applies. Because of this, the arithmetic operators `/` and `*` must be set off by space.

8. **Assignments**: The right side of assignments must be non-empty.

9. **String quotes**: Either `'` or `"` may be used for literal quotes.

10. **Pseudo-variables**: The pseudo-variable `sysppt` is not available.

## References

- Snobol III manual (JACM; Vol. 11 No. 1; Jan 1964; pp 21)
- [Unix V4 Manual](https://dspinellis.github.io/unix-v4man/v4man.pdf)
- [Snobol III Paper](https://dl.acm.org/doi/epdf/10.1145/321203.321207)

## History

This is the Snobol III implementation from Unix 4 tape, originally written by Ken Thompson. The source code has been modernized to compile with modern C compilers while preserving the original functionality.

## License

This code is from historical Unix sources. Please refer to the original Unix license terms.

