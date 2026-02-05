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
    add s0, a0, x0       # save N in s0 (optional)
    jal ra, triangular_formula_recursive

    li a7, 1            # sys_call: print as unsigned int
    ecall

    li a7, 10            # sys_call: exit
    ecall
    
triangular_formula_recursive:
    addi sp, sp, -16     # create stack frame
    sw ra, 12(sp)        # save return address
    sw a0, 8(sp)         # save n

    beqz a0, base_case   # if n == 0, return 0

    addi a0, a0, -1      # a0 = n - 1
    jal ra, triangular_formula_recursive  # recursive call

    lw t0, 8(sp)         # restore original n
    add a0, a0, t0       # a0 = T(n-1) + n
    j end_rec

base_case:
    li a0, 0             # T(0) = 0

end_rec:
    lw ra, 12(sp)        # restore return address
    addi sp, sp, 16      # destroy stack frame
    ret
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

