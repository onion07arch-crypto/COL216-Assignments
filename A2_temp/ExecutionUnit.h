#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <climits>
#include "Basics.h"

struct PipelineSlot {
    bool valid = false;
    int rob_tag = -1;
    int result = 0;
    bool exception = false;
    int rs_index = -1;  // index into RS to free on completion
};

class ExecutionUnit {
public:
    UnitType name;
    int latency;
    std::vector<RSEntry> rs;
    std::vector<PipelineSlot> pipeline;  // shift register, index 0 = newest

    bool has_result = false;
    bool has_exception = false;
    int result_rob_tag = -1;
    int result_value = 0;

    ExecutionUnit() {}

    ExecutionUnit(UnitType type, int lat, int rs_size) {
        name = type;
        latency = lat;
        rs.resize(rs_size);
        pipeline.resize(lat);
    }

    // Find a free RS slot, return index or -1
    int findFreeRS() {
        for (int i = 0; i < (int)rs.size(); i++) {
            if (!rs[i].busy) return i;
        }
        return -1;
    }

    // Snoop CDB: update RS entries waiting for this tag
    void capture(int tag, int val) {
        for (auto& entry : rs) {
            if (!entry.busy) continue;
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

    // Execute one cycle: advance pipeline, start new instruction
    void executeCycle(int current_cycle) {
        has_result = false;
        has_exception = false;

        // 1. Check output end of pipeline
        if (pipeline[latency - 1].valid) {
            has_result = true;
            result_rob_tag = pipeline[latency - 1].rob_tag;
            result_value = pipeline[latency - 1].result;
            has_exception = pipeline[latency - 1].exception;
            // Free RS entry now that computation is complete
            int rs_idx = pipeline[latency - 1].rs_index;
            if (rs_idx >= 0 && rs_idx < (int)rs.size()) {
                rs[rs_idx].busy = false;
                rs[rs_idx].executing = false;
            }
        }

        // 2. Shift pipeline forward
        for (int i = latency - 1; i > 0; i--) {
            pipeline[i] = pipeline[i - 1];
        }
        pipeline[0] = PipelineSlot();  // clear input slot
    }

    // Phase 2: Start new instruction from RS (called after broadcasts update RS)
    void startNewInstruction() {
        // Find oldest ready RS entry (not already executing) to start
        int best = -1;
        int best_cycle = INT_MAX;
        for (int i = 0; i < (int)rs.size(); i++) {
            if (rs[i].busy && !rs[i].executing && rs[i].Qj == -1 && rs[i].Qk == -1) {
                if (rs[i].dispatch_cycle < best_cycle) {
                    best_cycle = rs[i].dispatch_cycle;
                    best = i;
                }
            }
        }

        if (best != -1) {
            // Compute result
            RSEntry& e = rs[best];
            int result = 0;
            bool exc = false;
            compute(e, result, exc);

            pipeline[0].valid = true;
            pipeline[0].rob_tag = e.dest_rob_tag;
            pipeline[0].result = result;
            pipeline[0].exception = exc;
            pipeline[0].rs_index = best;

            // Mark as executing (don't free yet)
            e.executing = true;
        }
    }

    void compute(const RSEntry& e, int& result, bool& exc) {
        long long a = e.Vj;
        long long b = e.Vk;
        long long imm = e.imm;
        exc = false;

        switch (e.op) {
            case OpCode::ADD: {
                long long r = a + b;
                if (r > 2147483647LL || r < -2147483648LL) exc = true;
                result = (int)r;
                break;
            }
            case OpCode::SUB: {
                long long r = a - b;
                if (r > 2147483647LL || r < -2147483648LL) exc = true;
                result = (int)r;
                break;
            }
            case OpCode::ADDI: {
                long long r = a + imm;
                if (r > 2147483647LL || r < -2147483648LL) exc = true;
                result = (int)r;
                break;
            }
            case OpCode::MUL: {
                long long r = a * b;
                if (r > 2147483647LL || r < -2147483648LL) exc = true;
                result = (int)r;
                break;
            }
            case OpCode::DIV: {
                if (b == 0) { exc = true; result = 0; }
                else result = (int)(a / b);
                break;
            }
            case OpCode::REM: {
                if (b == 0) { exc = true; result = 0; }
                else result = (int)(a % b);
                break;
            }
            case OpCode::SLT:
                result = (a < b) ? 1 : 0;
                break;
            case OpCode::SLTI:
                result = (a < imm) ? 1 : 0;
                break;
            case OpCode::AND:
                result = (int)(a & b);
                break;
            case OpCode::OR:
                result = (int)(a | b);
                break;
            case OpCode::XOR:
                result = (int)(a ^ b);
                break;
            case OpCode::ANDI:
                result = (int)(a & imm);
                break;
            case OpCode::ORI:
                result = (int)(a | imm);
                break;
            case OpCode::XORI:
                result = (int)(a ^ imm);
                break;
            // Branch resolution
            case OpCode::BEQ:
                result = (a == b) ? 1 : 0;
                break;
            case OpCode::BNE:
                result = (a != b) ? 1 : 0;
                break;
            case OpCode::BLT:
                result = (a < b) ? 1 : 0;
                break;
            case OpCode::BLE:
                result = (a <= b) ? 1 : 0;
                break;
            default:
                result = 0;
                break;
        }
    }

    void flush() {
        for (auto& e : rs) e.busy = false;
        for (auto& s : pipeline) s.valid = false;
        has_result = false;
        has_exception = false;
    }
};