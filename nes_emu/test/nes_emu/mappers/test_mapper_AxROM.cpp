//  Program:      nes-py
//  File:         test_mapper_AxROM.cpp
//  Description:  Catch2 coverage for native mapper 007 / AxROM
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include <chrono>
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
#include "nes_emu/test/nes_emu/support/synthetic_rom.hpp"

namespace {

class TemporaryNES2AxROM {
 private:
    std::string path;

    static std::string temporary_path(const std::string& name) {
        static unsigned int counter = 0;
        const char* tmpdir = std::getenv("TMPDIR");
        if (tmpdir == nullptr)
            tmpdir = "/tmp";
        std::ostringstream stream;
        stream << tmpdir << "/nes_emu_" << name << "_"
               << std::chrono::steady_clock::now().time_since_epoch().count()
               << "_" << counter++ << ".nes";
        return stream.str();
    }

 public:
    TemporaryNES2AxROM(
        const std::string& name,
        NES::NES_Byte submapper,
        NES::NES_Byte visible_conflict_value
    ) : path(temporary_path(name)) {
        std::vector<NES::NES_Byte> bytes = {
            0x4e, 0x45, 0x53, 0x1a,
            0x08, 0x00, 0x70, 0x08,
            static_cast<NES::NES_Byte>((submapper & 0x0f) << 4),
            0x00, 0x00, 0x07,
            0x00, 0x00, 0x00, 0x00,
        };

        for (std::size_t bank = 0; bank < 8; ++bank) {
            bytes.insert(
                bytes.end(),
                NESTest::PRG_BANK_SIZE,
                NESTest::prg_bank_marker(bank)
            );
        }
        bytes[0x10] = visible_conflict_value;

        std::ofstream output(path.c_str(), std::ios::binary);
        output.write(
            reinterpret_cast<const char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size())
        );
        if (!output)
            throw std::runtime_error("failed to write synthetic AxROM ROM");
    }

    ~TemporaryNES2AxROM() {
        std::remove(path.c_str());
    }

    NES::Cartridge load() const {
        NES::Cartridge cartridge;
        cartridge.loadFromFile(path, true);
        return cartridge;
    }
};

}  // namespace

TEST_CASE(
    "mapper 007 AxROM switches 32 KiB PRG banks and one-screen mirroring",
    "[mapper][axrom]"
) {
    NESTest::TemporaryROM rom("axrom", 7, 8, 0);
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = NESTest::mapper_for(cartridge);

    REQUIRE(mapper->readPRG(0x9000) == NESTest::prg_bank_marker(0));
    REQUIRE(mapper->readPRG(0xd000) == NESTest::prg_bank_marker(1));
    REQUIRE(mapper->getNameTableMirroring() == NES::ONE_SCREEN_LOWER);

    mapper->writePRG(0x8000, 0x13);
    REQUIRE(mapper->readPRG(0x9000) == NESTest::prg_bank_marker(6));
    REQUIRE(mapper->readPRG(0xd000) == NESTest::prg_bank_marker(7));
    REQUIRE(mapper->getNameTableMirroring() == NES::ONE_SCREEN_HIGHER);

    mapper->writePRG(0x8000, 0x06);
    REQUIRE(mapper->readPRG(0x9000) == NESTest::prg_bank_marker(4));
    REQUIRE(mapper->readPRG(0xd000) == NESTest::prg_bank_marker(5));
    REQUIRE(mapper->getNameTableMirroring() == NES::ONE_SCREEN_LOWER);
}

TEST_CASE("mapper 007 AxROM provides CHR RAM", "[mapper][axrom]") {
    NESTest::TemporaryROM rom("axrom_chr_ram", 7, 2, 0);
    NES::Cartridge cartridge = rom.load();
    std::unique_ptr<NES::Mapper> mapper = NESTest::mapper_for(cartridge);

    mapper->writeCHR(0x0456, 0x3c);
    mapper->writeCHR(0x2456, 0xa5);
    REQUIRE(mapper->readCHR(0x0456) == 0xa5);
}

TEST_CASE(
    "mapper 007 AxROM resolves bus conflicts for NES 2.0 submapper 2",
    "[mapper][axrom]"
) {
    TemporaryNES2AxROM no_conflict_rom("axrom_no_conflict", 0, 0x12);
    NES::Cartridge no_conflict_cartridge = no_conflict_rom.load();
    std::unique_ptr<NES::Mapper> no_conflict_mapper =
        NESTest::mapper_for(no_conflict_cartridge);

    REQUIRE_FALSE(no_conflict_mapper->hasBusConflicts());
    no_conflict_mapper->writePRG(
        0x8000,
        no_conflict_mapper->resolveBusConflict(0x8000, 0x17)
    );
    REQUIRE(no_conflict_mapper->readPRG(0x9000) == NESTest::prg_bank_marker(6));
    REQUIRE(
        no_conflict_mapper->getNameTableMirroring() == NES::ONE_SCREEN_HIGHER
    );

    TemporaryNES2AxROM conflict_rom("axrom_conflict", 2, 0x12);
    NES::Cartridge conflict_cartridge = conflict_rom.load();
    std::unique_ptr<NES::Mapper> conflict_mapper =
        NESTest::mapper_for(conflict_cartridge);

    REQUIRE(conflict_mapper->hasBusConflicts());
    conflict_mapper->writePRG(
        0x8000,
        conflict_mapper->resolveBusConflict(0x8000, 0x17)
    );
    REQUIRE(conflict_mapper->readPRG(0x9000) == NESTest::prg_bank_marker(4));
    REQUIRE(
        conflict_mapper->getNameTableMirroring() == NES::ONE_SCREEN_HIGHER
    );
}

TEST_CASE(
    "mapper 007 emulator save-state preserves selected bank mirroring and CHR RAM",
    "[mapper][axrom]"
) {
    NESTest::TemporaryROM rom("axrom_emulator_backup", 7, 8, 0);
    NES::Emulator emulator(rom.filename());
    NES::Mapper& mapper = NES::EmulatorInspector::mapper(emulator);

    mapper.writePRG(0x8000, 0x13);
    mapper.writeCHR(0x0123, 0x5a);
    REQUIRE(mapper.readPRG(0x9000) == NESTest::prg_bank_marker(6));
    REQUIRE(mapper.readCHR(0x0123) == 0x5a);
    REQUIRE(mapper.getNameTableMirroring() == NES::ONE_SCREEN_HIGHER);

    emulator.backup();

    mapper.writePRG(0x8000, 0x02);
    mapper.writeCHR(0x0123, 0xa5);
    REQUIRE(mapper.readPRG(0x9000) == NESTest::prg_bank_marker(4));
    REQUIRE(mapper.readCHR(0x0123) == 0xa5);
    REQUIRE(mapper.getNameTableMirroring() == NES::ONE_SCREEN_LOWER);

    emulator.restore();
    NES::Mapper& restored = NES::EmulatorInspector::mapper(emulator);
    REQUIRE(restored.readPRG(0x9000) == NESTest::prg_bank_marker(6));
    REQUIRE(restored.readPRG(0xd000) == NESTest::prg_bank_marker(7));
    REQUIRE(restored.readCHR(0x0123) == 0x5a);
    REQUIRE(restored.getNameTableMirroring() == NES::ONE_SCREEN_HIGHER);
}
