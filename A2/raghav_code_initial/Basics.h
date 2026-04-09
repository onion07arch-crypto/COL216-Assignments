#pragma once
#include <string>

enum class OpCode { ADD, SUB, ADDI, MUL, DIV, REM, LW, SW, BEQ, BNE, BLT, BLE, J, SLT, SLTI, AND, OR, XOR, ANDI, ORI, XORI };
enum class UnitType { ADDER, MULTIPLIER, DIVIDER, LOADSTORE, BRANCH, LOGIC };

struct Instruction {
    OpCode op;
    int dest;
    int src1;
    int src2;
    int imm;
    int pc;
};

struct ProcessorConfig {
    int num_regs = 32;
    int rob_size = 64;
    int mem_size = 1024;

    int logic_lat = 1;
    int add_lat = 2;
    int mul_lat = 4;
    int div_lat = 5;
    int mem_lat = 4;

    int logic_rs_size = 4;
    int adder_rs_size = 4;
    int mult_rs_size = 2;
    int div_rs_size = 2;
    int br_rs_size = 2;
    int lsq_rs_size = 32;
};

struct ROBEntry {
    // valid bit, ready bit, architectural register ID
    // other fields as required

    bool ready = false;     // Has the execution unit finished computing this instruction's result?
    int dest_arch_reg = -1;     // which architectural register to write to, =4 => x4, =-1 for branches or stores
    int value = 0;      // The final computed result to be written to the ARF or Memory.
    bool exception = false;
    OpCode op;
    int pc = 0;     // Which instruction raised exception
    int mem_address = -1;   // holds address for stores
};

struct RSEntry {
    // value, tag, ready ... for both operands
    // other fields as required

    bool busy = false;  // Is this slot currently holding an instruction, or is it free to be allocated?
    OpCode op;
    int Vj = 0;     // Value of first operand
    int Vk = 0;     // Value of second operand
    int Qj = -1;    // The ROB tag for Source 1, =-1 => Vj is ready to use, =4 => waiting for ROB entry 4 to broadcast
    int Qk = -1;
    int dest_rob_tag = -1;  // When this instruction finishes executing, which ROB entry should it broadcast its result to
    int imm = 0;    // constant for addi, slti, branch offsets
    int pc = 0;
    int cycles_left = -1;   // -1 means not started. >0 means executing. 0 means finished

};