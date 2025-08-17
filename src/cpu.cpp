#include "cpu.h"
#include <iostream>

cpu::cpu() : PC(0x0000) {
    initInstructionTable();
}

void cpu::reset() {
    PC = read(0xFFFC) | (read(0xFFFD) << 8);
    SP = 0xFD;
    A = X = Y = 0;
    P = 0x34;
}

void cpu::loadAt0600AndSetReset(const std::vector<Byte>& program) {
    for (size_t i = 0; i < program.size(); ++i) {
        write(0x0600 + i, program[i]);
    }
    write(0xFFFC, 0x00);
    write(0xFFFD, 0x06);
}

void cpu::execute() {
    Byte opcode = read(PC++);
    instructionTable[opcode]();
}

Byte cpu::read(Word address) const {
    return memory[address];
}

void cpu::write(Word address, Byte value) {
    memory[address] = value;
}

void cpu::setFlag(StatusFlags flag, bool value) {
    if (value) P |= flag;
    else P &= ~flag;
}

bool cpu::getFlag(StatusFlags flag) const {
    return (P & flag) != 0;
}

Word cpu::getAddress(AddressingMode mode) {
    switch (mode) {
        case Immediate:
            return PC++;
        case ZeroPage:
            return read(PC++);
        case ZeroPageX:
            return (read(PC++) + X) & 0xFF;
        case ZeroPageY:
            return (read(PC++) + Y) & 0xFF;
        case Absolute: {
            Byte lo = read(PC++);
            Byte hi = read(PC++);
            return lo | (hi << 8);
        }
        case AbsoluteX: {
            Byte lo = read(PC++);
            Byte hi = read(PC++);
            return ((hi << 8) | lo) + X;
        }
        case AbsoluteY: {
            Byte lo = read(PC++);
            Byte hi = read(PC++);
            return ((hi << 8) | lo) + Y;
        }
        case Indirect: {
            Byte lo = read(PC++);
            Byte hi = read(PC++);
            Word addr = lo | (hi << 8);
            if ((addr & 0x00FF) == 0x00FF) { // 6502 page boundary bug
                return read(addr) | (read(addr & 0xFF00) << 8);
            }
            return read(addr) | (read(addr + 1) << 8);
        }
        case IndirectX: {
            Byte zpAddr = read(PC++);
            Byte lo = read((zpAddr + X) & 0xFF);
            Byte hi = read((zpAddr + X + 1) & 0xFF);
            return lo | (hi << 8);
        }
        case IndirectY: {
            Byte zpAddr = read(PC++);
            Byte lo = read(zpAddr);
            Byte hi = read((zpAddr + 1) & 0xFF);
            return ((hi << 8) | lo) + Y;
        }
        default:
            return 0;
    }
}

Byte cpu::fetch(AddressingMode mode) {
    if (mode == Accumulator) {
        return A;
    }
    return read(getAddress(mode));
}

void cpu::setZN(Byte value) {
    setFlag(Z, value == 0);
    setFlag(N, value & 0x80);
}

void cpu::op_ADC(Byte value) {
    Word sum = A + value + (getFlag(C) ? 1 : 0);
    setFlag(C, sum > 0xFF);
    setFlag(V, (~(A ^ value) & (A ^ sum) & 0x80) != 0);
    A = sum & 0xFF;
    setZN(A);
}

void cpu::op_SBC(Byte value) {
    op_ADC(~value);
}

void cpu::op_AND(Byte value) { A &= value; setZN(A); }
void cpu::op_EOR(Byte value) { A ^= value; setZN(A); }
void cpu::op_ORA(Byte value) { A |= value; setZN(A); }

void cpu::op_ASL(Byte& value) { setFlag(C, value & 0x80); value <<= 1; setZN(value); }
void cpu::op_LSR(Byte& value) { setFlag(C, value & 0x01); value >>= 1; setZN(value); }
void cpu::op_ROL(Byte& value) { bool carry = getFlag(C); setFlag(C, value & 0x80); value <<= 1; value |= (carry ? 1 : 0); setZN(value); }
void cpu::op_ROR(Byte& value) { bool carry = getFlag(C); setFlag(C, value & 0x01); value >>= 1; value |= (carry ? 0x80 : 0); setZN(value); }

void cpu::op_INC(Byte& value) { value++; setZN(value); }
void cpu::op_DEC(Byte& value) { value--; setZN(value); }

