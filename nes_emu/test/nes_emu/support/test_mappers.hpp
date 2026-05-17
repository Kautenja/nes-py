//  Program:      nes-py
//  File:         test_mappers.hpp
//  Description:  Native-only mapper fixtures for Catch2 tests and benchmarks
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef NES_EMU_TEST_SUPPORT_TEST_MAPPERS_HPP
#define NES_EMU_TEST_SUPPORT_TEST_MAPPERS_HPP

#include <cstddef>
#include <initializer_list>
#include <memory>
#include <vector>
#include "nes_emu/common.hpp"
#include "nes_emu/controller.hpp"
#include "nes_emu/cpu.hpp"
#include "nes_emu/main_bus.hpp"
#include "nes_emu/mapper.hpp"
#include "nes_emu/picture_bus.hpp"
#include "nes_emu/ppu.hpp"

namespace NESTest {

inline void fill_bank_markers(
    std::vector<NES::NES_Byte>& memory,
    std::size_t bank_size,
    NES::NES_Byte first_marker
) {
    for (std::size_t index = 0; index < memory.size(); ++index)
        memory[index] = first_marker + (index / bank_size);
}

class ProgramTestMapper : public NES::Mapper {
 private:
    std::vector<NES::NES_Byte> prg;
    std::vector<NES::NES_Byte> chr;
    NES::NES_Address last_prg_write_address;
    NES::NES_Byte last_prg_write_value;

 public:
    ProgramTestMapper() :
        NES::Mapper(nullptr),
        prg(0x8000, 0xea),
        chr(0x2000, 0),
        last_prg_write_address(0),
        last_prg_write_value(0) {
        setResetVector(0x8000);
        setIRQVector(0x9000);
        setNMIVector(0x9000);
        resizePRGRAM(0x2000);
    }

    inline std::unique_ptr<NES::Mapper> clone() const {
        return std::unique_ptr<NES::Mapper>(new ProgramTestMapper(*this));
    }

    inline void setByte(NES::NES_Address address, NES::NES_Byte value) {
        prg[(address - 0x8000) & 0x7fff] = value;
    }

    inline void load(
        NES::NES_Address address,
        std::initializer_list<NES::NES_Byte> bytes
    ) {
        for (auto value : bytes)
            setByte(address++, value);
    }

    inline void setResetVector(NES::NES_Address address) {
        setByte(0xfffc, static_cast<NES::NES_Byte>(address));
        setByte(0xfffd, static_cast<NES::NES_Byte>(address >> 8));
    }

    inline void setIRQVector(NES::NES_Address address) {
        setByte(0xfffe, static_cast<NES::NES_Byte>(address));
        setByte(0xffff, static_cast<NES::NES_Byte>(address >> 8));
    }

    inline void setNMIVector(NES::NES_Address address) {
        setByte(0xfffa, static_cast<NES::NES_Byte>(address));
        setByte(0xfffb, static_cast<NES::NES_Byte>(address >> 8));
    }

    inline NES::NES_Address lastPRGWriteAddress() const {
        return last_prg_write_address;
    }

    inline NES::NES_Byte lastPRGWriteValue() const {
        return last_prg_write_value;
    }

    inline NES::NES_Byte readPRG(NES::NES_Address address) {
        return prg[(address - 0x8000) & 0x7fff];
    }

    inline void writePRG(NES::NES_Address address, NES::NES_Byte value) {
        last_prg_write_address = address;
        last_prg_write_value = value;
    }

    inline NES::NES_Byte readCHR(NES::NES_Address address) {
        return chr[address & 0x1fff];
    }

    inline void writeCHR(NES::NES_Address address, NES::NES_Byte value) {
        chr[address & 0x1fff] = value;
    }
};

inline void run_cpu_cycles(NES::CPU& cpu, NES::MainBus& bus, int cycles) {
    for (int index = 0; index < cycles; ++index)
        cpu.cycle(bus);
}

class IRQTestMapper : public NES::Mapper {
 private:
    std::vector<NES::NES_Byte> prg;

 public:
    IRQTestMapper() : NES::Mapper(nullptr), prg(0x8000, 0xea) {
        prg[0x0000] = 0x58;  // CLI
        prg[0x1000] = 0xa9;  // LDA #$42
        prg[0x1001] = 0x42;
        prg[0x1002] = 0x85;  // STA $02
        prg[0x1003] = 0x02;
        prg[0x7ffc] = 0x00;  // reset vector = $8000
        prg[0x7ffd] = 0x80;
        prg[0x7ffe] = 0x00;  // IRQ vector = $9000
        prg[0x7fff] = 0x90;
    }

