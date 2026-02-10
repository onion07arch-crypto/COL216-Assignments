README – Triangular Numbers in RISC-V (RIPES)

Overview

- This program computes the Triangular number T(n) using four different methods in RISC-V assembly, executed and tested on the RIPES simulator.
- The triangular number is defined as:

T(n) = 1 + 2 + 3 + ⋯ + n = n*(n+1)/2
	​

The program:
- Reads an integer n from the console
- Computes T(n) using:
    - Closed-form formula
    - Iterative loop
    - Simple recursion
    - Recursion with memoization
    - Prints the result of each method to the console

Closed-Form Formula

- Can correctly compute results up to very large n
- Uses 64-bit intermediate results via mul + mulh
- Limited only by 64-bit arithmetic
- Correct for values of n well beyond 100000

Iterative Method

- Uses 32-bit registers
- Maximum correct result before overflow:
- T(n) <= 2^31 − 1
- Beyond this, integer overflow occurs (theoretically, testing not feasible with 1ms gap)

Recursive Method

- Same numeric limitation as iteration (32-bit result)
- Additional limitation due to stack depth
- Large n may cause stack overflow

Memoized Recursive Method

- Same numeric limit as recursion (32-bit result)
- Memo table size limits n ≤ 99 with certainity/safely
- Efficient within this range due to caching

Limitations

- RIPES print syscalls print 32-bit signed integers
- Full 64-bit values must be printed as upper and lower halves
- Recursive versions are limited by stack size
- Memoization array limits maximum cached n to 99

Conclusion

- The closed-form implementation is the most efficient and accurate for large inputs
- Iteration is simple but limited by overflow
- Recursion demonstrates function calls and stack usage
- Memoization improves recursive performance but is limited by memory size