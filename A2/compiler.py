#!/usr/bin/env python3
"""
Preprocessor for RISC-V assembly files.
Strips comments, resolves labels, replaces memory references,
and outputs a clean format for the C++ simulator.

Usage: python3 compiler.py <filename.s>
Overwrites the input file with preprocessed output.
"""

import sys
import re

def preprocess(filename):
    with open(filename, 'r') as f:
        raw_lines = f.readlines()

    # Phase 1: Strip comments, trim whitespace
    lines = []
    for line in raw_lines:
        # Remove inline comments (# ...)
        idx = line.find('#')
        if idx != -1:
            line = line[:idx]
        line = line.strip()
        if line:
            lines.append(line)

    # Phase 2: Parse memory declarations and code labels
    memory_labels = {}  # label -> base address in memory
    memory_data = []     # flat list of all memory values
    code_labels = {}     # label -> instruction index (PC)
    instructions_raw = []  # list of (original_line, original_index)

    mem_offset = 0
    for line in lines:
        # Memory declaration: .LABEL: val1 val2 ...
        mem_match = re.match(r'^\.(\w+):\s*(.*)', line)
        if mem_match:
            label = mem_match.group(1)
            values_str = mem_match.group(2).strip()
            values = list(map(int, values_str.split()))
            memory_labels[label] = mem_offset
            memory_data.extend(values)
            mem_offset += len(values)
            continue

        # Code label on its own line: "label:"
        label_match = re.match(r'^(\w+):\s*$', line)
        if label_match:
            label = label_match.group(1)
            code_labels[label] = len(instructions_raw)
            continue

        # Code label followed by instruction: "label: instruction"
        label_instr_match = re.match(r'^(\w+):\s+(.+)$', line)
        if label_instr_match:
            label = label_instr_match.group(1)
            code_labels[label] = len(instructions_raw)
            instr_line = label_instr_match.group(2).strip()
            instructions_raw.append(instr_line)
            continue

        # Regular instruction
        instructions_raw.append(line)

    # Phase 3: Parse and resolve each instruction
    r_type_ops = {'add', 'sub', 'mul', 'div', 'rem', 'slt', 'and', 'or', 'xor'}
    i_type_ops = {'addi', 'slti', 'andi', 'ori', 'xori'}
    branch_ops = {'beq', 'bne', 'blt', 'ble'}

    output_instructions = []

    for pc, line in enumerate(instructions_raw):
        # Remove commas, normalize whitespace
        line = line.replace(',', ' ')
        parts = line.split()
        opcode = parts[0].lower()

        if opcode in r_type_ops:
            # R-type: opcode rd rs1 rs2
            rd = parse_reg(parts[1])
            rs1 = parse_reg(parts[2])
            rs2 = parse_reg(parts[3])
            output_instructions.append(f"{opcode} {rd} {rs1} {rs2} 0")

        elif opcode in i_type_ops:
            # I-type: opcode rd rs1 imm
            rd = parse_reg(parts[1])
            rs1 = parse_reg(parts[2])
            imm = int(parts[3])
            output_instructions.append(f"{opcode} {rd} {rs1} 0 {imm}")

        elif opcode in ('lw', 'sw'):
            # lw rd, offset(rs1)
            rd = parse_reg(parts[1])
            # Reconstruct the memory operand string, as it might have been split
            # if there were spaces, e.g., "0( x28)" -> ["0(", "x28)"]
            mem_op_str = "".join(parts[2:])
            base_reg, offset = parse_mem_operand(mem_op_str, memory_labels)
            output_instructions.append(f"{opcode} {rd} {base_reg} 0 {offset}")

        elif opcode in branch_ops:
            # branch rs1 rs2 label_or_offset
            rs1 = parse_reg(parts[1])
            rs2 = parse_reg(parts[2])
            target_str = parts[3]
            if target_str in code_labels:
                offset = code_labels[target_str] - pc
            else:
                offset = int(target_str)
            output_instructions.append(f"{opcode} {rs1} {rs2} 0 {offset}")

        elif opcode == 'j':
            # j label_or_offset
            target_str = parts[1]
            if target_str in code_labels:
                offset = code_labels[target_str] - pc
            else:
                offset = int(target_str)
            output_instructions.append(f"j 0 0 0 {offset}")

        else:
            # Unknown instruction, pass through (shouldn't happen)
            output_instructions.append(line)

    # Phase 4: Write output back to file
    with open(filename, 'w') as f:
        # Memory initialization line
        if memory_data:
            f.write(".MEM " + " ".join(map(str, memory_data)) + "\n")
        # Instructions
        for instr in output_instructions:
            f.write(instr + "\n")


REG_ALIASES = {
    'zero': 0, 'ra': 1, 'sp': 2, 'gp': 3, 'tp': 4, 't0': 5, 't1': 6, 't2': 7,
    's0': 8, 's1': 9, 'a0': 10, 'a1': 11, 'a2': 12, 'a3': 13, 'a4': 14, 'a5': 15,
    'a6': 16, 'a7': 17, 's2': 18, 's3': 19, 's4': 20, 's5': 21, 's6': 22, 's7': 23,
    's8': 24, 's9': 25, 's10': 26, 's11': 27, 't3': 28, 't4': 29, 't5': 30, 't6': 31
}

def parse_reg(s):
    """Parse register name like 'x4' or 'x0' to integer 4 or 0."""
    s = s.strip().lower()
    if s in REG_ALIASES:
        return REG_ALIASES[s]
    elif s.startswith('x'):
        return int(s[1:])
    else:
        print("Error: Invalid register name: ", s)
        exit(1)


def parse_mem_operand(s, memory_labels):
    """
    Parse memory operand like 'A(x1)' or '0(x1)'.
    Returns (base_reg_num, offset).
    """
    match = re.match(r'(\w+)\((\w+)\)', s)
    if not match:
        raise ValueError(f"Cannot parse memory operand: {s}")
    label_or_offset = match.group(1)
    base_reg = parse_reg(match.group(2))

    if label_or_offset in memory_labels:
        offset = memory_labels[label_or_offset]
    else:
        offset = int(label_or_offset)

    return base_reg, offset


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: python3 compiler.py <filename.s>")
        sys.exit(1)
    preprocess(sys.argv[1])