    inline std::unique_ptr<NES::Mapper> clone() const {
        return std::unique_ptr<NES::Mapper>(new IRQTestMapper(*this));
    }

    inline NES::NES_Byte readPRG(NES::NES_Address address) {
        return prg[(address - 0x8000) & 0x7fff];
    }

    inline void writePRG(NES::NES_Address address, NES::NES_Byte value) {
        (void) address;
        (void) value;
    }

    inline NES::NES_Byte readCHR(NES::NES_Address address) {
        (void) address;
        return 0;
    }

    inline void writeCHR(NES::NES_Address address, NES::NES_Byte value) {
        (void) address;
        (void) value;
    }

    inline void triggerIRQ() { requestIRQ(); }
};

class CPUCycleHookMapper : public NES::Mapper {
 public:
    int cpu_cycles;

    CPUCycleHookMapper() : NES::Mapper(nullptr), cpu_cycles(0) { }

    inline std::unique_ptr<NES::Mapper> clone() const {
        return std::unique_ptr<NES::Mapper>(new CPUCycleHookMapper(*this));
    }

    inline NES::NES_Byte readPRG(NES::NES_Address address) {
        (void) address;
        return 0;
    }

    inline void writePRG(NES::NES_Address address, NES::NES_Byte value) {
        (void) address;
        (void) value;
    }

    inline NES::NES_Byte readCHR(NES::NES_Address address) {
        (void) address;
        return 0;
    }

    inline void writeCHR(NES::NES_Address address, NES::NES_Byte value) {
        (void) address;
        (void) value;
    }

    inline void onCPUCycle() { ++cpu_cycles; }

    inline bool observesCPUCycles() const { return true; }
};

class PPUHookMapper : public NES::Mapper {
 public:
    int address_observations;
    int read_observations;
    int write_observations;
    NES::NES_Address last_address;
    NES::NES_Byte last_value;
    std::vector<NES::NES_Address> address_sequence;

    PPUHookMapper() :
        NES::Mapper(nullptr),
        address_observations(0),
        read_observations(0),
        write_observations(0),
        last_address(0),
        last_value(0),
        address_sequence() { }

    inline std::unique_ptr<NES::Mapper> clone() const {
        return std::unique_ptr<NES::Mapper>(new PPUHookMapper(*this));
    }

    inline NES::NES_Byte readPRG(NES::NES_Address address) {
        (void) address;
        return 0;
    }

    inline void writePRG(NES::NES_Address address, NES::NES_Byte value) {
        (void) address;
        (void) value;
    }

    inline NES::NES_Byte readCHR(NES::NES_Address address) {
        (void) address;
        return 0x33;
    }

    inline void writeCHR(NES::NES_Address address, NES::NES_Byte value) {
        (void) address;
        last_value = value;
    }

    inline void onPPUAddress(NES::NES_Address address) {
        ++address_observations;
        last_address = address;
        address_sequence.push_back(address);
    }

    inline bool observesPPUAddresses() const { return true; }

    inline void onPPURead(NES::NES_Address address, NES::NES_Byte value) {
        (void) address;
        ++read_observations;
        last_value = value;
    }

    inline bool observesPPUReads() const { return true; }

    inline void onPPUWrite(NES::NES_Address address, NES::NES_Byte value) {
        (void) address;
        ++write_observations;
        last_value = value;
    }

    inline bool observesPPUWrites() const { return true; }
};

class ExpansionTestMapper : public NES::Mapper {
 private:
    NES::NES_Byte expansion[0x1fe0];

 public:
    ExpansionTestMapper() : NES::Mapper(nullptr), expansion() { }

    inline std::unique_ptr<NES::Mapper> clone() const {
        return std::unique_ptr<NES::Mapper>(new ExpansionTestMapper(*this));
    }

    inline NES::NES_Byte readPRG(NES::NES_Address address) {
        (void) address;
        return 0;
    }

    inline void writePRG(NES::NES_Address address, NES::NES_Byte value) {
        (void) address;
        (void) value;
    }

    inline NES::NES_Byte readCHR(NES::NES_Address address) {
        (void) address;
        return 0;
    }

    inline void writeCHR(NES::NES_Address address, NES::NES_Byte value) {
        (void) address;
        (void) value;
    }

