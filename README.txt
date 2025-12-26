SNO(I)                          2/9/73                          SNO(I)

NAME
        sno − Snobol interpreter

SYNOPSIS
        sno [ file ]

DESCRIPTION

        Sno is a Snobol III (with slight differences) compiler and
        interpreter. Sno obtains input from the concatenation of file
        and the standard input. All input through a statement
        containing the label ‘end’ is considered program and is
        compiled. The rest is available to ‘syspit’.

        Sno differs from Snobol III in the following ways.

        There are no unanchored searches. To get the same effect:

            a **b unanchored search for b
            a *x* b = x c unanchored assignment

        There is no back referencing.

            x="abc"
            a *x* x is an unanchored search for ‘abc’

        Function declaration is different. The function declaration is
        done at compile time by the use of the label ‘define’. Thus
        there is no ability to define functions at run time and the use
        of the name ‘define’ is preempted. There is also no provision
        for automatic variables other than the parameters. For example:

            definef( )
        or
            define f(a,b,c)

        All labels except ‘define’ (even ‘end’) must have a non-empty
        statement.

        If ‘start’ is a label in the program, program execution will
        start there. If not, execution begins with the first executable
        statement. ‘define’ is not an executable statement.

        There are no builtin functions.

        Parentheses for arithmetic are not needed. Normal precedence
        applies. Because of this, the arithmetic operators ‘/’ and ‘*’
        must be set off by space.

        The right side of assignments must be non-empty.

        Either ´ or " may be used for literal quotes.

        The pseudo-variable ‘sysppt’ is not available.

SEE ALSO
        Snobol III manual. (JACM; Vol. 11 No. 1; Jan 1964; pp 21)

        https://dspinellis.github.io/unix-v4man/v4man.pdf

        https://dl.acm.org/doi/epdf/10.1145/321203.321207

BUGS
