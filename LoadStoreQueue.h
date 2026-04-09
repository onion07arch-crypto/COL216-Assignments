#pragma once
#include <iostream>
#include <vector>
#include <string>
#include "Basics.h"

class LoadStoreQueue {
public:
    // LSQ reservation station
    int latency;    // cycles it takes to access memory

    std::vector<RSEntry> lsq_entries;   // Acts as a FIFO queue
    int max_lsq_size;
    
    bool has_result = false; // result flag
    bool has_exception = false; // exception flag
    int store_data = 0;     // Holds the data to be stored
    int result_value = 0;
    int result_rob_tag = -1;

    LoadStoreQueue(int mem_latency, int lsq_size){
        latency = mem_latency;
        max_lsq_size = lsq_size;
    }
    
    void capture(int tag, int val) {
        for (int i=0; i<lsq_entries.size(); i++){
            if (lsq_entries[i].Qj==tag){
                lsq_entries[i].Vj = val;
                lsq_entries[i].Qj = -1;  // mark the Load Store Queue entry as ready
            }
            if (lsq_entries[i].Qk==tag){
                lsq_entries[i].Vk = val;
                lsq_entries[i].Qk = -1;
            }
        }
    };  // Snoops the bus for register values needed to calculate memory addresses
    void executeCycle(std::vector<int>& Memory) {
        if (lsq_entries.empty()) return;
        auto& front_inst = lsq_entries[0];  // FIFO queue
        if (front_inst.cycles_left > 0){
            front_inst.cycles_left--;
            if (front_inst.cycles_left==0){
                has_result = true;
                has_exception = false;
                result_rob_tag = front_inst.dest_rob_tag;

                int address = front_inst.Vj + front_inst.imm;
                if (address<0 || address>=Memory.size()){
                    has_exception = true;
                }
                else{
                    if (front_inst.op == OpCode::LW){
                        // Store to Load Forwarding
                        bool forwarded = false;
                        for (int i=front_inst.dest_rob_tag-1; i>=0; i--){
                            if (ROB[i].op==OpCode::SW && ROB[i].mem_address==address){
                                result_value=ROB[i].value;
                                forwarded = true;
                                break;
                            }
                        }
                        if (!forwarded){
                            result_value = Memory[address];
                        }
                        
                    }
                    else if (front_inst.op == OpCode::SW){
                        // Pass on to ROB
                        result_value = front_inst.Vk;
                        store_data = front_inst.Vk;
                    }
                }
            }
        }
        else if (front_inst.cycles_left == -1){
            if (front_inst.Qj==-1 && front_inst.Qk==-1){
                front_inst.cycles_left = latency;
            }
        }
    };
};