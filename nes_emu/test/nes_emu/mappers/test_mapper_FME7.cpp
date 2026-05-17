//  Program:      nes-py
//  File:         test_mapper_FME7.cpp
//  Description:  Catch2 coverage for native mapper 069 / Sunsoft FME-7
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <catch2/catch_test_macros.hpp>
#include "nes_emu/test/nes_emu/support/emulator_inspector.hpp"
#include "nes_emu/test/nes_emu/support/mapper_test_helpers.hpp"

namespace {

const std::size_t PRG_8K_SIZE = 0x2000;
const std::size_t CHR_1K_SIZE = 0x0400;

NES::NES_Byte prg_8k_marker(std::size_t bank) {
    return static_cast<NES::NES_Byte>((0x20 + bank) & 0xff);
}

NES::NES_Byte chr_1k_marker(std::size_t bank) {
    return static_cast<NES::NES_Byte>((0x80 + bank) & 0xff);
}

std::string temporary_path(const std::string& name) {
    static unsigned int counter = 0;
    const char* tmpdir = std::getenv("TMPDIR");
    if (tmpdir == nullptr)
        tmpdir = "/tmp";
    std::ostringstream stream;
    stream << tmpdir << "/nes_emu_fme7_" << name << "_"
           << std::chrono::steady_clock::now().time_since_epoch().count()
           << "_" << counter++ << ".nes";
    return stream.str();
}

class TemporaryFME7ROM {
 private:
    std::string path;

 public:
    TemporaryFME7ROM(
        const std::string& name,
        std::size_t prg_8k_banks = 8,
        std::size_t chr_1k_banks = 16
    ) : path(temporary_path(name)) {
        const std::uint16_t mapper = 69;
        std::vector<NES::NES_Byte> bytes = {
            0x4e, 0x45, 0x53, 0x1a,
            static_cast<NES::NES_Byte>((prg_8k_banks / 2) & 0xff),
            static_cast<NES::NES_Byte>((chr_1k_banks / 8) & 0xff),
            static_cast<NES::NES_Byte>((mapper & 0x0f) << 4),
            static_cast<NES::NES_Byte>(mapper & 0xf0),
            0x01, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
        };

        std::vector<NES::NES_Byte> prg;
        for (std::size_t bank = 0; bank < prg_8k_banks; ++bank) {
            prg.insert(prg.end(), PRG_8K_SIZE, prg_8k_marker(bank));
        }
        if (!prg.empty()) {
            prg[0] = 0xea;
            prg[1] = 0x4c;
            prg[2] = 0x00;
            prg[3] = 0xe0;
            prg[prg.size() - 4] = 0x00;
            prg[prg.size() - 3] = 0xe0;
        }
        bytes.insert(bytes.end(), prg.begin(), prg.end());

        for (std::size_t bank = 0; bank < chr_1k_banks; ++bank) {
            bytes.insert(bytes.end(), CHR_1K_SIZE, chr_1k_marker(bank));
        }

        std::ofstream output(path.c_str(), std::ios::binary);
        output.write(
            reinterpret_cast<const char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size())
        );
        if (!output)
            throw std::runtime_error("failed to write synthetic FME-7 ROM");
    }

    ~TemporaryFME7ROM() {
        std::remove(path.c_str());
    }

    const std::string& filename() const {
        return path;
    }

    NES::Cartridge load() const {
        NES::Cartridge cartridge;
        cartridge.loadFromFile(path, true);
        return cartridge;
    }
};

void write_fme7_command(
    NES::Mapper& mapper,
    NES::NES_Byte command,
    NES::NES_Byte value
) {
    mapper.writePRG(0x8000, command);
    mapper.writePRG(0xa000, value);
}

}  // namespace

TEST_CASE(
    "mapper 069 FME-7 registers and maps default PRG windows",
    "[mapper][fme7]"
) {
    TemporaryFME7ROM rom("registration");
    NES::Cartridge cartridge = rom.load();

    REQUIRE(NES::IsMapperSupported(69));
    std::unique_ptr<NES::Mapper> mapper = NESTest::mapper_for(cartridge);

    REQUIRE(mapper->readPRG(0x8100) == prg_8k_marker(0));
    REQUIRE(mapper->readPRG(0xa100) == prg_8k_marker(1));
    REQUIRE(mapper->readPRG(0xc100) == prg_8k_marker(2));
    REQUIRE(mapper->readPRG(0xe100) == prg_8k_marker(7));
}

