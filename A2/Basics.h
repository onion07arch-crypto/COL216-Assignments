#pragma once
#include <string>

enum class OpCode { ADD, SUB, ADDI, MUL, DIV, REM, LW, SW, BEQ, BNE, BLT, BLE, J, SLT, SLTI, AND, OR, XOR, ANDI, ORI, XORI };
enum class UnitType { ADDER, MULTIPLIER, DIVIDER, LOADSTORE, BRANCH, LOGIC };

struct Instruction {
    OpCode op;
    int dest = 0;
    int src1 = 0;
    int src2 = 0;
    int imm = 0;
    int pc = 0;
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
    bool valid = false;
    bool ready = false;
    int dest_arch_reg = -1;
    int value = 0;
    bool exception = false;
    OpCode op;
    int pc = 0;

    // For stores
    bool is_store = false;
    int mem_addr = 0;
    int store_value = 0;

    // For loads
    bool is_load = false;

    // For branches
    bool is_branch = false;
    int predicted_target = 0;
    int actual_target = 0;
    bool branch_taken = false;

    // For jumps (unconditional)
    bool is_jump = false;
};

struct RSEntry {
    bool busy = false;
    bool executing = false;  // in pipeline, not yet complete
    OpCode op;
    int Vj = 0;
    int Vk = 0;
    int Qj = -1;    // -1 means value is ready
    int Qk = -1;
    int dest_rob_tag = -1;
    int imm = 0;
    int pc = 0;
    int dispatch_cycle = 0;  // for ordering (oldest first)
};
