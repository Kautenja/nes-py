//  Program:      nes-py
//  File:         test_cpu.cpp
//  Description:  Catch2 CPU characterization tests
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include <catch2/catch_test_macros.hpp>
#include "nes_emu/test/nes_emu/support/test_mappers.hpp"

namespace {

int run_ready_instruction(NES::CPU& cpu, NES::MainBus& bus) {
    REQUIRE(cpu.can_execute_instruction());
    int cycles = cpu.execute_instruction(bus);
    cpu.consume_pending_cycles(cycles - 1);
    return cycles;
}

bool reset_vector_starts_program_counter() {
    NESTest::ProgramTestMapper mapper;
    NES::MainBus bus;
    NES::CPU cpu;
    mapper.setResetVector(0x8123);
    mapper.load(0x8123, {
        0xa9, 0x42,       // LDA #$42
        0x85, 0x01,       // STA $01
        0x4c, 0x27, 0x81  // JMP $8127
    });
    bus.set_mapper(&mapper);
    cpu.reset(bus);
    NESTest::run_cpu_cycles(cpu, bus, 24);
    return bus.read(0x0001) == 0x42;
}

bool stack_push_pop_and_status_round_trip() {
    NESTest::ProgramTestMapper mapper;
    NES::MainBus bus;
    NES::CPU cpu;
    mapper.load(0x8000, {
        0xa9, 0x5a,       // LDA #$5a
        0x48,             // PHA
        0xa9, 0x00,       // LDA #$00
        0x68,             // PLA
        0x85, 0x02,       // STA $02
        0x38,             // SEC
        0x08,             // PHP
        0x18,             // CLC
        0x28,             // PLP
        0xb0, 0x07,       // BCS success
        0xa9, 0x00,       // fail: LDA #$00
        0x85, 0x03,       // STA $03
        0x4c, 0x1c, 0x80, // JMP end
        0xa9, 0x01,       // success: LDA #$01
        0x85, 0x03,       // STA $03
        0x4c, 0x1c, 0x80  // end: JMP end
    });
    bus.set_mapper(&mapper);
    cpu.reset(bus);
    NESTest::run_cpu_cycles(cpu, bus, 96);
    return bus.read(0x0002) == 0x5a && bus.read(0x0003) == 0x01;
}

bool addressing_modes_resolve_expected_memory() {
    NESTest::ProgramTestMapper mapper;
    NES::MainBus bus;
    NES::CPU cpu;
    bus.write(0x0010, 0x77);
    bus.write(0x0203, 0x88);
    bus.write(0x0334, 0x99);
    bus.write(0x0024, 0x03);
    bus.write(0x0025, 0x04);
    bus.write(0x0406, 0xaa);
    bus.write(0x0034, 0x00);
    bus.write(0x0035, 0x05);
    bus.write(0x0500, 0xbb);
    mapper.load(0x8000, {
        0xa2, 0x04,       // LDX #$04
        0xa0, 0x03,       // LDY #$03
        0xb5, 0x0c,       // LDA $0c,X -> $10
        0x85, 0x20,       // STA $20
        0xb9, 0x00, 0x02, // LDA $0200,Y -> $0203
        0x85, 0x21,       // STA $21
        0xbd, 0x30, 0x03, // LDA $0330,X -> $0334
        0x85, 0x22,       // STA $22
        0xb1, 0x24,       // LDA ($24),Y -> $0406
        0x85, 0x23,       // STA $23
        0xa1, 0x30,       // LDA ($30,X) -> ($34) -> $0500
        0x85, 0x26,       // STA $26
        0x4c, 0x1c, 0x80  // JMP end
    });
    bus.set_mapper(&mapper);
    cpu.reset(bus);
    NESTest::run_cpu_cycles(cpu, bus, 180);
    return (
        bus.read(0x0020) == 0x77 &&
        bus.read(0x0021) == 0x88 &&
        bus.read(0x0022) == 0x99 &&
        bus.read(0x0023) == 0xaa &&
        bus.read(0x0026) == 0xbb
    );
}

bool instruction_api_reports_representative_cycle_counts() {
    NESTest::ProgramTestMapper mapper;
    NES::MainBus bus;
    NES::CPU cpu;
    bus.write(0x0010, 0x33);
    bus.write(0x0200, 0x44);
    bus.write(0x0201, 0x55);
    mapper.load(0x8000, {
        0xa2, 0x01,       // LDX #$01 -> 2
        0xa9, 0x7f,       // LDA #$7f -> 2
        0x85, 0x10,       // STA $10 -> 3
        0xbd, 0xff, 0x01, // LDA $01ff,X -> page cross -> 5
        0xbd, 0x00, 0x02, // LDA $0200,X -> no page cross -> 4
        0xf0, 0x01,       // BEQ not taken -> 2
        0xea,             // NOP -> 2
        0xa9, 0x00,       // LDA #$00 -> 2
        0xf0, 0x01,       // BEQ taken same page -> 3
        0xea,             // skipped NOP
        0x48,             // PHA -> 3
        0x68,             // PLA -> 4
        0x20, 0x20, 0x80, // JSR $8020 -> 6
        0x4c, 0x1c, 0x80  // JMP end -> 3
    });
    mapper.load(0x8020, {
        0x60              // RTS -> 6
    });
    bus.set_mapper(&mapper);
    cpu.reset(bus);

    return (
        run_ready_instruction(cpu, bus) == 2 &&
        run_ready_instruction(cpu, bus) == 2 &&
        run_ready_instruction(cpu, bus) == 3 &&
        run_ready_instruction(cpu, bus) == 5 &&
        run_ready_instruction(cpu, bus) == 4 &&
        run_ready_instruction(cpu, bus) == 2 &&
        run_ready_instruction(cpu, bus) == 2 &&
        run_ready_instruction(cpu, bus) == 2 &&
        run_ready_instruction(cpu, bus) == 3 &&
        run_ready_instruction(cpu, bus) == 3 &&
        run_ready_instruction(cpu, bus) == 4 &&
        run_ready_instruction(cpu, bus) == 6 &&
        run_ready_instruction(cpu, bus) == 6 &&
        bus.read(0x0010) == 0x7f
    );
}

bool instruction_api_reports_branch_page_cross_penalty() {
    NESTest::ProgramTestMapper mapper;
    NES::MainBus bus;
    NES::CPU cpu;
    mapper.setResetVector(0x80fa);
    mapper.load(0x80fa, {
        0xa9, 0x00,       // LDA #$00 -> set Z
        0xf0, 0x05,       // BEQ $8103, crossing $80xx to $81xx
        0xea,
        0xea,
        0xea,
        0xea,
        0xea
    });
    mapper.load(0x8103, {
        0xa9, 0x01        // LDA #$01
    });
    bus.set_mapper(&mapper);
    cpu.reset(bus);

    return (
        run_ready_instruction(cpu, bus) == 2 &&
        run_ready_instruction(cpu, bus) == 5 &&
        run_ready_instruction(cpu, bus) == 2
    );
}

bool instruction_api_consumes_interrupt_entry_stalls() {
    NESTest::IRQTestMapper mapper;
    NES::MainBus bus;
    NES::CPU cpu;
    bus.set_mapper(&mapper);
    mapper.setIRQCallback([&]() {
        cpu.interrupt(bus, NES::CPU::IRQ_INTERRUPT);
    });
    cpu.reset(bus);
    REQUIRE(run_ready_instruction(cpu, bus) == 2);  // CLI

    mapper.triggerIRQ();
    int stall_cycles = 0;
    while (!cpu.can_execute_instruction())
        stall_cycles += cpu.execute_instruction(bus);

    int handler_cycles = run_ready_instruction(cpu, bus);
    int store_cycles = run_ready_instruction(cpu, bus);
    return (
        stall_cycles == 7 &&
        handler_cycles == 2 &&
        store_cycles == 3 &&
        bus.read(0x0002) == 0x42
    );
}

bool instruction_api_reports_dma_stalls() {
    NESTest::ProgramTestMapper mapper;
    NES::MainBus bus;
    NES::PictureBus picture_bus;
    NES::PPU ppu;
    NES::CPU cpu;
    NES::Controller controllers[2];
    mapper.load(0x8000, {
        0xa9, 0x02,       // LDA #$02
        0x8d, 0x14, 0x40, // STA $4014
        0xa9, 0x55,       // LDA #$55
        0x85, 0x06        // STA $06
    });
    bus.set_mapper(&mapper);
    picture_bus.set_mapper(&mapper);
    ppu.reset();
    bus.connect_devices(&cpu, &ppu, &picture_bus, controllers);
    bus.write(0x0200, 0xcc);
    cpu.reset(bus);

    REQUIRE(run_ready_instruction(cpu, bus) == 2);
    int dma_instruction_cycles = cpu.execute_instruction(bus);
    bool has_dma_stall = dma_instruction_cycles == 518;
    cpu.consume_pending_cycles(40);
    bool still_stalled = !cpu.can_execute_instruction();
    cpu.consume_pending_cycles(dma_instruction_cycles - 1 - 40);
    bool ready_after_dma = cpu.can_execute_instruction();
    int load_cycles = run_ready_instruction(cpu, bus);
    int store_cycles = run_ready_instruction(cpu, bus);
    bus.write(NES::OAMADDR, 0x00);

    return (
        has_dma_stall &&
        still_stalled &&
        ready_after_dma &&
        load_cycles == 2 &&
        store_cycles == 3 &&
        bus.read(0x0006) == 0x55 &&
        bus.read(NES::OAMDATA) == 0xcc
    );
}

bool branch_page_crossing_reaches_target() {
    NESTest::ProgramTestMapper mapper;
    NES::MainBus bus;
    NES::CPU cpu;
    mapper.setResetVector(0x80fa);
    mapper.load(0x80fa, {
        0xa9, 0x00,       // LDA #$00
        0xf0, 0x05,       // BEQ $8103, crossing the $80xx page
        0xa9, 0x00,       // fail: LDA #$00
        0x85, 0x04,       // STA $04
        0xea              // NOP filler before target
    });
    mapper.load(0x8103, {
        0xa9, 0x01,       // success: LDA #$01
        0x85, 0x04,       // STA $04
        0x4c, 0x08, 0x81  // JMP end
    });
    bus.set_mapper(&mapper);
    cpu.reset(bus);
    NESTest::run_cpu_cycles(cpu, bus, 72);
    return bus.read(0x0004) == 0x01;
}

bool mapper_irq_enters_interrupt_handler() {
    NESTest::IRQTestMapper mapper;
    NES::MainBus bus;
    NES::CPU cpu;
    bus.set_mapper(&mapper);
    mapper.setIRQCallback([&]() {
        cpu.interrupt(bus, NES::CPU::IRQ_INTERRUPT);
    });
    cpu.reset(bus);
    for (int index = 0; index < 4; ++index)
        cpu.cycle(bus);
    mapper.triggerIRQ();
    for (int index = 0; index < 32; ++index)
        cpu.cycle(bus);
    return bus.read(0x0002) == 0x42;
}

bool oam_dma_stalls_cpu_before_next_instruction() {
    NESTest::ProgramTestMapper mapper;
    NES::MainBus bus;
    NES::PictureBus picture_bus;
    NES::PPU ppu;
    NES::CPU cpu;
    NES::Controller controllers[2];
    mapper.load(0x8000, {
        0xa9, 0x02,       // LDA #$02
        0x8d, 0x14, 0x40, // STA $4014
        0xa9, 0x55,       // LDA #$55
        0x85, 0x06,       // STA $06
        0x4c, 0x09, 0x80  // JMP end
    });
    bus.set_mapper(&mapper);
    picture_bus.set_mapper(&mapper);
    ppu.reset();
    bus.connect_devices(&cpu, &ppu, &picture_bus, controllers);
    bus.write(0x0200, 0xcc);
    cpu.reset(bus);
    NESTest::run_cpu_cycles(cpu, bus, 40);
    bool stalled = bus.read(0x0006) == 0x00;
    NESTest::run_cpu_cycles(cpu, bus, 620);
    bus.write(NES::OAMADDR, 0x00);
    return (
        stalled &&
        bus.read(0x0006) == 0x55 &&
        bus.read(NES::OAMDATA) == 0xcc
    );
}

bool status_flags_match_zero_negative_overflow_and_carry_behavior() {
    NESTest::ProgramTestMapper mapper;
    NES::MainBus bus;
    NES::CPU cpu;
    mapper.load(0x8000, {
        0xa9, 0x00,       // LDA #$00
        0x08,             // PHP
        0x68,             // PLA
        0x85, 0x10,       // STA $10
        0xa9, 0x80,       // LDA #$80
        0x08,             // PHP
        0x68,             // PLA
        0x85, 0x11,       // STA $11
        0x18,             // CLC
        0xa9, 0x7f,       // LDA #$7f
        0x69, 0x01,       // ADC #$01
        0x08,             // PHP
        0x68,             // PLA
        0x85, 0x12,       // STA $12
        0x38,             // SEC
        0xa9, 0x01,       // LDA #$01
        0xe9, 0x01,       // SBC #$01
        0x08,             // PHP
        0x68,             // PLA
        0x85, 0x13,       // STA $13
        0x4c, 0x25, 0x80  // JMP end
    });
    bus.set_mapper(&mapper);
    cpu.reset(bus);
    NESTest::run_cpu_cycles(cpu, bus, 180);
    NES::NES_Byte zero_status = bus.read(0x0010);
    NES::NES_Byte negative_status = bus.read(0x0011);
    NES::NES_Byte overflow_status = bus.read(0x0012);
    NES::NES_Byte subtract_status = bus.read(0x0013);
    return (
        (zero_status & NES::CPU_Flags::FLAG_ZERO) &&
        !(zero_status & NES::CPU_Flags::FLAG_NEGATIVE) &&
        (negative_status & NES::CPU_Flags::FLAG_NEGATIVE) &&
        !(negative_status & NES::CPU_Flags::FLAG_ZERO) &&
        (overflow_status & NES::CPU_Flags::FLAG_OVERFLOW) &&
        (overflow_status & NES::CPU_Flags::FLAG_NEGATIVE) &&
        (subtract_status & NES::CPU_Flags::FLAG_CARRY) &&
        (subtract_status & NES::CPU_Flags::FLAG_ZERO)
    );
}

}  // namespace

TEST_CASE("CPU reset, addressing, interrupts, and flags are stable", "[cpu]") {
    REQUIRE(reset_vector_starts_program_counter());
    REQUIRE(stack_push_pop_and_status_round_trip());
    REQUIRE(addressing_modes_resolve_expected_memory());
    REQUIRE(branch_page_crossing_reaches_target());
    REQUIRE(mapper_irq_enters_interrupt_handler());
    REQUIRE(oam_dma_stalls_cpu_before_next_instruction());
    REQUIRE(status_flags_match_zero_negative_overflow_and_carry_behavior());
}

TEST_CASE(
    "CPU instruction-level API reports cycles and preserves stalls",
    "[cpu][batching]"
) {
    REQUIRE(instruction_api_reports_representative_cycle_counts());
    REQUIRE(instruction_api_reports_branch_page_cross_penalty());
    REQUIRE(instruction_api_consumes_interrupt_entry_stalls());
    REQUIRE(instruction_api_reports_dma_stalls());
}