TEST_CASE(
    "mapper 069 FME-7 command latch controls 1 KiB CHR windows",
    "[mapper][fme7]"
) {
    TemporaryFME7ROM rom("chr");
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = NESTest::mapper_for(cartridge);

    for (std::size_t window = 0; window < 8; ++window) {
        write_fme7_command(
            *mapper,
            static_cast<NES::NES_Byte>(window),
            static_cast<NES::NES_Byte>(15 - window)
        );
    }

    for (std::size_t window = 0; window < 8; ++window) {
        const NES::NES_Address address = static_cast<NES::NES_Address>(
            window * CHR_1K_SIZE
        );
        REQUIRE(mapper->readCHR(address) == chr_1k_marker(15 - window));
        REQUIRE(mapper->readCHR(address + 0x03ff) == chr_1k_marker(15 - window));
    }

    mapper->writePRG(0x8000, 0x02);
    mapper->writePRG(0xa000, 0x04);
    REQUIRE(mapper->readCHR(0x0800) == chr_1k_marker(4));
    mapper->writePRG(0xa000, 0x06);
    REQUIRE(mapper->readCHR(0x0800) == chr_1k_marker(6));
    REQUIRE(mapper->readCHR(0x0c00) == chr_1k_marker(12));
}

TEST_CASE(
    "mapper 069 FME-7 switches PRG windows and $6000 RAM/ROM mode",
    "[mapper][fme7]"
) {
    TemporaryFME7ROM rom("prg");
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = NESTest::mapper_for(cartridge);

    write_fme7_command(*mapper, 0x09, 0x04);
    write_fme7_command(*mapper, 0x0a, 0x05);
    write_fme7_command(*mapper, 0x0b, 0x06);
    REQUIRE(mapper->readPRG(0x8000) == prg_8k_marker(4));
    REQUIRE(mapper->readPRG(0xa000) == prg_8k_marker(5));
    REQUIRE(mapper->readPRG(0xc000) == prg_8k_marker(6));
    REQUIRE(mapper->readPRG(0xe000) == prg_8k_marker(7));

    write_fme7_command(*mapper, 0x08, 0x03);
    REQUIRE(mapper->readPRGRAM(0x6000) == prg_8k_marker(3));
    mapper->writePRGRAM(0x6000, 0xa5);
    REQUIRE(mapper->readPRGRAM(0x6000) == prg_8k_marker(3));

    write_fme7_command(*mapper, 0x08, 0xc0);
    mapper->writePRGRAM(0x6000, 0x5a);
    REQUIRE(mapper->readPRGRAM(0x6000) == 0x5a);
    REQUIRE(mapper->getPRGRAMPointer(0x6000) != nullptr);
    REQUIRE(*mapper->getPRGRAMPointer(0x6000) == 0x5a);

    write_fme7_command(*mapper, 0x08, 0x40);
    mapper->writePRGRAM(0x6000, 0x33);
    REQUIRE(mapper->readPRGRAM(0x6000) == 0x00);
    REQUIRE(mapper->getPRGRAMPointer(0x6000) == nullptr);

    write_fme7_command(*mapper, 0x08, 0xc0);
    REQUIRE(mapper->readPRGRAM(0x6000) == 0x5a);
}

TEST_CASE("mapper 069 FME-7 supports all mirroring modes", "[mapper][fme7]") {
    TemporaryFME7ROM rom("mirroring");
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = NESTest::mapper_for(cartridge);

    write_fme7_command(*mapper, 0x0c, 0x00);
    REQUIRE(mapper->getNameTableMirroring() == NES::VERTICAL);
    write_fme7_command(*mapper, 0x0c, 0x01);
    REQUIRE(mapper->getNameTableMirroring() == NES::HORIZONTAL);
    write_fme7_command(*mapper, 0x0c, 0x02);
    REQUIRE(mapper->getNameTableMirroring() == NES::ONE_SCREEN_LOWER);
    write_fme7_command(*mapper, 0x0c, 0x03);
    REQUIRE(mapper->getNameTableMirroring() == NES::ONE_SCREEN_HIGHER);
}

