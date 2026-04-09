#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>
#include <algorithm>
#include <climits>
#include "Basics.h"
#include "BranchPredictor.h"
#include "ExecutionUnit.h"
#include "LoadStoreQueue.h"

class Processor {
public:
    int pc;
    int clock_cycle;
    ProcessorConfig config;

    std::vector<Instruction> inst_memory;

    // Architectural state
    std::vector<int> ARF;
    std::vector<int> Memory;
    bool exception = false;

    // ROB (circular buffer)
    std::vector<ROBEntry> ROB;
    int rob_head = 0;
    int rob_tail = 0;
    int rob_count = 0;

    // RAT: maps arch reg -> ROB tag (-1 = not renamed)
    std::vector<int> RAT;

    // Execution units
    std::vector<ExecutionUnit> units;
    LoadStoreQueue lsq;

    BranchPredictor bp;

    // Pipeline state
    bool halted = false;
    bool fetch_stalled = false;
    bool flushed_this_cycle = false;
    int decode_dispatch_cycle = 0;

    // Fetch buffer
    bool fetch_buffer_valid = false;
    Instruction fetch_buffer;

    Processor(ProcessorConfig& cfg) : config(cfg) {
        pc = 0;
        clock_cycle = 0;
        ARF.resize(config.num_regs, 0);
        Memory.resize(config.mem_size, 0);
        ROB.resize(config.rob_size);
        RAT.resize(config.num_regs, -1);

        units.push_back(ExecutionUnit(UnitType::ADDER, config.add_lat, config.adder_rs_size));
        units.push_back(ExecutionUnit(UnitType::MULTIPLIER, config.mul_lat, config.mult_rs_size));
        units.push_back(ExecutionUnit(UnitType::DIVIDER, config.div_lat, config.div_rs_size));
        units.push_back(ExecutionUnit(UnitType::LOGIC, config.logic_lat, config.logic_rs_size));
        units.push_back(ExecutionUnit(UnitType::BRANCH, config.add_lat, config.br_rs_size));
        lsq = LoadStoreQueue(config.mem_lat, config.lsq_rs_size);
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

            if (iss.fail()) {
                std::cerr << "Error: Failed to parse instruction at PC " << instr.pc << ". "
                          << "Make sure to run the preprocessor (compiler.py) first.\n";
                exit(1);
            }
            inst_memory.push_back(instr);
        }
    }

    // ============================================================
    // FETCH STAGE
    // ============================================================
    void stageFetch() {
        if (halted) return;
        if (fetch_buffer_valid) return;  // previous fetch not yet consumed

        if (pc < 0 || pc >= (int)inst_memory.size()) return;

        fetch_buffer = inst_memory[pc];
        fetch_buffer.pc = pc;
        fetch_buffer_valid = true;

        // Predict next PC
        OpCode op = fetch_buffer.op;
        if (op == OpCode::J) {
            pc = pc + fetch_buffer.imm;
        } else if (op == OpCode::BEQ || op == OpCode::BNE ||
                   op == OpCode::BLT || op == OpCode::BLE) {
            pc = bp.predict(pc, fetch_buffer.imm, op);
        } else {
            pc = pc + 1;
        }
    }

    // ============================================================
    // DECODE / DISPATCH STAGE
    // ============================================================
    void stageDecode() {
        if (!fetch_buffer_valid) return;
        if (halted) return;

        Instruction& instr = fetch_buffer;
        OpCode op = instr.op;

        // Check ROB space
        if (rob_count >= config.rob_size) return;  // stall

        // Determine target unit and check RS availability
        ExecutionUnit* target_unit = nullptr;
        bool use_lsq = false;
        bool is_jump = (op == OpCode::J);

        if (!is_jump) {
            if (op == OpCode::LW || op == OpCode::SW) {
                use_lsq = true;
                if (lsq.isFull()) return;  // stall
            } else if (op == OpCode::ADD || op == OpCode::SUB || op == OpCode::ADDI || op == OpCode::SLT || op == OpCode::SLTI) {
                target_unit = &units[0];
            } else if (op == OpCode::MUL) {
                target_unit = &units[1];
            } else if (op == OpCode::DIV || op == OpCode::REM) {
                target_unit = &units[2];
            } else if (op == OpCode::AND || op == OpCode::OR || op == OpCode::XOR || op == OpCode::ANDI || op == OpCode::ORI || op == OpCode::XORI) {
                target_unit = &units[3];
            } else if (op == OpCode::BEQ || op == OpCode::BNE || op == OpCode::BLT || op == OpCode::BLE) {
                target_unit = &units[4];
            }

            if (target_unit && target_unit->findFreeRS() == -1) return;  // stall
        }

        // Allocate ROB entry
        int rob_tag = rob_tail;
        ROBEntry& rob_entry = ROB[rob_tag];
        rob_entry = ROBEntry();  // reset
        rob_entry.valid = true;
        rob_entry.op = op;
        rob_entry.pc = instr.pc;

        // Determine instruction type and set up ROB/RS
        if (is_jump) {
            // J: no execution unit, immediately ready
            rob_entry.ready = true;
            rob_entry.is_jump = true;
            rob_entry.dest_arch_reg = -1;
        } else if (op == OpCode::LW) {
            rob_entry.dest_arch_reg = instr.dest;
            rob_entry.is_load = true;

            RSEntry rs_entry;
            rs_entry.busy = true;
            rs_entry.op = op;
            rs_entry.dest_rob_tag = rob_tag;
            rs_entry.imm = instr.imm;
            rs_entry.pc = instr.pc;
            rs_entry.dispatch_cycle = clock_cycle;

            // src1 = base register
            readOperand(instr.src1, rs_entry.Vj, rs_entry.Qj);
            rs_entry.Qk = -1;  // no second operand for loads

            lsq.addEntry(rs_entry);

            // Update RAT
            if (instr.dest > 0 && instr.dest < config.num_regs) {
                RAT[instr.dest] = rob_tag;
            } else if (instr.dest != 0) {
                std::cerr << "Warning: Invalid destination register " << instr.dest << " at PC " << instr.pc << "\n";
            }
        } else if (op == OpCode::SW) {
            rob_entry.dest_arch_reg = -1;
            rob_entry.is_store = true;

            RSEntry rs_entry;
            rs_entry.busy = true;
            rs_entry.op = op;
            rs_entry.dest_rob_tag = rob_tag;
            rs_entry.imm = instr.imm;
            rs_entry.pc = instr.pc;
            rs_entry.dispatch_cycle = clock_cycle;

            // For SW: dest field = data reg, src1 = base reg (from preprocessor)
            // SW format: sw data_reg base_reg 0 offset
            readOperand(instr.dest, rs_entry.Vj, rs_entry.Qj);   // data to store
            readOperand(instr.src1, rs_entry.Vk, rs_entry.Qk);    // base register

            lsq.addEntry(rs_entry);
        } else if (op == OpCode::BEQ || op == OpCode::BNE ||
                   op == OpCode::BLT || op == OpCode::BLE) {
            rob_entry.dest_arch_reg = -1;
            rob_entry.is_branch = true;
            // Store predicted target for resolution at commit
            rob_entry.predicted_target = bp.predict(instr.pc, instr.imm, op);

            int rs_idx = target_unit->findFreeRS();
            RSEntry& rs_entry = target_unit->rs[rs_idx];
            rs_entry = RSEntry();
            rs_entry.busy = true;
            rs_entry.op = op;
            rs_entry.dest_rob_tag = rob_tag;
            rs_entry.imm = instr.imm;
            rs_entry.pc = instr.pc;
            rs_entry.dispatch_cycle = clock_cycle;

            // Branches: dest field = src1, src1 field = src2 (from preprocessor)
            readOperand(instr.dest, rs_entry.Vj, rs_entry.Qj);
            readOperand(instr.src1, rs_entry.Vk, rs_entry.Qk);
        } else {
            // ALU / Logic instructions
            rob_entry.dest_arch_reg = instr.dest;

            int rs_idx = target_unit->findFreeRS();
            RSEntry& rs_entry = target_unit->rs[rs_idx];
            rs_entry = RSEntry();
            rs_entry.busy = true;
            rs_entry.op = op;
            rs_entry.dest_rob_tag = rob_tag;
            rs_entry.imm = instr.imm;
            rs_entry.pc = instr.pc;
            rs_entry.dispatch_cycle = clock_cycle;

            // Read source operands
            readOperand(instr.src1, rs_entry.Vj, rs_entry.Qj);

            // R-type: use src2 register; I-type: no second register
            bool is_i_type = (op == OpCode::ADDI || op == OpCode::SLTI ||
                              op == OpCode::ANDI || op == OpCode::ORI ||
                              op == OpCode::XORI);
            if (is_i_type) {
                rs_entry.Qk = -1;
                rs_entry.Vk = 0;
            } else {
                readOperand(instr.src2, rs_entry.Vk, rs_entry.Qk);
            }

            // Update RAT
            if (instr.dest != 0) {
                RAT[instr.dest] = rob_tag;
            }
        }

        // Advance ROB tail
        rob_tail = (rob_tail + 1) % config.rob_size;
        rob_count++;
        fetch_buffer_valid = false;  // consumed
    }

    void readOperand(int reg, int& value, int& tag) {
        if (reg < 0 || reg >= config.num_regs) {
            std::cerr << "Warning: Invalid register index " << reg << ". Treating as x0.\n";
            value = 0;
            tag = -1;
            return;
        }
        if (reg == 0) {
            // x0 is always 0
            value = 0;
            tag = -1;
            return;
        }
        if (RAT[reg] != -1) {
            int rob_idx = RAT[reg];
            if (ROB[rob_idx].valid && ROB[rob_idx].ready) {
                value = ROB[rob_idx].value;
                tag = -1;
            } else {
                tag = rob_idx;
                value = 0;
            }
        } else {
            value = ARF[reg];
            tag = -1;
        }
    }

    // ============================================================
    // EXECUTE AND BROADCAST STAGE
    // ============================================================
    void stageExecuteAndBroadcast() {
        if (halted) return;

        // Phase 1: Advance all pipelines (produces outputs)
        for (auto& unit : units) unit.executeCycle(clock_cycle);
        lsq.executeCycle(Memory, ROB, rob_head, rob_count, config.rob_size);

        // Phase 2: Broadcast all results (updates RS entries and ROB)
        for (auto& unit : units) broadcastUnit(unit);
        broadcastLSQ();

        // Phase 3: Start new instructions (RS entries now updated by broadcasts)
        for (auto& unit : units) unit.startNewInstruction();
        lsq.startNewInstruction(Memory);
    }

    void broadcastUnit(ExecutionUnit& unit) {
        if (!unit.has_result) return;

        int tag = unit.result_rob_tag;
        int val = unit.result_value;
        bool exc = unit.has_exception;

        // Update ROB
        ROB[tag].ready = true;
        ROB[tag].value = val;
        ROB[tag].exception = exc;

        // For branches, compute actual target
        if (ROB[tag].is_branch) {
            bool taken = (val != 0);
            ROB[tag].branch_taken = taken;
            if (taken) {
                ROB[tag].actual_target = ROB[tag].pc + ROB[tag].value;
                // Wait, value is 1 or 0 for branch taken/not-taken
                // We need the actual target: pc + imm if taken, pc + 1 if not
                ROB[tag].actual_target = taken ? (ROB[tag].pc + findImm(tag)) : (ROB[tag].pc + 1);
            } else {
                ROB[tag].actual_target = ROB[tag].pc + 1;
            }
        }

        // Snoop CDB: update all RS entries waiting for this result
        for (auto& u : units) u.capture(tag, val);
        lsq.capture(tag, val);
    }

    int findImm(int rob_tag) {
        // Find the immediate value for a branch instruction in the ROB
        for (auto& instr : inst_memory) {
            if (instr.pc == ROB[rob_tag].pc) {
                return instr.imm;
            }
        }
        return 0;
    }

    void broadcastLSQ() {
        if (!lsq.has_result) return;

        int tag = lsq.result_rob_tag;
        int val = lsq.result_value;
        bool exc = lsq.has_exception;

        ROB[tag].ready = true;
        ROB[tag].exception = exc;

        if (lsq.result_is_store) {
            ROB[tag].mem_addr = lsq.result_mem_addr;
            ROB[tag].store_value = lsq.result_store_value;
            ROB[tag].value = val;
        } else {
            ROB[tag].value = val;
        }

        // Snoop CDB
        for (auto& u : units) u.capture(tag, val);
        lsq.capture(tag, val);
    }

    // ============================================================
    // COMMIT STAGE
    // ============================================================
    void stageCommit() {
        if (rob_count == 0) return;

        ROBEntry& head = ROB[rob_head];
        if (!head.valid || !head.ready) return;

        // Check for exception
        if (head.exception) {
            exception = true;
            pc = head.pc;
            flushPipeline();
            halted = true;
            // Don't commit this instruction
            return;
        }

        // Commit based on instruction type
        if (head.is_store) {
            // Write to memory
            if (head.mem_addr >= 0 && head.mem_addr < (int)Memory.size()) {
                Memory[head.mem_addr] = head.store_value;
            }
        } else if (head.is_branch) {
            // Check prediction
            bool taken = head.branch_taken;
            bp.update(head.pc, taken);

            if (head.predicted_target != head.actual_target) {
                // Misprediction: flush and set correct PC
                pc = head.actual_target;

                // Dequeue this ROB entry first
                head.valid = false;
                rob_head = (rob_head + 1) % config.rob_size;
                rob_count--;

                flushPipeline();
                flushed_this_cycle = true;
                return;
            }
        } else if (head.is_jump) {
            // Jump: nothing special to commit
        } else {
            // ALU/Logic/Load: write result to ARF
            if (head.dest_arch_reg > 0 && head.dest_arch_reg < config.num_regs) {
                ARF[head.dest_arch_reg] = head.value;
                // Clear RAT if it still points to this ROB entry
                if (RAT[head.dest_arch_reg] == rob_head) {
                    RAT[head.dest_arch_reg] = -1;
                }
            }
        }

        // For stores and jumps, also clear RAT if applicable
        if (head.is_store || head.is_jump || head.is_branch) {
            // These don't write to registers, no RAT update needed
        }

        // Dequeue ROB head
        head.valid = false;
        rob_head = (rob_head + 1) % config.rob_size;
        rob_count--;
    }

    // ============================================================
    // FLUSH
    // ============================================================
    void flushPipeline() {
        // Clear all RS, pipelines, fetch buffer
        for (auto& unit : units) unit.flush();
        lsq.flush();

        fetch_buffer_valid = false;

        // Clear remaining ROB entries (everything still in ROB)
        // First, fix RAT for any entries being flushed
        for (int i = 0; i < rob_count; i++) {
            int idx = (rob_head + i) % config.rob_size;
            if (ROB[idx].valid && ROB[idx].dest_arch_reg > 0) {
                if (RAT[ROB[idx].dest_arch_reg] == idx) {
                    RAT[ROB[idx].dest_arch_reg] = -1;
                }
            }
            ROB[idx].valid = false;
        }
        rob_count = 0;
        rob_tail = rob_head;

        // Also clear any RAT entries that point to invalid ROB entries
        for (int i = 0; i < config.num_regs; i++) {
            if (RAT[i] != -1 && !ROB[RAT[i]].valid) {
                RAT[i] = -1;
            }
        }
    }

    // ============================================================
    // STEP (one cycle)
    // ============================================================
    bool step() {
        clock_cycle++;
        flushed_this_cycle = false;

        // Execute/broadcast first so results are ready for commit this cycle
        stageExecuteAndBroadcast();
        stageCommit();
        if (!flushed_this_cycle) {
            stageDecode();
            stageFetch();
        }

        // x0 is always 0
        ARF[0] = 0;

        // Check termination: halted (exception) or no more work
        if (halted) return false;

        // If ROB is empty and no more instructions to fetch and no fetch buffer
        bool no_more_instr = (pc < 0 || pc >= (int)inst_memory.size());
        if (rob_count == 0 && !fetch_buffer_valid && no_more_instr) {
            return false;
        }

        return true;
    }

    // ============================================================
    // OUTPUT
    // ============================================================
    void dumpArchitecturalState() {
        std::cout << "\n=== ARCHITECTURAL STATE (CYCLE " << clock_cycle << ") ===" << std::endl;
        for (int i = 0; i < (int)ARF.size(); i++) {
            std::cout << "x" << i << ": " << std::setw(4) << ARF[i] << " | ";
            if ((i + 1) % 8 == 0) std::cout << std::endl;
        }
        if (exception) {
            std::cout << "EXCEPTION raised by instruction " << pc + 1 << std::endl;
        }
        std::cout << "Branch Predictor Stats: " << bp.correct_predictions
                  << "/" << bp.total_branches << " correct." << std::endl;
    }
};