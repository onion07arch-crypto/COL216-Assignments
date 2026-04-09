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
    bool has_fetched_inst = false;
    Instruction fetched_inst;

    // architectural state (do not change)
    std::vector<int> ARF; // regFile
    std::vector<int> Memory; // Memory
    bool exception = false; // exception bit

    // register alias table / reorder buffer

    std::vector<ExecutionUnit> units;
    LoadStoreQueue* lsq;
    BranchPredictor bp;

    std::vector<int> RAT;
    std::vector<ROBEntry> ROB;
    int max_rob_size;

    Processor(ProcessorConfig& config) {
        pc = 0;
        clock_cycle = 0;
        ARF.resize(config.num_regs, 0);
        Memory.resize(config.mem_size);

        max_rob_size = config.rob_size;
        RAT.resize(config.num_regs, -1);

        // Instantiate Hardware Units
        // Adder
        units.push_back(ExecutionUnit(UnitType::ADDER, config.add_lat, config.adder_rs_size));
        // Multiplier
        units.push_back(ExecutionUnit(UnitType::MULTIPLIER, config.mul_lat, config.mult_rs_size));
        // Divider
        units.push_back(ExecutionUnit(UnitType::DIVIDER, config.div_lat, config.div_rs_size));
        // Bitwise Logic
        units.push_back(ExecutionUnit(UnitType::LOGIC, config.logic_lat, config.logic_rs_size));
        // Branch Computation
        units.push_back(ExecutionUnit(UnitType::BRANCH, config.add_lat, config.br_rs_size));
        // Load-Store Unit
        lsq = new LoadStoreQueue(config.mem_lat, config.lsq_rs_size);
    }

    void loadProgram(const std::string& filename) {
        std::ifstream file(filename);
    }

    void flush() {};

    void broadcastOnCDB() {};

    void stageFetch() {};

    void stageDecode() {
        if (!has_fetched_inst){return;}

        ExecutionUnit* target_unit = nullptr;
        bool is_lsq = false;
        OpCode op = fetched_inst.op;
        if (op==OpCode::ADD || op==OpCode::SUB || op==OpCode::ADDI || op==OpCode::SLT || op==OpCode::SLTI){
            target_unit = &units[0];
        }
        else if (op==OpCode::MUL){
            target_unit = &units[1];
        }
        else if (op==OpCode::DIV || op==OpCode::REM){
            target_unit = &units[2];
        }
        else if (op==OpCode::AND || op==OpCode::OR || op==OpCode::XOR || op==OpCode::ANDI || op==OpCode::ORI || op==OpCode::XORI){
            target_unit = &units[3];
        }
        else if (op==OpCode::BEQ || op==OpCode::BNE || op==OpCode::BLT || op==OpCode::BLE){
            target_unit = &units[4];
        }
        else if (op==OpCode::LW || op==OpCode::SW){
            is_lsq = true;
        }
        else if (op==OpCode::J){
            target_unit = nullptr;
        }

        if (ROB.size()>=max_rob_size){return;}  // Stall as ROB is full
        if (is_lsq && lsq->lsq_entries.size()>=lsq->max_lsq_size){return;}  // Stall when instruction is lw/sw and LSQ is full
        if (target_unit!=nullptr && target_unit->rs_entries.size()>=target_unit->max_rs_size){return;}  // Stall when RS is full

        // In decode stage instruction is allocated to ROB
        int new_rob_tag = ROB.size();   // tag = index in ROB vector
        ROBEntry new_rob;
        new_rob.op = op;
        new_rob.pc = fetched_inst.pc;
        bool writes_to_reg = true;
        if (op==OpCode::SW || op==OpCode::BEQ || op==OpCode::BNE || op==OpCode::BLT || op==OpCode::BLE || op==OpCode::J){
            writes_to_reg = false;
        }
        new_rob.dest_arch_reg = writes_to_reg ? fetched_inst.dest:-1;
        if (op==OpCode::J){
            new_rob.ready = true;
        }
        ROB.push_back(new_rob);

        // Creating new entry in Reservation Station
        if (target_unit!=nullptr || is_lsq){
            RSEntry new_rs;
            new_rs.busy = true;
            new_rs.op = op;
            new_rs.dest_rob_tag = new_rob_tag;
            new_rs.pc = fetched_inst.pc;

            int src1 = fetched_inst.src1;
            if (src1==0){
                new_rs.Vj = 0;
            }
            else if (RAT[src1]!=-1){
                new_rs.Qj = RAT[src1];  // Waiting for another instruction to terminate execution
            }
            else{
                new_rs.Vj = ARF[src1];
            }
            int src2 = fetched_inst.src2;
            if (op==OpCode::LW || op==OpCode::SW){
                new_rs.imm = fetched_inst.imm;
                if (op==OpCode::SW){
                    int store_reg = fetched_inst.dest;
                    if (store_reg==0){
                        new_rs.Vk=0;
                    }
                    else if (RAT[store_reg]!=-1){
                        new_rs.Qk = RAT[store_reg];
                    }
                    else{
                        new_rs.Vk = ARF[store_reg];
                    }
                }
            }
            else if (op==OpCode::ADDI || op==OpCode::SLTI || op==OpCode::ANDI || op==OpCode::ORI || op==OpCode::XORI){
                new_rs.Vk = fetched_inst.imm;
            }
            else{
                if (src2==0){
                    new_rs.Vk = 0;
                }
                else if (RAT[src2]!=-1){
                    new_rs.Qk = RAT[src2];
                }
                else{
                    new_rs.Vk = ARF[src2];
                }
                if (!writes_to_reg && op!=OpCode::SW){
                    // i.e. for branches
                    new_rs.imm = fetched_inst.imm;
                }
            }

            if (is_lsq){
                lsq->lsq_entries.push_back(new_rs);
            }
            else{
                target_unit->rs_entries.push_back(new_rs);
            }
        }
        if (writes_to_reg && fetched_inst.dest!=0){
            // Point the register to the new ROB tag
            RAT[fetched_inst.dest] = new_rob_tag;
        }
        has_fetched_inst = false;   // Instruction dispatched
    };

    void stageExecuteAndBroadcast() {
        // Start all Execution Units
        for (int i=0; i<units.size(); i++){
            units[i].executeCycle();
        }
        lsq->executeCycle(Memory,ROB);

        // Broadcasting for non memory units
        for (int i=0; i<units.size(); i++){
            if (units[i].has_result){
                int tag = units[i].result_rob_tag;
                int val = units[i].result_value;
                bool exc = units[i].has_exception;
                ROB[tag].ready = true;
                ROB[tag].value = val;
                ROB[tag].exception = exc;

                // Broadcast for snooping
                for (int j=0; j<units.size();j++){
                    units[j].capture(tag,val);
                }
                lsq->capture(tag,val);

                // Remove finish instruction from Reservation Station
                for (auto it = units[i].rs_entries.begin(); it!=units[i].rs_entries.end(); ++it){
                    if (it->cycles_left==0){
                        units[i].rs_entries.erase(it);
                        break;
                    }
                }
                units[i].has_result = false;    // Reset for next cycle
            }
        }
        // Broadcasting for memory lsq
        if (lsq->has_result){
            int tag = lsq->result_rob_tag;
            int val = lsq->result_value;
            bool exc = lsq->has_exception;
            ROB[tag].ready = true;
            ROB[tag].value = val;
            ROB[tag].exception = exc;

            if (lsq->lsq_entries[0].op == OpCode::SW){
                ROB[tag].mem_address = lsq->lsq_entries[0].Vj + lsq->lsq_entries[0].imm;
            }

            for (int j=0; j<units.size();j++){
                units[j].capture(tag,val);
            }
            lsq->capture(tag,val);

            for (auto it=lsq->lsq_entries.begin(); it!=lsq->lsq_entries.end();++it){
                if (it->cycles_left==0){
                    lsq->lsq_entries.erase(it);
                    break;
                }
            }
            lsq->has_result = false;
        }
    };

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