void cpu::op_CMP(Byte value) { setFlag(C, A >= value); setZN(A - value); }
void cpu::op_CPX(Byte value) { setFlag(C, X >= value); setZN(X - value); }
void cpu::op_CPY(Byte value) { setFlag(C, Y >= value); setZN(Y - value); }

void cpu::op_BIT(Byte value) { setFlag(Z, (A & value) == 0); setFlag(N, value & 0x80); setFlag(V, value & 0x40); }

template <void (cpu::*op)(Byte)>
void cpu::ins_read(AddressingMode mode) {
    (this->*op)(fetch(mode));
}

template <void (cpu::*op)(Byte&)>
void cpu::ins_rmw(AddressingMode mode) {
    if (mode == Accumulator) {
        (this->*op)(A);
    } else {
        Word addr = getAddress(mode);
        Byte value = read(addr);
        (this->*op)(value);
        write(addr, value);
    }
}

void cpu::initInstructionTable() {
    for (int i = 0; i < 256; i++) instructionTable[i] = [this](){ opcodeUnknown(); };

    // Load
    instructionTable[0xA9] = [this](){ A = fetch(Immediate); setZN(A); };
    instructionTable[0xA5] = [this](){ A = fetch(ZeroPage); setZN(A); };
    instructionTable[0xB5] = [this](){ A = fetch(ZeroPageX); setZN(A); };
    instructionTable[0xAD] = [this](){ A = fetch(Absolute); setZN(A); };
    instructionTable[0xBD] = [this](){ A = fetch(AbsoluteX); setZN(A); };
    instructionTable[0xB9] = [this](){ A = fetch(AbsoluteY); setZN(A); };
    instructionTable[0xA1] = [this](){ A = fetch(IndirectX); setZN(A); };
    instructionTable[0xB1] = [this](){ A = fetch(IndirectY); setZN(A); };
    instructionTable[0xA2] = [this](){ X = fetch(Immediate); setZN(X); };
    instructionTable[0xA6] = [this](){ X = fetch(ZeroPage); setZN(X); };
    instructionTable[0xB6] = [this](){ X = fetch(ZeroPageY); setZN(X); };
    instructionTable[0xAE] = [this](){ X = fetch(Absolute); setZN(X); };
    instructionTable[0xBE] = [this](){ X = fetch(AbsoluteY); setZN(X); };
    instructionTable[0xA0] = [this](){ Y = fetch(Immediate); setZN(Y); };
    instructionTable[0xA4] = [this](){ Y = fetch(ZeroPage); setZN(Y); };
    instructionTable[0xB4] = [this](){ Y = fetch(ZeroPageX); setZN(Y); };
    instructionTable[0xAC] = [this](){ Y = fetch(Absolute); setZN(Y); };
    instructionTable[0xBC] = [this](){ Y = fetch(AbsoluteX); setZN(Y); };

    // Store
    instructionTable[0x85] = [this](){ write(getAddress(ZeroPage), A); };
    instructionTable[0x95] = [this](){ write(getAddress(ZeroPageX), A); };
    instructionTable[0x8D] = [this](){ write(getAddress(Absolute), A); };
    instructionTable[0x9D] = [this](){ write(getAddress(AbsoluteX), A); };
    instructionTable[0x99] = [this](){ write(getAddress(AbsoluteY), A); };
    instructionTable[0x81] = [this](){ write(getAddress(IndirectX), A); };
    instructionTable[0x91] = [this](){ write(getAddress(IndirectY), A); };
    instructionTable[0x86] = [this](){ write(getAddress(ZeroPage), X); };
    instructionTable[0x96] = [this](){ write(getAddress(ZeroPageY), X); };
    instructionTable[0x8E] = [this](){ write(getAddress(Absolute), X); };
    instructionTable[0x84] = [this](){ write(getAddress(ZeroPage), Y); };
    instructionTable[0x94] = [this](){ write(getAddress(ZeroPageX), Y); };
    instructionTable[0x8C] = [this](){ write(getAddress(Absolute), Y); };

    // Transfer
    instructionTable[0xAA] = [this](){ X = A; setZN(X); };
    instructionTable[0xA8] = [this](){ Y = A; setZN(Y); };
    instructionTable[0xBA] = [this](){ X = SP; setZN(X); };
    instructionTable[0x8A] = [this](){ A = X; setZN(A); };
    instructionTable[0x9A] = [this](){ SP = X; };
    instructionTable[0x98] = [this](){ A = Y; setZN(A); };

    // Stack
    instructionTable[0x48] = [this](){ write(0x0100 + SP--, A); };
    instructionTable[0x08] = [this](){ write(0x0100 + SP--, P | B | U); };
    instructionTable[0x68] = [this](){ A = read(0x0100 + ++SP); setZN(A); };
    instructionTable[0x28] = [this](){ P = read(0x0100 + ++SP); P &= ~B; P |= U; };

    // Arithmetic
    instructionTable[0x69] = [this](){ ins_read<&cpu::op_ADC>(Immediate); };
    instructionTable[0x65] = [this](){ ins_read<&cpu::op_ADC>(ZeroPage); };
    instructionTable[0x75] = [this](){ ins_read<&cpu::op_ADC>(ZeroPageX); };
    instructionTable[0x6D] = [this](){ ins_read<&cpu::op_ADC>(Absolute); };
    instructionTable[0x7D] = [this](){ ins_read<&cpu::op_ADC>(AbsoluteX); };
    instructionTable[0x79] = [this](){ ins_read<&cpu::op_ADC>(AbsoluteY); };
    instructionTable[0x61] = [this](){ ins_read<&cpu::op_ADC>(IndirectX); };
    instructionTable[0x71] = [this](){ ins_read<&cpu::op_ADC>(IndirectY); };
    instructionTable[0xE9] = [this](){ ins_read<&cpu::op_SBC>(Immediate); };
    instructionTable[0xE5] = [this](){ ins_read<&cpu::op_SBC>(ZeroPage); };
    instructionTable[0xF5] = [this](){ ins_read<&cpu::op_SBC>(ZeroPageX); };
    instructionTable[0xED] = [this](){ ins_read<&cpu::op_SBC>(Absolute); };
    instructionTable[0xFD] = [this](){ ins_read<&cpu::op_SBC>(AbsoluteX); };
    instructionTable[0xF9] = [this](){ ins_read<&cpu::op_SBC>(AbsoluteY); };
    instructionTable[0xE1] = [this](){ ins_read<&cpu::op_SBC>(IndirectX); };
    instructionTable[0xF1] = [this](){ ins_read<&cpu::op_SBC>(IndirectY); };

    // Compare
    instructionTable[0xC9] = [this](){ ins_read<&cpu::op_CMP>(Immediate); };
    instructionTable[0xC5] = [this](){ ins_read<&cpu::op_CMP>(ZeroPage); };
    instructionTable[0xD5] = [this](){ ins_read<&cpu::op_CMP>(ZeroPageX); };
    instructionTable[0xCD] = [this](){ ins_read<&cpu::op_CMP>(Absolute); };
    instructionTable[0xDD] = [this](){ ins_read<&cpu::op_CMP>(AbsoluteX); };
    instructionTable[0xD9] = [this](){ ins_read<&cpu::op_CMP>(AbsoluteY); };
    instructionTable[0xC1] = [this](){ ins_read<&cpu::op_CMP>(IndirectX); };
    instructionTable[0xD1] = [this](){ ins_read<&cpu::op_CMP>(IndirectY); };
    instructionTable[0xE0] = [this](){ ins_read<&cpu::op_CPX>(Immediate); };
    instructionTable[0xE4] = [this](){ ins_read<&cpu::op_CPX>(ZeroPage); };
    instructionTable[0xEC] = [this](){ ins_read<&cpu::op_CPX>(Absolute); };
    instructionTable[0xC0] = [this](){ ins_read<&cpu::op_CPY>(Immediate); };
    instructionTable[0xC4] = [this](){ ins_read<&cpu::op_CPY>(ZeroPage); };
    instructionTable[0xCC] = [this](){ ins_read<&cpu::op_CPY>(Absolute); };

    // Bitwise
    instructionTable[0x29] = [this](){ ins_read<&cpu::op_AND>(Immediate); };
    instructionTable[0x25] = [this](){ ins_read<&cpu::op_AND>(ZeroPage); };
    instructionTable[0x35] = [this](){ ins_read<&cpu::op_AND>(ZeroPageX); };
    instructionTable[0x2D] = [this](){ ins_read<&cpu::op_AND>(Absolute); };
    instructionTable[0x3D] = [this](){ ins_read<&cpu::op_AND>(AbsoluteX); };
    instructionTable[0x39] = [this](){ ins_read<&cpu::op_AND>(AbsoluteY); };
    instructionTable[0x21] = [this](){ ins_read<&cpu::op_AND>(IndirectX); };
    instructionTable[0x31] = [this](){ ins_read<&cpu::op_AND>(IndirectY); };
    instructionTable[0x49] = [this](){ ins_read<&cpu::op_EOR>(Immediate); };
    instructionTable[0x45] = [this](){ ins_read<&cpu::op_EOR>(ZeroPage); };
    instructionTable[0x55] = [this](){ ins_read<&cpu::op_EOR>(ZeroPageX); };
    instructionTable[0x4D] = [this](){ ins_read<&cpu::op_EOR>(Absolute); };
    instructionTable[0x5D] = [this](){ ins_read<&cpu::op_EOR>(AbsoluteX); };
    instructionTable[0x59] = [this](){ ins_read<&cpu::op_EOR>(AbsoluteY); };
    instructionTable[0x41] = [this](){ ins_read<&cpu::op_EOR>(IndirectX); };
    instructionTable[0x51] = [this](){ ins_read<&cpu::op_EOR>(IndirectY); };
    instructionTable[0x09] = [this](){ ins_read<&cpu::op_ORA>(Immediate); };
    instructionTable[0x05] = [this](){ ins_read<&cpu::op_ORA>(ZeroPage); };
    instructionTable[0x15] = [this](){ ins_read<&cpu::op_ORA>(ZeroPageX); };
    instructionTable[0x0D] = [this](){ ins_read<&cpu::op_ORA>(Absolute); };
    instructionTable[0x1D] = [this](){ ins_read<&cpu::op_ORA>(AbsoluteX); };
    instructionTable[0x19] = [this](){ ins_read<&cpu::op_ORA>(AbsoluteY); };
    instructionTable[0x01] = [this](){ ins_read<&cpu::op_ORA>(IndirectX); };
    instructionTable[0x11] = [this](){ ins_read<&cpu::op_ORA>(IndirectY); };
    instructionTable[0x24] = [this](){ ins_read<&cpu::op_BIT>(ZeroPage); };
    instructionTable[0x2C] = [this](){ ins_read<&cpu::op_BIT>(Absolute); };

    // Increment/Decrement
    instructionTable[0xE6] = [this](){ ins_rmw<&cpu::op_INC>(ZeroPage); };
    instructionTable[0xF6] = [this](){ ins_rmw<&cpu::op_INC>(ZeroPageX); };
    instructionTable[0xEE] = [this](){ ins_rmw<&cpu::op_INC>(Absolute); };
    instructionTable[0xFE] = [this](){ ins_rmw<&cpu::op_INC>(AbsoluteX); };
    instructionTable[0xE8] = [this](){ X++; setZN(X); };
    instructionTable[0xC8] = [this](){ Y++; setZN(Y); };
    instructionTable[0xC6] = [this](){ ins_rmw<&cpu::op_DEC>(ZeroPage); };
    instructionTable[0xD6] = [this](){ ins_rmw<&cpu::op_DEC>(ZeroPageX); };
    instructionTable[0xCE] = [this](){ ins_rmw<&cpu::op_DEC>(Absolute); };
    instructionTable[0xDE] = [this](){ ins_rmw<&cpu::op_DEC>(AbsoluteX); };
    instructionTable[0xCA] = [this](){ X--; setZN(X); };
    instructionTable[0x88] = [this](){ Y--; setZN(Y); };

    // Shift
    instructionTable[0x0A] = [this](){ ins_rmw<&cpu::op_ASL>(Accumulator); };
    instructionTable[0x06] = [this](){ ins_rmw<&cpu::op_ASL>(ZeroPage); };
    instructionTable[0x16] = [this](){ ins_rmw<&cpu::op_ASL>(ZeroPageX); };
    instructionTable[0x0E] = [this](){ ins_rmw<&cpu::op_ASL>(Absolute); };
    instructionTable[0x1E] = [this](){ ins_rmw<&cpu::op_ASL>(AbsoluteX); };
    instructionTable[0x4A] = [this](){ ins_rmw<&cpu::op_LSR>(Accumulator); };
    instructionTable[0x46] = [this](){ ins_rmw<&cpu::op_LSR>(ZeroPage); };
    instructionTable[0x56] = [this](){ ins_rmw<&cpu::op_LSR>(ZeroPageX); };
    instructionTable[0x4E] = [this](){ ins_rmw<&cpu::op_LSR>(Absolute); };
    instructionTable[0x5E] = [this](){ ins_rmw<&cpu::op_LSR>(AbsoluteX); };
    instructionTable[0x2A] = [this](){ ins_rmw<&cpu::op_ROL>(Accumulator); };
    instructionTable[0x26] = [this](){ ins_rmw<&cpu::op_ROL>(ZeroPage); };
    instructionTable[0x36] = [this](){ ins_rmw<&cpu::op_ROL>(ZeroPageX); };
    instructionTable[0x2E] = [this](){ ins_rmw<&cpu::op_ROL>(Absolute); };
    instructionTable[0x3E] = [this](){ ins_rmw<&cpu::op_ROL>(AbsoluteX); };
    instructionTable[0x6A] = [this](){ ins_rmw<&cpu::op_ROR>(Accumulator); };
    instructionTable[0x66] = [this](){ ins_rmw<&cpu::op_ROR>(ZeroPage); };
    instructionTable[0x76] = [this](){ ins_rmw<&cpu::op_ROR>(ZeroPageX); };
    instructionTable[0x6E] = [this](){ ins_rmw<&cpu::op_ROR>(Absolute); };
    instructionTable[0x7E] = [this](){ ins_rmw<&cpu::op_ROR>(AbsoluteX); };

    // Jump
    instructionTable[0x4C] = [this](){ PC = getAddress(Absolute); };
    instructionTable[0x6C] = [this](){ PC = getAddress(Indirect); };
    instructionTable[0x20] = [this](){ Word addr = getAddress(Absolute); Word returnAddr = PC - 1; write(0x0100 + SP--, (returnAddr >> 8) & 0xFF); write(0x0100 + SP--, returnAddr & 0xFF); PC = addr; };
    instructionTable[0x60] = [this](){ Byte lo = read(0x0100 + ++SP); Byte hi = read(0x0100 + ++SP); PC = (lo | (hi << 8)) + 1; };

    // Branch
    instructionTable[0x90] = [this](){ Byte offset = read(PC++); if (!getFlag(C)) PC += (int8_t)offset; };
    instructionTable[0xB0] = [this](){ Byte offset = read(PC++); if (getFlag(C)) PC += (int8_t)offset; };
    instructionTable[0xF0] = [this](){ Byte offset = read(PC++); if (getFlag(Z)) PC += (int8_t)offset; };
    instructionTable[0x30] = [this](){ Byte offset = read(PC++); if (getFlag(N)) PC += (int8_t)offset; };
    instructionTable[0xD0] = [this](){ Byte offset = read(PC++); if (!getFlag(Z)) PC += (int8_t)offset; };
    instructionTable[0x10] = [this](){ Byte offset = read(PC++); if (!getFlag(N)) PC += (int8_t)offset; };
    instructionTable[0x50] = [this](){ Byte offset = read(PC++); if (!getFlag(V)) PC += (int8_t)offset; };
    instructionTable[0x70] = [this](){ Byte offset = read(PC++); if (getFlag(V)) PC += (int8_t)offset; };

    // Flag
    instructionTable[0x18] = [this](){ setFlag(C, false); };
    instructionTable[0xD8] = [this](){ setFlag(D, false); };
    instructionTable[0x58] = [this](){ setFlag(I, false); };
    instructionTable[0xB8] = [this](){ setFlag(V, false); };
    instructionTable[0x38] = [this](){ setFlag(C, true); };
    instructionTable[0xF8] = [this](){ setFlag(D, true); };
    instructionTable[0x78] = [this](){ setFlag(I, true); };

    // System
    instructionTable[0x00] = [this](){ PC++; write(0x0100 + SP--, (PC >> 8) & 0xFF); write(0x0100 + SP--, PC & 0xFF); write(0x0100 + SP--, P | B | U); setFlag(B, true); PC = (read(0xFFFE) | (read(0xFFFF) << 8)); };
    instructionTable[0xEA] = [](){};
    instructionTable[0x40] = [this](){ P = read(0x0100 + ++SP); P &= ~B; P |= U; Byte lo = read(0x0100 + ++SP); Byte hi = read(0x0100 + ++SP); PC = (lo | (hi << 8)); };
}

void cpu::opcodeUnknown() {
    std::cerr << "Unknown opcode: 0x" << std::hex << (int)read(PC - 1) << " at PC: 0x" << std::hex << PC - 1 << std::endl;
}
