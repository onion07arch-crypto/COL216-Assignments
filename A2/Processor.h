#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include "Basics.h"
#include "BranchPredictor.h"
#include "ExecutionUnit.h"
#include "LoadStoreQueue.h"

class Processor {
public:
    int pc;
    int clock_cycle;

    // pipeline registers

    std::vector<Instruction> inst_memory;

    // architectural state (do not change)
    std::vector<int> ARF; // regFile
    std::vector<int> Memory; // Memory
    bool exception = false; // exception bit

    // register alias table / reorder buffer

    std::vector<ExecutionUnit> units;
    LoadStoreQueue* lsq;
    BranchPredictor bp;

    Processor(ProcessorConfig& config) {
        pc = 0;
        clock_cycle = 0;
        ARF.resize(config.num_regs, 0);
        Memory.resize(config.mem_size);

        // Instantiate Hardware Units
        // Adder
        // Multiplier
        // Divider
        // Branch Computation
        // Bitwise Logic
        // Load-Store Unit
    }
    
    void loadProgram(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file: " + filename);
        }

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty()) continue;

            // Memory initialization
            if (line.substr(0, 4) == ".MEM") {
                std::istringstream iss(line.substr(5));
                int val;
                int idx = 0;
                while (iss >> val && idx < (int)Memory.size()) {
                    Memory[idx++] = val;
                }
                continue;
            }

            // Parse instruction
            std::istringstream iss(line);
            std::string opcode;
            iss >> opcode;

            Instruction instr;
            instr.pc = (int)inst_memory.size();

            if (opcode == "add") { instr.op = OpCode::ADD; iss >> instr.dest >> instr.src1 >> instr.src2 >> instr.imm; }
            else if (opcode == "sub") { instr.op = OpCode::SUB; iss >> instr.dest >> instr.src1 >> instr.src2 >> instr.imm; }
            else if (opcode == "mul") { instr.op = OpCode::MUL; iss >> instr.dest >> instr.src1 >> instr.src2 >> instr.imm; }
            else if (opcode == "div") { instr.op = OpCode::DIV; iss >> instr.dest >> instr.src1 >> instr.src2 >> instr.imm; }
            else if (opcode == "rem") { instr.op = OpCode::REM; iss >> instr.dest >> instr.src1 >> instr.src2 >> instr.imm; }
            else if (opcode == "slt") { instr.op = OpCode::SLT; iss >> instr.dest >> instr.src1 >> instr.src2 >> instr.imm; }
            else if (opcode == "and") { instr.op = OpCode::AND; iss >> instr.dest >> instr.src1 >> instr.src2 >> instr.imm; }
            else if (opcode == "or") { instr.op = OpCode::OR; iss >> instr.dest >> instr.src1 >> instr.src2 >> instr.imm; }
            else if (opcode == "xor") { instr.op = OpCode::XOR; iss >> instr.dest >> instr.src1 >> instr.src2 >> instr.imm; }
            else if (opcode == "addi") { instr.op = OpCode::ADDI; iss >> instr.dest >> instr.src1 >> instr.src2 >> instr.imm; }
            else if (opcode == "slti") { instr.op = OpCode::SLTI; iss >> instr.dest >> instr.src1 >> instr.src2 >> instr.imm; }
            else if (opcode == "andi") { instr.op = OpCode::ANDI; iss >> instr.dest >> instr.src1 >> instr.src2 >> instr.imm; }
            else if (opcode == "ori") { instr.op = OpCode::ORI; iss >> instr.dest >> instr.src1 >> instr.src2 >> instr.imm; }
            else if (opcode == "xori") { instr.op = OpCode::XORI; iss >> instr.dest >> instr.src1 >> instr.src2 >> instr.imm; }
            else if (opcode == "lw") { instr.op = OpCode::LW; iss >> instr.dest >> instr.src1 >> instr.src2 >> instr.imm; }
            else if (opcode == "sw") { instr.op = OpCode::SW; iss >> instr.dest >> instr.src1 >> instr.src2 >> instr.imm; }
            else if (opcode == "beq") { instr.op = OpCode::BEQ; iss >> instr.dest >> instr.src1 >> instr.src2 >> instr.imm; }
            else if (opcode == "bne") { instr.op = OpCode::BNE; iss >> instr.dest >> instr.src1 >> instr.src2 >> instr.imm; }
            else if (opcode == "blt") { instr.op = OpCode::BLT; iss >> instr.dest >> instr.src1 >> instr.src2 >> instr.imm; }
            else if (opcode == "ble") { instr.op = OpCode::BLE; iss >> instr.dest >> instr.src1 >> instr.src2 >> instr.imm; }
            else if (opcode == "j") { instr.op = OpCode::J; iss >> instr.dest >> instr.src1 >> instr.src2 >> instr.imm; }
            else {
                continue; // skip unknown
            }

            //instr.printInstr();
            
            if (iss.fail()) {
                std::cerr << "Error: Failed to parse instruction at PC " << instr.pc << ". "
                          << "Make sure to run the preprocessor (compiler.py) first.\n";
                exit(1);
            }
            inst_memory.push_back(instr);
        }
    } 

    void flush() {};

    void broadcastOnCDB() {};

    void stageFetch() {};

    void stageDecode() {};

    void stageExecuteAndBroadcast() {};

    void stageCommit() {};

    bool step() {
        clock_cycle++;
        return true; // return false if CPU has no more to do after this cycle
    }

    void dumpArchitecturalState() {
        std::cout << "\n=== ARCHITECTURAL STATE (CYCLE " << clock_cycle << ") ===\n";
        for (int i = 0; i < ARF.size(); i++) {
            std::cout << "x" << i << ": " << std::setw(4) << ARF[i] << " | ";
            if ((i+1) % 8 == 0) std::cout << std::endl;
        }
        if (exception) {
            std::cout << "EXCEPTION raised by instruction " << pc + 1 << std::endl;
        }
        std::cout << "Branch Predictor Stats: " << bp.correct_predictions << "/" << bp.total_branches << " correct.\n";
    }
};