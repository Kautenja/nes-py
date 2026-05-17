//  Program:      nes-py
//  File:         lib_nes_env.cpp
//  Description:  file describes the outward facing ctypes API for Python
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include <cstdint>
#include <exception>
#include <memory>
#include <string>
#include <vector>
#include "common.hpp"
#include "cartridge.hpp"
#include "cpu.hpp"
#include "emulator.hpp"
#include "main_bus.hpp"
#include "mapper_bank.hpp"
#include "mapper_factory.hpp"
#include "picture_bus.hpp"

// Windows-base systems
#if defined(_WIN32) || defined(WIN32) || defined(__CYGWIN__) || defined(__MINGW32__) || defined(__BORLANDC__)
    // setup the module initializer. required to link visual studio C++ ctypes
    void PyInit_lib_nes_env() { }
    // setup the function modifier to export in the DLL
    #define EXP __declspec(dllexport)
// Unix-like systems
#else
    // setup the modifier as a dummy
    #define EXP
#endif

namespace {

std::string ROMPath(wchar_t* path) {
    std::wstring ws_rom_path(path);
    return std::string(ws_rom_path.begin(), ws_rom_path.end());
}

NES::Cartridge LoadCartridgeMetadata(wchar_t* path) {
    NES::Cartridge cartridge;
    cartridge.loadFromFile(ROMPath(path), true);
    return cartridge;
}

thread_local std::string cartridge_error;

const unsigned int BANK_HELPER_PRG_8K = 1u << 0;
const unsigned int BANK_HELPER_PRG_16K = 1u << 1;
const unsigned int BANK_HELPER_PRG_32K = 1u << 2;
const unsigned int BANK_HELPER_CHR_1K = 1u << 3;
const unsigned int BANK_HELPER_CHR_2K = 1u << 4;
const unsigned int BANK_HELPER_CHR_4K = 1u << 5;
const unsigned int BANK_HELPER_CHR_8K = 1u << 6;
const unsigned int BANK_HELPER_MASKS = 1u << 7;

void FillBankMarkers(
    std::vector<NES::NES_Byte>& memory,
    std::size_t bank_size,
    NES::NES_Byte first_marker
) {
    for (std::size_t index = 0; index < memory.size(); ++index)
        memory[index] = first_marker + (index / bank_size);
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

    PPUHookMapper() :
        NES::Mapper(nullptr),
        address_observations(0),
        read_observations(0),
        write_observations(0),
        last_address(0),
        last_value(0) { }

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

bool RunMapperIRQSmokeTest() {
    IRQTestMapper mapper;
    NES::MainBus bus;
    NES::CPU cpu;
    bus.set_mapper(&mapper);
    mapper.setIRQCallback([&]() { cpu.interrupt(bus, NES::CPU::IRQ_INTERRUPT); });
    cpu.reset(bus);
    for (int i = 0; i < 4; ++i)
        cpu.cycle(bus);
    mapper.triggerIRQ();
    for (int i = 0; i < 32; ++i)
        cpu.cycle(bus);
    return bus.read(0x0002) == 0x42;
}

bool RunMapperCPUCycleHookSmokeTest() {
    CPUCycleHookMapper mapper;
    mapper.onCPUCycle();
    mapper.onCPUCycle();
    mapper.onCPUCycle();
    return mapper.cpu_cycles == 3;
}

bool RunMapperPPUHookSmokeTest() {
    PPUHookMapper mapper;
    NES::PictureBus bus;
    bus.set_mapper(&mapper);
    NES::NES_Byte value = bus.read(0x0123);
    bus.write(0x0456, 0x77);
    return (
        value == 0x33 &&
        mapper.address_observations == 2 &&
        mapper.read_observations == 1 &&
        mapper.write_observations == 1 &&
        mapper.last_address == 0x0456 &&
        mapper.last_value == 0x77
    );
}

bool RunMapperExpansionSmokeTest() {
    ExpansionTestMapper mapper;
    NES::MainBus bus;
    bus.set_mapper(&mapper);
    bus.write(0x5000, 0x5a);
    return bus.read(0x5000) == 0x5a;
}

bool RunMapperPRGRAMSmokeTest() {
    PRGRAMTestMapper mapper;
    NES::MainBus bus;
    bus.set_mapper(&mapper);
    bus.write(0x6000, 0x11);
    bus.write(0x8000, 0x01);
    bus.write(0x6000, 0x22);
    bool banked = bus.read(0x6000) == 0x22;
    bus.write(0x8000, 0x00);
    banked = banked && bus.read(0x6000) == 0x11;
    mapper.protectPRGRAM();
    bus.write(0x6000, 0x99);
    return banked && bus.read(0x6000) == 0x11;
}

bool RunMapperNameTableSmokeTest() {
    NameTableTestMapper mapper;
    NES::PictureBus bus;
    bus.set_mapper(&mapper);
    bus.write(0x2401, 0x66);
    return bus.read(0x2401) == 0x66;
}

unsigned int RunMapperBankHelperSmokeTests() {
    unsigned int results = 0;

    std::vector<NES::NES_Byte> prg_8k(4 * 0x2000, 0);
    FillBankMarkers(prg_8k, 0x2000, 0x10);
    NES::MapperBank::BankWindow prg_8k_window;
    prg_8k_window.selectBank(prg_8k.size(), 0x2000, 2);
    if (
        prg_8k_window.read(prg_8k, 0x8000, 0x8000) == 0x12 &&
        prg_8k_window.read(prg_8k, 0x9fff, 0x8000) == 0x12
    ) {
        results |= BANK_HELPER_PRG_8K;
    }

    std::vector<NES::NES_Byte> prg_16k(4 * 0x4000, 0);
    FillBankMarkers(prg_16k, 0x4000, 0x20);
    NES::MapperBank::BankWindow first_prg_16k;
    NES::MapperBank::BankWindow final_prg_16k;
    first_prg_16k.selectFirst(prg_16k.size(), 0x4000);
    final_prg_16k.selectFinal(prg_16k.size(), 0x4000);
    if (
        first_prg_16k.read(prg_16k, 0x8000, 0x8000) == 0x20 &&
        final_prg_16k.read(prg_16k, 0xc000, 0xc000) == 0x23
    ) {
        results |= BANK_HELPER_PRG_16K;
    }

    NES::MapperBank::BankWindow prg_32k_window;
    prg_32k_window.selectWindow(prg_16k.size(), 0x4000, 0x8000, 3);
    if (
        prg_32k_window.bank() == 2 &&
        prg_32k_window.read(prg_16k, 0x8000, 0x8000) == 0x22 &&
        prg_32k_window.read(prg_16k, 0xc000, 0x8000) == 0x23
    ) {
        results |= BANK_HELPER_PRG_32K;
    }

    std::vector<NES::NES_Byte> chr_1k(16 * 0x0400, 0);
    FillBankMarkers(chr_1k, 0x0400, 0x40);
    NES::MapperBank::BankWindow chr_1k_window;
    chr_1k_window.selectBank(chr_1k.size(), 0x0400, 7);
    if (chr_1k_window.read(chr_1k, 0x0000, 0x0000) == 0x47)
        results |= BANK_HELPER_CHR_1K;

    NES::MapperBank::BankWindow chr_2k_window;
    chr_2k_window.selectWindow(chr_1k.size(), 0x0400, 0x0800, 5);
    if (
        chr_2k_window.bank() == 4 &&
        chr_2k_window.read(chr_1k, 0x0000, 0x0000) == 0x44 &&
        chr_2k_window.read(chr_1k, 0x0400, 0x0000) == 0x45
    ) {
        results |= BANK_HELPER_CHR_2K;
    }

    NES::MapperBank::BankWindow chr_4k_window;
    chr_4k_window.selectWindow(chr_1k.size(), 0x0400, 0x1000, 7);
    if (
        chr_4k_window.bank() == 4 &&
        chr_4k_window.read(chr_1k, 0x0000, 0x0000) == 0x44 &&
        chr_4k_window.read(chr_1k, 0x0c00, 0x0000) == 0x47
    ) {
        results |= BANK_HELPER_CHR_4K;
    }

    NES::MapperBank::BankWindow chr_8k_window;
    chr_8k_window.selectWindow(chr_1k.size(), 0x0400, 0x2000, 13);
    if (
        chr_8k_window.bank() == 8 &&
        chr_8k_window.read(chr_1k, 0x0000, 0x0000) == 0x48 &&
        chr_8k_window.read(chr_1k, 0x1c00, 0x0000) == 0x4f
    ) {
        results |= BANK_HELPER_CHR_8K;
    }

    if (
        NES::MapperBank::maskBankSelect(0x15, 4) == 1 &&
        NES::MapperBank::maskBankSelect(3, 8, 2) == 2 &&
        NES::MapperBank::resolveBusConflict(true, 0xf0, 0xcc) == 0xc0 &&
        NES::MapperBank::resolveBusConflict(false, 0xf0, 0xcc) == 0xf0
    ) {
        results |= BANK_HELPER_MASKS;
    }

    return results;
}

}  // namespace

// definitions of functions for the Python interface to access
extern "C" {
    /// Return the width of the NES.
    EXP int Width() {
        return NES::Emulator::WIDTH;
    }

    /// Return the height of the NES.
    EXP int Height() {
        return NES::Emulator::HEIGHT;
    }

    /// Initialize a new emulator and return a pointer to it
    EXP NES::Emulator* Initialize(wchar_t* path) {
        // convert the c string to a c++ std string data structure
        std::string rom_path = ROMPath(path);
        // create a new emulator with the given ROM path
        return new NES::Emulator(rom_path);
    }

    /// Return a native cartridge validation error, or null when valid.
    EXP const char* CartridgeError(wchar_t* path) {
        cartridge_error.clear();
        try {
            NES::Cartridge cartridge;
            cartridge.loadFromFile(ROMPath(path));
            return nullptr;
        } catch (const std::exception& error) {
            cartridge_error = error.what();
        } catch (...) {
            cartridge_error = "unknown cartridge validation error";
        }
        return cartridge_error.c_str();
    }

    /// Return the parsed native mapper number for a ROM path.
    EXP unsigned int CartridgeMapperNumber(wchar_t* path) {
        return LoadCartridgeMetadata(path).getMapper();
    }

    /// Return the parsed native NES 2.0 submapper number for a ROM path.
    EXP unsigned int CartridgeSubmapper(wchar_t* path) {
        return LoadCartridgeMetadata(path).getSubmapper();
    }

    /// Return the parsed native PRG ROM size in bytes.
    EXP std::size_t CartridgePRGROMSize(wchar_t* path) {
        return LoadCartridgeMetadata(path).getMetadata().prg_rom_size;
    }

    /// Return the parsed native PRG ROM bank count.
    EXP std::size_t CartridgePRGROMBanks(wchar_t* path) {
        return LoadCartridgeMetadata(path).getMetadata().prg_rom_banks;
    }

    /// Return the parsed native CHR ROM size in bytes.
    EXP std::size_t CartridgeCHRROMSize(wchar_t* path) {
        return LoadCartridgeMetadata(path).getMetadata().chr_rom_size;
    }

    /// Return the parsed native CHR ROM bank count.
    EXP std::size_t CartridgeCHRROMBanks(wchar_t* path) {
        return LoadCartridgeMetadata(path).getMetadata().chr_rom_banks;
    }

    /// Return the parsed native ordinary PRG RAM size in bytes.
    EXP std::size_t CartridgePRGRAMSize(wchar_t* path) {
        return LoadCartridgeMetadata(path).getMetadata().prg_ram_size;
    }

    /// Return the parsed native battery-backed PRG RAM size in bytes.
    EXP std::size_t CartridgePRGBatteryRAMSize(wchar_t* path) {
        return LoadCartridgeMetadata(path).getMetadata().prg_battery_ram_size;
    }

    /// Return the parsed native ordinary CHR RAM size in bytes.
    EXP std::size_t CartridgeCHRRAMSize(wchar_t* path) {
        return LoadCartridgeMetadata(path).getMetadata().chr_ram_size;
    }

    /// Return the parsed native battery-backed CHR RAM size in bytes.
    EXP std::size_t CartridgeCHRBatteryRAMSize(wchar_t* path) {
        return LoadCartridgeMetadata(path).getMetadata().chr_battery_ram_size;
    }

    /// Return the parsed native trainer flag.
    EXP int CartridgeHasTrainer(wchar_t* path) {
        return LoadCartridgeMetadata(path).getMetadata().has_trainer ? 1 : 0;
    }

    /// Return the parsed native trainer start offset.
    EXP std::size_t CartridgeTrainerStart(wchar_t* path) {
        return LoadCartridgeMetadata(path).getMetadata().trainer_start;
    }

    /// Return the parsed native trainer stop offset.
    EXP std::size_t CartridgeTrainerStop(wchar_t* path) {
        return LoadCartridgeMetadata(path).getMetadata().trainer_stop;
    }

    /// Return the parsed native battery flag.
    EXP int CartridgeHasBattery(wchar_t* path) {
        return LoadCartridgeMetadata(path).getMetadata().has_battery ? 1 : 0;
    }

    /// Return the parsed native header mirroring mode.
    EXP unsigned int CartridgeNameTableMirroring(wchar_t* path) {
        return LoadCartridgeMetadata(path).getMetadata().name_table_mirroring;
    }

    /// Return the parsed native VS Unisystem flag.
    EXP int CartridgeHasVSUnisystem(wchar_t* path) {
        return LoadCartridgeMetadata(path).getMetadata().has_vs_unisystem ? 1 : 0;
    }

    /// Return the parsed native PlayChoice-10 flag.
    EXP int CartridgeHasPlayChoice10(wchar_t* path) {
        return LoadCartridgeMetadata(path).getMetadata().has_play_choice_10 ? 1 : 0;
    }

    /// Return the parsed native PAL flag.
    EXP int CartridgeIsPAL(wchar_t* path) {
        return LoadCartridgeMetadata(path).getMetadata().is_pal ? 1 : 0;
    }

    /// Return the parsed native NES 2.0 header flag.
    EXP int CartridgeIsNES2(wchar_t* path) {
        return LoadCartridgeMetadata(path).getMetadata().is_nes2 ? 1 : 0;
    }

    /// Return 1 when a mapper ID has a registered native implementation.
    EXP int IsMapperSupported(unsigned int mapper_id) {
        return NES::IsMapperSupported(
            static_cast<std::uint16_t>(mapper_id)
        ) ? 1 : 0;
    }

    /// Return 1 when a fake mapper IRQ reaches the CPU IRQ vector path.
    EXP int MapperIRQSmokeTest() {
        return RunMapperIRQSmokeTest() ? 1 : 0;
    }

    /// Return 1 when mapper CPU-cycle hooks are observable.
    EXP int MapperCPUCycleHookSmokeTest() {
        return RunMapperCPUCycleHookSmokeTest() ? 1 : 0;
    }

    /// Return 1 when mapper PPU address/read/write hooks are observable.
    EXP int MapperPPUHookSmokeTest() {
        return RunMapperPPUHookSmokeTest() ? 1 : 0;
    }

    /// Return 1 when mapper expansion-area read/write routing works.
    EXP int MapperExpansionSmokeTest() {
        return RunMapperExpansionSmokeTest() ? 1 : 0;
    }

    /// Return 1 when mapper PRG RAM banking and write protection work.
    EXP int MapperPRGRAMSmokeTest() {
        return RunMapperPRGRAMSmokeTest() ? 1 : 0;
    }

    /// Return 1 when mapper-owned nametable read/write routing works.
    EXP int MapperNameTableSmokeTest() {
        return RunMapperNameTableSmokeTest() ? 1 : 0;
    }

    /// Return a pass bitmask for focused native mapper bank helper checks.
    EXP unsigned int MapperBankHelperSmokeResults() {
        return RunMapperBankHelperSmokeTests();
    }

    /// Return a pointer to a controller on the machine
    EXP NES::NES_Byte* Controller(NES::Emulator* emu, int port) {
        return emu->get_controller(port);
    }

    /// Return the pointer to the screen buffer
    EXP NES::NES_Pixel* Screen(NES::Emulator* emu) {
        return emu->get_screen_buffer();
    }

    /// Return the pointer to the memory buffer
    EXP NES::NES_Byte* Memory(NES::Emulator* emu) {
        return emu->get_memory_buffer();
    }

    /// Return the active mapper number.
    EXP int MapperNumber(NES::Emulator* emu) {
        return emu->get_mapper_number();
    }

    /// Return the PRG ROM size in bytes.
    EXP unsigned int PRGROMSize(NES::Emulator* emu) {
        return emu->get_prg_rom_size();
    }

    /// Return the CHR ROM size in bytes.
    EXP unsigned int CHRROMSize(NES::Emulator* emu) {
        return emu->get_chr_rom_size();
    }

    /// Return 1 when the active mapper uses CHR RAM, 0 for CHR ROM.
    EXP int HasCHRRAM(NES::Emulator* emu) {
        return emu->has_chr_ram() ? 1 : 0;
    }

    /// Return the active mapper mirroring mode.
    EXP int NameTableMirroring(NES::Emulator* emu) {
        return emu->get_name_table_mirroring();
    }

    /// Read active mapper PRG memory.
    EXP unsigned int ReadPRG(NES::Emulator* emu, unsigned int address) {
        return emu->read_prg(static_cast<NES::NES_Address>(address));
    }

    /// Write active mapper PRG memory.
    EXP void WritePRG(NES::Emulator* emu, unsigned int address, unsigned int value) {
        emu->write_prg(
            static_cast<NES::NES_Address>(address),
            static_cast<NES::NES_Byte>(value)
        );
    }

    /// Read active mapper CHR memory.
    EXP unsigned int ReadCHR(NES::Emulator* emu, unsigned int address) {
        return emu->read_chr(static_cast<NES::NES_Address>(address));
    }

    /// Write active mapper CHR memory.
    EXP void WriteCHR(NES::Emulator* emu, unsigned int address, unsigned int value) {
        emu->write_chr(
            static_cast<NES::NES_Address>(address),
            static_cast<NES::NES_Byte>(value)
        );
    }

    /// Reset the emulator
    EXP void Reset(NES::Emulator* emu) {
        emu->reset();
    }

    /// Perform a discrete step in the emulator (i.e., 1 frame)
    EXP void Step(NES::Emulator* emu) {
        emu->step();
    }

    /// Create a deep copy (i.e., a clone) of the given emulator
    EXP void Backup(NES::Emulator* emu) {
        emu->backup();
    }

    /// Create a deep copy (i.e., a clone) of the given emulator
    EXP void Restore(NES::Emulator* emu) {
        emu->restore();
    }

    /// Close the emulator, i.e., purge it from memory
    EXP void Close(NES::Emulator* emu) {
        delete emu;
    }
}

// un-define the macro
#undef EXP
