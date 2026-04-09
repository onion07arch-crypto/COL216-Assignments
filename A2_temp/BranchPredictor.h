#pragma once
#include "Basics.h"
#include <iostream>
#include <vector>
#include <unordered_map>

class BranchPredictor {
public:
    int total_branches = 0;
    int correct_predictions = 0;

    // Per-instruction 2-bit saturating counter
    // State 0,1 => predict taken; State 2,3 => predict not taken
    // Starting state = 0 (predict taken)
    std::unordered_map<int, int> state;  // pc -> state

    int predict(int current_pc, int imm, OpCode op) {
        if (op == OpCode::J) {
            // Unconditional jump, always taken
            return current_pc + imm;
        }
        int s = 0; // default state = 0
        if (state.count(current_pc)) {
            s = state[current_pc];
        }
        if (s <= 1) {
            // Predict taken
            return current_pc + imm;
        } else {
            // Predict not taken
            return current_pc + 1;
        }
    }

    void update(int pc, bool taken) {
        total_branches++;
        int s = 0;
        if (state.count(pc)) {
            s = state[pc];
        }

        int old_s = s;

        if (taken) {
            if (s > 0) s--;
        } else {
            if (s < 3) s++;
        }
        state[pc] = s;

        // Determine if prediction was correct
        bool predicted_taken = (old_s <= 1);
        if (predicted_taken == taken) {
            correct_predictions++;
        }
    }
};