.data
buffer: .zero 32        # space to store input

.text
.globl main
main:

    # Read from Console
    li a0, 0            # file descriptor 0 = stdin
    la a1, buffer       # buffer address
    li a2, 32           # max bytes
    li a7, 63           # syscall: read
    ecall

    # Convert ASCII to integer
    la t0, buffer       # pointer to buffer
    li t1, 0            # result = 0

convert_loop:
    lb t2, 0(t0)        # load one character

    li t3, 10           # newline '\n'
    beq t2, t3, done    # stop when Enter is pressed

    addi t2, t2, -48    # convert ASCII to digit
    li t4, 10
    mul t1, t1, t4      # result *= 10
    add t1, t1, t2      # result += digit

    addi t0, t0, 1      # move to next char
    j convert_loop


done:
    mv a0,t1
    
triangular:
    add x8,x10,x0    # Save N in s0
    jal x1, triangular_formula    # First implementation
    mv a0,x19
    li a7,34
    ecall
    mv a0,x18
    li a7,34
    ecall
    # Exit
    li a7, 10
    ecall

triangular_formula:
    addi x9,x8,1    # Save N+1 in s1
    addi x28,x0,2    # Store 2 in t3
    rem x29,x8,x28    # N % 2 stored in t4
    beq x29,x0,n_even    # If x29==0, branch to n_even
    div x9,x9,x28    # Update N+1 by (N+1)/2
    mul x18,x8,x9    # Lower 32 bits of N(N+1)/2
    mulh x19,x8,x9    # Upper 32 bits of N(N+1)/2
    jalr x0,x1,0    # Return to triangular
n_even:
    div x8,x8,x28    # Update N by N/2
    mul x18,x8,x9    # Lower 32 bits of N(N+1)/2
    mulh x19,x8,x9    # Upper 32 bits of N(N+1)/2
    jalr x0,x1,0    # Return to triangular


    


