#pragma once
#include <iostream>
#include <vector>
#include <string>
#include "Basics.h"

class ExecutionUnit {
public:
    // per-unit reservation station
    UnitType name;
    int latency;    // how many cycles this unit takes for execution

    std::vector<RSEntry> rs_entries;    // Reservation Station for this unit
    int max_rs_size;
    
    bool has_result = false; // result flag
    bool has_exception = false; // exception flag
    int result_value = 0;
    int result_rob_tag = -1;    // Which ROB entry to broadcast this result to

    ExecutionUnit(UnitType type, int unit_latency, int rs_size){
        name = type;
        latency = unit_latency;
        max_rs_size = rs_size;
    }
    
    void capture(int tag, int val) {
        for (int i=0; i<rs_entries.size(); i++){
            if (rs_entries[i].Qj==tag){
                rs_entries[i].Vj = val;
                rs_entries[i].Qj = -1;  // mark the Reservation station entry as ready
            }
            if (rs_entries[i].Qk==tag){
                rs_entries[i].Vk = val;
                rs_entries[i].Qk = -1;
            }
        }
    };
    void executeCycle() {
        // Process already executing instructions
        for (int i=0; i<rs_entries.size(); i++){
            if (rs_entries[i].cycles_left > 0){
                rs_entries[i].cycles_left--;
                if (rs_entries[i].cycles_left==0){
                    // Execution finished
                    has_result = true;
                    has_exception = false;
                    result_rob_tag = rs_entries[i].dest_rob_tag;
                    OpCode op = rs_entries[i].op;

                    long long safe_calc = 0;
                    bool check_bounds = false;
                    if (op==OpCode::ADD || op==OpCode::ADDI){
                        safe_calc = (long long)rs_entries[i].Vj + (long long)rs_entries[i].Vk;
                        check_bounds = true;
                    }
                    else if (op==OpCode::SUB){
                        safe_calc = (long long)rs_entries[i].Vj - (long long)rs_entries[i].Vk;
                        check_bounds = true;
                    }
                    else if (op==OpCode::MUL){
                        safe_calc = (long long)rs_entries[i].Vj * (long long)rs_entries[i].Vk;
                        check_bounds = true;
                    }
                    else if (op==OpCode::DIV){
                        if (rs_entries[i].Vk==0){
                            has_exception=true;
                        }
                        else if (rs_entries[i].Vj == -2147483648LL && rs_entries[i].Vk == -1) {
                            has_exception = true; 
                        }
                        else{
                            result_value = rs_entries[i].Vj/rs_entries[i].Vk;
                        }
                    }
                    else if (op==OpCode::REM){
                        if (rs_entries[i].Vk==0){
                            has_exception=true;
                        }
                        else{
                            result_value = rs_entries[i].Vj % rs_entries[i].Vk;
                        }
                    }
                    if (check_bounds){
                        if (safe_calc>2147483647LL || safe_calc<-2147483648LL){
                            has_exception = true;
                        }
                        else{
                            result_value = (int)safe_calc;
                        }
                    }
                    else if (op==OpCode::AND || op==OpCode::ANDI){
                        result_value = rs_entries[i].Vj & rs_entries[i].Vk;
                    }
                    else if (op==OpCode::OR || op==OpCode::ORI){
                        result_value = rs_entries[i].Vj | rs_entries[i].Vk;
                    }
                    else if (op==OpCode::XOR || op==OpCode::XORI){
                        result_value = rs_entries[i].Vj ^ rs_entries[i].Vk;
                    }
                    else if (op==OpCode::SLT || op==OpCode::SLTI){
                        result_value = (rs_entries[i].Vj < rs_entries[i].Vk) ? 1 : 0;
                    }

                    else if (op==OpCode::BEQ){
                        result_value = (rs_entries[i].Vj == rs_entries[i].Vk) ? 1 : 0;
                    }
                    else if (op==OpCode::BNE){
                        result_value=(rs_entries[i].Vj != rs_entries[i].Vk) ? 1 : 0;
                    }
                    else if (op==OpCode::BLT){
                        result_value = (rs_entries[i].Vj < rs_entries[i].Vk) ? 1 : 0;
                    }
                    else if (op==OpCode::BLE){
                        result_value = (rs_entries[i].Vj <= rs_entries[i].Vk) ? 1 : 0;
                    }
                }
            }
        }
        for (int i=0; i<rs_entries.size(); i++){
            if (rs_entries[i].Qj==-1 && rs_entries[i].Qk==-1 && rs_entries[i].cycles_left==-1){
                rs_entries[i].cycles_left=latency;
                break;  // ensures more than one instructions in the reservation station don't start getting executed in same cycle
            }
        }
    };
};