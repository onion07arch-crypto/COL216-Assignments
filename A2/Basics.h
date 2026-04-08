#pragma once
#include <iostream>
#include <string>

enum class OpCode { ADD, SUB, ADDI, MUL, DIV, REM, LW, SW, BEQ, BNE, BLT, BLE, J, SLT, SLTI, AND, OR, XOR, ANDI, ORI, XORI };
enum class UnitType { ADDER, MULTIPLIER, DIVIDER, LOADSTORE, BRANCH, LOGIC };

//for debugging
inline std::string to_string(OpCode op) {
    switch (op) {
        case OpCode::ADD: return "ADD";
        case OpCode::SUB: return "SUB";
        case OpCode::ADDI: return "ADDI";
        case OpCode::MUL: return "MUL";
        case OpCode::DIV: return "DIV";
        case OpCode::REM: return "REM";
        case OpCode::LW: return "LW";
        case OpCode::SW: return "SW";
        case OpCode::BEQ: return "BEQ";
        case OpCode::BNE: return "BNE";
        case OpCode::BLT: return "BLT";
        case OpCode::BLE: return "BLE";
        case OpCode::J: return "J";
        case OpCode::SLT: return "SLT";
        case OpCode::SLTI: return "SLTI";
        case OpCode::AND: return "AND";
        case OpCode::OR: return "OR";
        case OpCode::XOR: return "XOR";
        case OpCode::ANDI: return "ANDI";
        case OpCode::ORI: return "ORI";
        case OpCode::XORI: return "XORI";
    }
    return "UNKNOWN";
}

struct Instruction {
    OpCode op;
    int dest;
    int src1;
    int src2;
    int imm;
    int pc;

    //for debugging
    void printInstr(){
        std::cout<<"Opcode: "<<to_string(op)<<std::endl;
        std::cout<<"Dest: "<<dest<<std::endl;
        std::cout<<"Src1: "<<src1<<std::endl;
        std::cout<<"Src2: "<<src2<<std::endl;
        std::cout<<"Imm: "<<imm<<std::endl;
        std::cout<<"PC: "<<pc<<std::endl;    
    }
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
};

struct RSEntry {
    // value, tag, ready ... for both operands
    // other fields as required
};