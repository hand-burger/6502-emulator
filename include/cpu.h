#ifndef CPU_H
#define CPU_H

#include <vector>
#include <functional>

using Word = unsigned short; // 16-bit word type
using Byte = unsigned char;  // 8-bit byte type

enum StatusFlags {
    C = 1 << 0, // Carry Flag
    Z = 1 << 1, // Zero Flag
    I = 1 << 2, // Interrupt Disable Flag
    D = 1 << 3, // Decimal Mode Flag
    B = 1 << 4, // Break Command Flag
    U = 1 << 5, // Unused Flag
    V = 1 << 6, // Overflow Flag
    N = 1 << 7  // Negative Flag
};

class cpu {
public:
    cpu();

    void reset();
    void loadAt0600AndSetReset(const std::vector<Byte>& program);
    void execute();

    Byte read(Word address) const;
    void write(Word address, Byte value);

    void setFlag(StatusFlags flag, bool value);
    bool getFlag(StatusFlags flag) const;

    Byte memory[256 * 256]{};
    Word PC;
    Byte SP;
    Byte A, X, Y;
    Byte P;

private:
    enum AddressingMode {
        Immediate, ZeroPage, ZeroPageX, ZeroPageY, Absolute, AbsoluteX, AbsoluteY, Indirect, IndirectX, IndirectY, Implied, Accumulator, Relative
    };

    std::function<void()> instructionTable[256];

    Word getAddress(AddressingMode mode);
    Byte fetch(AddressingMode mode);
    
    void setZN(Byte value);

    void op_ADC(Byte value);
    void op_SBC(Byte value);
    void op_AND(Byte value);
    void op_EOR(Byte value);
    void op_ORA(Byte value);
    void op_ASL(Byte& value);
    void op_LSR(Byte& value);
    void op_ROL(Byte& value);
    void op_ROR(Byte& value);
    void op_INC(Byte& value);
    void op_DEC(Byte& value);
    void op_CMP(Byte value);
    void op_CPX(Byte value);
    void op_CPY(Byte value);
    void op_BIT(Byte value);

    template <void (cpu::*op)(Byte)> 
    void ins_read(AddressingMode mode);

    template <void (cpu::*op)(Byte&)>
    void ins_rmw(AddressingMode mode);

    void initInstructionTable();
    void opcodeUnknown();
};

#endif // CPU_H