    inline bool handlesExpansion(NES::NES_Address address) const {
        return address >= 0x4020 && address < 0x6000;
    }

    inline NES::NES_Byte readExpansion(NES::NES_Address address) {
        return expansion[address - 0x4020];
    }

    inline void writeExpansion(
        NES::NES_Address address,
        NES::NES_Byte value
    ) {
        expansion[address - 0x4020] = value;
    }
};

class PRGRAMTestMapper : public NES::Mapper {
 private:
    std::size_t active_bank;

 public:
    PRGRAMTestMapper() : NES::Mapper(nullptr), active_bank(0) {
        resizePRGRAM(0x4000);
    }

    inline std::unique_ptr<NES::Mapper> clone() const {
        return std::unique_ptr<NES::Mapper>(new PRGRAMTestMapper(*this));
    }

    inline NES::NES_Byte readPRG(NES::NES_Address address) {
        (void) address;
        return 0;
    }

    inline void writePRG(NES::NES_Address address, NES::NES_Byte value) {
        (void) address;
        active_bank = value & 1;
    }

    inline NES::NES_Byte readCHR(NES::NES_Address address) {
        (void) address;
        return 0;
    }

    inline void writeCHR(NES::NES_Address address, NES::NES_Byte value) {
        (void) address;
        (void) value;
    }

    inline NES::NES_Byte readPRGRAM(NES::NES_Address address) {
        std::size_t offset = active_bank * 0x2000 + (address - 0x6000);
        return prg_ram[offset];
    }

    inline void writePRGRAM(NES::NES_Address address, NES::NES_Byte value) {
        std::size_t offset = active_bank * 0x2000 + (address - 0x6000);
        if (prg_ram_writable)
            prg_ram[offset] = value;
    }

    inline const NES::NES_Byte* getPRGRAMPointer(NES::NES_Address address) {
        std::size_t offset = active_bank * 0x2000 + (address - 0x6000);
        return &prg_ram[offset];
    }

    inline void protectPRGRAM() { setPRGRAMWritable(false); }
};

class NameTableTestMapper : public NES::Mapper {
 private:
    std::vector<NES::NES_Byte> name_table;

 public:
    NameTableTestMapper() : NES::Mapper(nullptr), name_table(0x1000, 0) { }

    inline std::unique_ptr<NES::Mapper> clone() const {
        return std::unique_ptr<NES::Mapper>(new NameTableTestMapper(*this));
    }

    inline NES::NES_Byte readPRG(NES::NES_Address address) {
        (void) address;
        return 0;
    }

    inline void writePRG(NES::NES_Address address, NES::NES_Byte value) {
        (void) address;
        (void) value;
    }

    inline NES::NES_Byte readCHR(NES::NES_Address address) {
        (void) address;
        return 0;
    }

    inline void writeCHR(NES::NES_Address address, NES::NES_Byte value) {
        (void) address;
        (void) value;
    }

    inline bool mapsNameTable(NES::NES_Address address) const {
        return address >= 0x2000 && address < 0x3f00;
    }

    inline bool hasNameTableMapping() const { return true; }

    inline NES::NES_Byte readNameTable(NES::NES_Address address) {
        return name_table[address & 0x0fff];
    }

    inline void writeNameTable(
        NES::NES_Address address,
        NES::NES_Byte value
    ) {
        name_table[address & 0x0fff] = value;
    }
};

class PictureBusTestMapper : public NES::Mapper {
 private:
    std::vector<NES::NES_Byte> chr;

 public:
    explicit PictureBusTestMapper(
        NES::NameTableMirroring mirroring = NES::HORIZONTAL
    ) :
        NES::Mapper(nullptr),
        chr(0x2000, 0) {
        setNameTableMirroring(mirroring);
    }

    inline std::unique_ptr<NES::Mapper> clone() const {
        return std::unique_ptr<NES::Mapper>(new PictureBusTestMapper(*this));
    }

    inline NES::NES_Byte readPRG(NES::NES_Address address) {
        (void) address;
        return 0;
    }

    inline void writePRG(NES::NES_Address address, NES::NES_Byte value) {
        (void) address;
        (void) value;
    }

    inline NES::NES_Byte readCHR(NES::NES_Address address) {
        return chr[address & 0x1fff];
    }

    inline void writeCHR(NES::NES_Address address, NES::NES_Byte value) {
        chr[address & 0x1fff] = value;
    }
};

}  // namespace NESTest

#endif  // NES_EMU_TEST_SUPPORT_TEST_MAPPERS_HPP