TEST_CASE("mapper 069 FME-7 IRQ counter wraps on CPU cycles", "[mapper][fme7]") {
    TemporaryFME7ROM rom("irq");
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = NESTest::mapper_for(cartridge);
    int irq_count = 0;
    mapper->setIRQCallback([&]() { ++irq_count; });

    REQUIRE(mapper->observesCPUCycles());
    write_fme7_command(*mapper, 0x0d, 0x01);
    mapper->onCPUCycle();
    REQUIRE(irq_count == 0);

    write_fme7_command(*mapper, 0x0d, 0x81);
    mapper->onCPUCycle();
    REQUIRE(irq_count == 1);
    mapper->onCPUCycle();
    REQUIRE(irq_count == 1);

    write_fme7_command(*mapper, 0x0d, 0x81);
    write_fme7_command(*mapper, 0x0e, 0x00);
    write_fme7_command(*mapper, 0x0f, 0x00);
    mapper->onCPUCycle();
    REQUIRE(irq_count == 2);

    write_fme7_command(*mapper, 0x0e, 0x00);
    write_fme7_command(*mapper, 0x0f, 0x00);
    write_fme7_command(*mapper, 0x0d, 0x80);
    mapper->onCPUCycle();
    REQUIRE(irq_count == 2);
}

TEST_CASE(
    "mapper 069 FME-7 clone preserves command, banks, RAM, mirroring, and IRQ",
    "[mapper][fme7]"
) {
    TemporaryFME7ROM rom("clone");
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = NESTest::mapper_for(cartridge);

    write_fme7_command(*mapper, 0x00, 0x05);
    write_fme7_command(*mapper, 0x09, 0x04);
    write_fme7_command(*mapper, 0x08, 0xc0);
    mapper->writePRGRAM(0x6000, 0x33);
    write_fme7_command(*mapper, 0x0c, 0x01);
    write_fme7_command(*mapper, 0x0e, 0x00);
    write_fme7_command(*mapper, 0x0f, 0x00);
    write_fme7_command(*mapper, 0x0d, 0x81);
    mapper->writePRG(0x8000, 0x00);

    std::unique_ptr<NES::Mapper> backup = mapper->clone();

    write_fme7_command(*mapper, 0x00, 0x07);
    write_fme7_command(*mapper, 0x09, 0x02);
    write_fme7_command(*mapper, 0x0c, 0x00);
    mapper->writePRGRAM(0x6000, 0x44);

    REQUIRE(backup->readCHR(0x0000) == chr_1k_marker(5));
    REQUIRE(backup->readPRG(0x8000) == prg_8k_marker(4));
    REQUIRE(backup->readPRGRAM(0x6000) == 0x33);
    REQUIRE(backup->getNameTableMirroring() == NES::HORIZONTAL);

    int irq_count = 0;
    backup->setIRQCallback([&]() { ++irq_count; });
    backup->onCPUCycle();
    REQUIRE(irq_count == 1);

    backup->writePRG(0xa000, 0x06);
    REQUIRE(backup->readCHR(0x0000) == chr_1k_marker(6));
}

TEST_CASE("mapper 069 emulator save-state preserves FME-7 state", "[mapper][fme7]") {
    TemporaryFME7ROM rom("emulator_backup");
    NES::Emulator emulator(rom.filename());
    NES::Mapper& mapper = NES::EmulatorInspector::mapper(emulator);

    write_fme7_command(mapper, 0x00, 0x05);
    write_fme7_command(mapper, 0x09, 0x04);
    write_fme7_command(mapper, 0x08, 0xc0);
    mapper.writePRGRAM(0x6000, 0x33);
    write_fme7_command(mapper, 0x0c, 0x01);

    emulator.backup();

    write_fme7_command(mapper, 0x00, 0x07);
    write_fme7_command(mapper, 0x09, 0x02);
    mapper.writePRGRAM(0x6000, 0x44);
    write_fme7_command(mapper, 0x0c, 0x00);

    emulator.restore();
    NES::Mapper& restored = NES::EmulatorInspector::mapper(emulator);
    REQUIRE(restored.readCHR(0x0000) == chr_1k_marker(5));
    REQUIRE(restored.readPRG(0x8000) == prg_8k_marker(4));
    REQUIRE(restored.readPRGRAM(0x6000) == 0x33);
    REQUIRE(restored.getNameTableMirroring() == NES::HORIZONTAL);
}
