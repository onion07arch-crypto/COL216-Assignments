.data
buffer: .zero 32        # space to store input
memo:   .zero 400       # space to store an array of 100 integers in memory

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
    add s0, a0, x0                 # Save N in s0
    blt s0, x0, exit
    add t6, s0, x0                 # Save N temporarily in t6
    jal ra, triangular_formula     # First implementation

    mv a0, s3
    li a7, 34                      #print upper 32 bits
    ecall
    
    li a0, 32
    li a7, 11                      #print space
    ecall
    
    mv a0, s2
    li a7, 34                      #print lower 32 bits
    ecall
    
    li a0, 10     
    li a7, 11                      #newline 
    ecall
    
    mv s0, t6
    jal ra, triangular_iteration
    mv a0, t3
    li a7, 34                      #print as hex
    ecall
    
    li a0, 10     
    li a7, 11                      #newline    
    ecall
    
    mv a0, s0
    jal ra, triangular_recursive
    li a7, 34                      #print as hex
    ecall
    
    li a0, 10     
    li a7, 11                      #newline    
    ecall
    
    la s1, memo                    #Store the base address of memo in s1
    mv a0, s0                      #Store N in a0 to pass it to function
    jal ra, triangular_memoization
    mv a0, s4                      #Output will be stored in s4
    li a7, 34                      #print as hex
    ecall
    
    # Exit
    li a7, 10
    ecall
    
    

triangular_formula:
    addi s1, s0, 1        # Save N+1 in s1
    addi t3, x0, 2        # Store 2 in t3
    rem t4, s0, t3        # N % 2 stored in t4
    beq t4, x0, n_even    # If t4==0, branch to n_even
    div s1, s1, t3        # Update N+1 by (N+1)/2
    mul s2, s0, s1        # Lower 32 bits of N(N+1)/2
    mulh s3, s0, s1       # Upper 32 bits of N(N+1)/2
    jalr x0, ra, 0        # Return to triangular
n_even:
    div s0, s0, t3        # Update N by N/2
    mul s2, s0, s1        # Lower 32 bits of N(N+1)/2
    mulh s3, s0, s1       # Upper 32 bits of N(N+1)/2
    jalr x0, ra, 0        # Return to triangular
    
    
    
triangular_iteration:
    addi t6, t6, 1
                      # t6 holds the number of iterations to be performed
    add t3, x0, x0    # t3 will store the sum after ith iteration
    add t4, x0, x0    # t4 holds the iteration counter
loop:
    beq t4, t6, exit_loop
    add t3, t3, t4
    addi t4, t4, 1
    jal x0, loop
exit_loop:
    jalr x0, ra, 0
    
    
    
triangular_recursive:
    addi sp, sp, -16               # create stack frame
    sw ra, 12(sp)                  # save return address
    sw a0, 8(sp)                   # save n

    beqz a0, base_case_recursive   # if n == 0, return 0

    addi a0, a0, -1                # a0 = n - 1
    jal ra, triangular_recursive   # recursive call
    
    lw t0, 8(sp)                   # restore original n
    add a0, a0, t0                 # a0 = T(n-1) + n
    j end_rec
base_case_recursive:
    li a0, 0                       # T(0) = 0
end_rec:
    lw ra, 12(sp)                  # restore return address
    addi sp, sp, 16                # destroy stack frame
    ret
    
    
    
triangular_memoization:
    beq a0, x0, base_case_memo        # If a0=0 we reached base case
    slli t3, a0, 2                    # t3 holds 4*n
    add t3, t3, s1                    # t3 now holds memo + n*4
    lw t4, 0( t3)                     # Loads memo[n] into  t4
    addi sp, sp, -12                  # Adjust stack by 3 items
    sw ra, 8(sp)                      # Save return address to stack
    sw a0, 4(sp)                      # Save n to stack
    sw t3, 0(sp)
    bne t4, x0, memoize               # if memo[n]!=0 return stored value
                                      # If value is not stored
    addi a0, a0, -1                   # Decrement n by 1
    jal ra, triangular_memoization    # Recursive call
    lw a0, 4(sp)                      # restore original n before this call
    lw ra, 8(sp)
    lw t3, 0(sp)
    addi sp, sp, 12                   # Cleanup
    add s4, s4, a0
    sw s4, 0(t3)
    jalr x0, ra, 0
base_case_memo:
    add  s4, x0, x0
    jalr x0, ra, 0
memoize:
    add s4, x0, t4
    lw ra, 8(sp)
    addi sp, sp, 12
    jalr x0, ra, 0
    
    
    
exit:
    li a7, 10
    ecall

    


