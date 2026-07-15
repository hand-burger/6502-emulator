// Assert-based unit tests for the 6502 CPU core.
// Build and run with `make test`. No SDL required.

#include "cpu.h"
#include <cstdio>
#include <vector>

static int tests_run = 0;
static int tests_failed = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            std::printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);    \
            ++tests_failed;                                                  \
        }                                                                    \
    } while (0)

#define TEST(name) static void name()

#define RUN_TEST(name)                                                       \
    do {                                                                     \
        std::printf("%s\n", #name);                                          \
        ++tests_run;                                                         \
        name();                                                              \
    } while (0)

// Loads a program at 0x0600, resets, and executes `steps` instructions.
static cpu makeCpu(const std::vector<Byte>& program, int steps) {
    cpu c;
    c.loadAt0600AndSetReset(program);
    c.reset();
    for (int i = 0; i < steps; ++i) c.execute();
    return c;
}

TEST(lda_immediate_sets_value_and_flags) {
    cpu c = makeCpu({0xA9, 0x42}, 1); // LDA #$42
    CHECK(c.A == 0x42);
    CHECK(!c.getFlag(Z));
    CHECK(!c.getFlag(N));

    c = makeCpu({0xA9, 0x00}, 1); // LDA #$00
    CHECK(c.getFlag(Z));
    CHECK(!c.getFlag(N));

    c = makeCpu({0xA9, 0x80}, 1); // LDA #$80
    CHECK(!c.getFlag(Z));
    CHECK(c.getFlag(N));
}

TEST(adc_sets_carry_zero_and_overflow) {
    // CLC; LDA #$50; ADC #$50 -> 0xA0, V and N set (positive overflow)
    cpu c = makeCpu({0x18, 0xA9, 0x50, 0x69, 0x50}, 3);
    CHECK(c.A == 0xA0);
    CHECK(c.getFlag(V));
    CHECK(c.getFlag(N));
    CHECK(!c.getFlag(C));

    // CLC; LDA #$FF; ADC #$01 -> 0x00, C and Z set, no overflow
    c = makeCpu({0x18, 0xA9, 0xFF, 0x69, 0x01}, 3);
    CHECK(c.A == 0x00);
    CHECK(c.getFlag(C));
    CHECK(c.getFlag(Z));
    CHECK(!c.getFlag(V));
}

TEST(sbc_subtracts_with_borrow_semantics) {
    // SEC; LDA #$50; SBC #$30 -> 0x20, carry still set (no borrow)
    cpu c = makeCpu({0x38, 0xA9, 0x50, 0xE9, 0x30}, 3);
    CHECK(c.A == 0x20);
    CHECK(c.getFlag(C));

    // SEC; LDA #$30; SBC #$50 -> 0xE0, carry clear (borrow occurred)
    c = makeCpu({0x38, 0xA9, 0x30, 0xE9, 0x50}, 3);
    CHECK(c.A == 0xE0);
    CHECK(!c.getFlag(C));
    CHECK(c.getFlag(N));
}

TEST(zero_page_x_wraps_within_zero_page) {
    // LDX #$FF; LDA $80,X -> effective address ($80 + $FF) & 0xFF = $7F
    cpu c;
    c.loadAt0600AndSetReset({0xA2, 0xFF, 0xB5, 0x80});
    c.reset();
    c.write(0x007F, 0x99);
    c.execute();
    c.execute();
    CHECK(c.A == 0x99);
}

TEST(indirect_y_indexes_after_dereference) {
    // LDY #$04; LDA ($10),Y with ($10) -> $0300 reads $0304
    cpu c;
    c.loadAt0600AndSetReset({0xA0, 0x04, 0xB1, 0x10});
    c.reset();
    c.write(0x0010, 0x00);
    c.write(0x0011, 0x03);
    c.write(0x0304, 0x77);
    c.execute();
    c.execute();
    CHECK(c.A == 0x77);
}

TEST(jmp_indirect_reproduces_page_boundary_bug) {
    // JMP ($02FF): the 6502 fetches the high byte from $0200, not $0300.
    cpu c;
    c.loadAt0600AndSetReset({0x6C, 0xFF, 0x02});
    c.reset();
    c.write(0x02FF, 0x34);
    c.write(0x0200, 0x12); // buggy high byte source
    c.write(0x0300, 0x56); // would be used by a correct (65C02) fetch
    c.execute();
    CHECK(c.PC == 0x1234);
}

TEST(jsr_rts_round_trip) {
    // JSR $0605; ... target: LDA #$07; RTS -> execution resumes at 0x0603
    cpu c;
    c.loadAt0600AndSetReset({
        0x20, 0x05, 0x06, // 0600: JSR $0605
        0xA9, 0xFF,       // 0603: LDA #$FF (runs after RTS)
        0xA9, 0x07,       // 0605: LDA #$07
        0x60              // 0607: RTS
    });
    c.reset();
    c.execute(); // JSR
    CHECK(c.PC == 0x0605);
    c.execute(); // LDA #$07
    CHECK(c.A == 0x07);
    c.execute(); // RTS
    CHECK(c.PC == 0x0603);
    c.execute(); // LDA #$FF
    CHECK(c.A == 0xFF);
}

TEST(stack_push_pull_round_trip) {
    // LDA #$5A; PHA; LDA #$00; PLA -> A restored, SP back where it started
    cpu c;
    c.loadAt0600AndSetReset({0xA9, 0x5A, 0x48, 0xA9, 0x00, 0x68});
    c.reset();
    Byte sp0 = c.SP;
    for (int i = 0; i < 4; ++i) c.execute();
    CHECK(c.A == 0x5A);
    CHECK(c.SP == sp0);
    CHECK(!c.getFlag(Z));
}

TEST(inc_memory_wraps_and_sets_zero) {
    // INC $40 with $40 = $FF -> 0x00, Z set
    cpu c;
    c.loadAt0600AndSetReset({0xE6, 0x40});
    c.reset();
    c.write(0x0040, 0xFF);
    c.execute();
    CHECK(c.read(0x0040) == 0x00);
    CHECK(c.getFlag(Z));
}

TEST(branches_take_and_skip) {
    // LDA #$00; BEQ +2 (skips LDA #$01); LDA #$02
    cpu c = makeCpu({0xA9, 0x00, 0xF0, 0x02, 0xA9, 0x01, 0xA9, 0x02}, 3);
    CHECK(c.A == 0x02);

    // LDA #$01; BEQ +2 (not taken); LDA #$03
    c = makeCpu({0xA9, 0x01, 0xF0, 0x02, 0xA9, 0x03}, 3);
    CHECK(c.A == 0x03);
}

TEST(backward_branch_loops) {
    // LDX #$03; DEX; BNE -3 -> loops until X == 0
    cpu c;
    c.loadAt0600AndSetReset({0xA2, 0x03, 0xCA, 0xD0, 0xFD});
    c.reset();
    for (int i = 0; i < 7; ++i) c.execute();
    CHECK(c.X == 0x00);
    CHECK(c.PC == 0x0605);
}

TEST(cmp_sets_carry_and_zero) {
    // LDA #$40; CMP #$40 -> Z and C set
    cpu c = makeCpu({0xA9, 0x40, 0xC9, 0x40}, 2);
    CHECK(c.getFlag(Z));
    CHECK(c.getFlag(C));

    // LDA #$20; CMP #$40 -> borrow: C clear, N set
    c = makeCpu({0xA9, 0x20, 0xC9, 0x40}, 2);
    CHECK(!c.getFlag(Z));
    CHECK(!c.getFlag(C));
    CHECK(c.getFlag(N));
}

TEST(asl_accumulator_shifts_into_carry) {
    // LDA #$81; ASL A -> A = 0x02, C set
    cpu c = makeCpu({0xA9, 0x81, 0x0A}, 2);
    CHECK(c.A == 0x02);
    CHECK(c.getFlag(C));
}

TEST(ror_rotates_through_carry) {
    // SEC; LDA #$01; ROR A -> A = 0x80 (carry in), C set (bit 0 out)
    cpu c = makeCpu({0x38, 0xA9, 0x01, 0x6A}, 3);
    CHECK(c.A == 0x80);
    CHECK(c.getFlag(C));
    CHECK(c.getFlag(N));
}

TEST(brk_pushes_state_sets_i_and_jumps_to_irq_vector) {
    cpu c;
    c.loadAt0600AndSetReset({0x00}); // BRK at 0x0600
    c.reset();
    c.write(0xFFFE, 0x00);
    c.write(0xFFFF, 0x80); // IRQ vector -> $8000
    c.setFlag(I, false);
    Byte sp0 = c.SP;
    c.execute();

    CHECK(c.PC == 0x8000);
    CHECK(c.SP == sp0 - 3);
    // Pushed return address is PC of BRK + 2
    CHECK(c.read(0x0100 + sp0) == 0x06);     // high byte
    CHECK(c.read(0x0100 + sp0 - 1) == 0x02); // low byte
    // Pushed status has B set, and BRK sets the interrupt disable flag
    CHECK((c.read(0x0100 + sp0 - 2) & B) != 0);
    CHECK(c.getFlag(I));
}

TEST(rti_restores_status_and_pc) {
    cpu c;
    c.loadAt0600AndSetReset({0x40}); // RTI at 0x0600
    c.reset();
    // Hand-build an interrupt frame: status, then return address $1234
    Byte sp0 = c.SP;
    c.write(0x0100 + sp0, 0x12);       // PC high
    c.write(0x0100 + sp0 - 1, 0x34);   // PC low
    c.write(0x0100 + sp0 - 2, C | Z);  // status to restore
    c.SP = sp0 - 3;
    c.execute();

    CHECK(c.PC == 0x1234);
    CHECK(c.getFlag(C));
    CHECK(c.getFlag(Z));
    CHECK(!c.getFlag(B));
    CHECK(c.SP == sp0);
}

int main() {
    RUN_TEST(lda_immediate_sets_value_and_flags);
    RUN_TEST(adc_sets_carry_zero_and_overflow);
    RUN_TEST(sbc_subtracts_with_borrow_semantics);
    RUN_TEST(zero_page_x_wraps_within_zero_page);
    RUN_TEST(indirect_y_indexes_after_dereference);
    RUN_TEST(jmp_indirect_reproduces_page_boundary_bug);
    RUN_TEST(jsr_rts_round_trip);
    RUN_TEST(stack_push_pull_round_trip);
    RUN_TEST(inc_memory_wraps_and_sets_zero);
    RUN_TEST(branches_take_and_skip);
    RUN_TEST(backward_branch_loops);
    RUN_TEST(cmp_sets_carry_and_zero);
    RUN_TEST(asl_accumulator_shifts_into_carry);
    RUN_TEST(ror_rotates_through_carry);
    RUN_TEST(brk_pushes_state_sets_i_and_jumps_to_irq_vector);
    RUN_TEST(rti_restores_status_and_pc);

    if (tests_failed == 0) {
        std::printf("All %d tests passed.\n", tests_run);
        return 0;
    }
    std::printf("%d of %d tests had failures.\n", tests_failed, tests_run);
    return 1;
}
