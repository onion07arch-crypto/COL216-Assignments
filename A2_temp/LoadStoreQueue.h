#pragma once
#include <iostream>
#include <vector>
#include <deque>
#include <string>
#include "Basics.h"

class LoadStoreQueue {
public:
    int latency;
    std::vector<RSEntry> lsq_entries;   // Acts as a FIFO queue
    int max_lsq_size;

    struct LSQPipeSlot {
        bool valid = false;
        int rob_tag = -1;
        int result = 0;
        bool exception = false;
        bool is_store = false;
        int mem_addr = 0;
        int store_value = 0;
    };
    std::vector<LSQPipeSlot> pipeline;

    bool has_result = false;
    bool has_exception = false;
    int result_rob_tag = -1;
    int result_value = 0;
    bool result_is_store = false;
    int result_mem_addr = 0;
    int result_store_value = 0;

    LoadStoreQueue() : latency(4), max_lsq_size(32) { pipeline.resize(latency); }

    LoadStoreQueue(int lat, int size) : latency(lat), max_lsq_size(size) { pipeline.resize(latency); }

    bool isFull() {
        return (int)lsq_entries.size() >= max_lsq_size;
    }

    // Add entry to back of LSQ (in program order)
    void addEntry(const RSEntry& entry) {
        lsq_entries.push_back(entry);
    }

    // Snoop CDB
    void capture(int tag, int val) {
        for (auto& entry : lsq_entries) {
            if (entry.Qj == tag) {
                entry.Vj = val;
                entry.Qj = -1;
            }
            if (entry.Qk == tag) {
                entry.Vk = val;
                entry.Qk = -1;
            }
        }
    }

    // Execute one cycle (in-order)
    void executeCycle(std::vector<int>& Memory, std::vector<ROBEntry>& ROB,
                      int rob_head, int rob_count, int rob_size) {
        has_result = false;
        has_exception = false;

        // 1. Check output
        if (pipeline[latency - 1].valid) {
            has_result = true;
            result_rob_tag = pipeline[latency - 1].rob_tag;
            result_is_store = pipeline[latency - 1].is_store;
            result_mem_addr = pipeline[latency - 1].mem_addr;

            if (result_is_store) {
                result_value = pipeline[latency - 1].store_value;
                result_store_value = pipeline[latency - 1].store_value;
                has_exception = pipeline[latency - 1].exception;
            } else {
                // Load: read from memory, then check store forwarding
                int addr = pipeline[latency - 1].mem_addr;
                bool exc = pipeline[latency - 1].exception;
                int val = 0;
                if (!exc) {
                    val = Memory[addr];
                    // Store forwarding
                    int forwarded_val = val;
                    for (int i = 0; i < rob_count; i++) {
                        int idx = (rob_head + i) % rob_size;
                        if (idx == result_rob_tag) break;
                        if (ROB[idx].valid && ROB[idx].is_store &&
                            ROB[idx].ready && ROB[idx].mem_addr == addr) {
                            forwarded_val = ROB[idx].store_value;
                        }
                    }
                    val = forwarded_val;
                }
                result_value = val;
                has_exception = exc;
            }
        }

        // 2. Shift pipeline
        for (int i = latency - 1; i > 0; i--) {
            pipeline[i] = pipeline[i - 1];
        }
        pipeline[0] = LSQPipeSlot();
    }

    // Phase 2: Start new instruction from front of queue (in-order)
    void startNewInstruction(std::vector<int>& Memory) {
        if (!lsq_entries.empty() && lsq_entries.front().busy &&
            lsq_entries.front().Qj == -1 && lsq_entries.front().Qk == -1) {
            RSEntry& e = lsq_entries.front();
            bool is_store = (e.op == OpCode::SW);
            int base_val = is_store ? e.Vk : e.Vj;
            int addr = base_val + e.imm;
            bool exc = false;

            if (addr < 0 || addr >= (int)Memory.size()) {
                exc = true;
            }

            pipeline[0].valid = true;
            pipeline[0].rob_tag = e.dest_rob_tag;
            pipeline[0].is_store = is_store;
            pipeline[0].mem_addr = addr;
            pipeline[0].exception = exc;

            if (is_store) {
                pipeline[0].store_value = e.Vj;
            }

            lsq_entries.erase(lsq_entries.begin());
        }
    }

    void flush() {
        lsq_entries.clear();
        for (auto& s : pipeline) s.valid = false;
        has_result = false;
        has_exception = false;
    }
};
