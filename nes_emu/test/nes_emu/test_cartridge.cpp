//  Program:      nes-py
//  File:         test_cartridge.cpp
//  Description:  Catch2 coverage for native cartridge metadata parsing
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <catch2/catch_test_macros.hpp>
#include "nes_emu/cartridge.hpp"
#include "nes_emu/test/nes_emu/support/synthetic_rom.hpp"

namespace {

class TemporaryBytes {
 private:
    std::string path;

    static std::string temporary_path(const std::string& name) {
        static unsigned int counter = 0;
        const char* tmpdir = std::getenv("TMPDIR");
        if (tmpdir == nullptr)
            tmpdir = "/tmp";
        std::ostringstream stream;
        stream << tmpdir << "/nes_emu_cartridge_" << name << "_"
               << std::chrono::steady_clock::now().time_since_epoch().count()
               << "_" << counter++ << ".nes";
        return stream.str();
    }

 public:
    TemporaryBytes(
        const std::string& name,
        const std::vector<NES::NES_Byte>& bytes
    ) : path(temporary_path(name)) {
        std::ofstream output(path.c_str(), std::ios::binary);
        output.write(
            reinterpret_cast<const char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size())
        );
        if (!output)
            throw std::runtime_error("failed to write temporary ROM");
    }

    ~TemporaryBytes() {
        std::remove(path.c_str());
    }

    const std::string& filename() const {
        return path;
    }
};

std::vector<NES::NES_Byte> ines_header(
    std::uint16_t mapper,
    std::size_t prg_banks,
    std::size_t chr_banks,
    const std::string& mirroring = "horizontal",
    bool trainer = false,
    bool battery = false,
    bool vs_unisystem = false,
    bool play_choice_10 = false,
    bool pal = false,
    NES::NES_Byte prg_ram_banks = 0,
    bool nes2 = false,
    NES::NES_Byte submapper = 0,
    NES::NES_Byte prg_ram_shift = 0,
    NES::NES_Byte prg_battery_ram_shift = 0,
    NES::NES_Byte chr_ram_shift = 0,
    NES::NES_Byte chr_battery_ram_shift = 0
) {
    NES::NES_Byte flags_6 = static_cast<NES::NES_Byte>(
        (mapper & 0x0f) << 4
    );
    if (mirroring == "vertical") {
        flags_6 |= 0x01;
    } else if (mirroring == "four-screen") {
        flags_6 |= 0x08;
    } else if (mirroring != "horizontal") {
        throw std::invalid_argument("unknown mirroring mode");
    }
    if (battery || prg_battery_ram_shift || chr_battery_ram_shift)
        flags_6 |= 0x02;
    if (trainer)
        flags_6 |= 0x04;

    NES::NES_Byte flags_7 = static_cast<NES::NES_Byte>(mapper & 0xf0);
    if (vs_unisystem)
        flags_7 |= 0x01;
    if (play_choice_10)
        flags_7 |= 0x02;
    if (nes2)
        flags_7 |= 0x08;

    NES::NES_Byte header_8 = prg_ram_banks;
    NES::NES_Byte header_9 = pal ? 0x01 : 0x00;
    NES::NES_Byte header_10 = 0x00;
    NES::NES_Byte header_11 = 0x00;
    NES::NES_Byte header_12 = 0x00;
    if (nes2) {
        header_8 = static_cast<NES::NES_Byte>(
            ((submapper & 0x0f) << 4) | ((mapper >> 8) & 0x0f)
        );
        header_9 = static_cast<NES::NES_Byte>(
            (((chr_banks >> 8) & 0x0f) << 4) |
            ((prg_banks >> 8) & 0x0f)
        );
        header_10 = static_cast<NES::NES_Byte>(
            ((prg_battery_ram_shift & 0x0f) << 4) |
            (prg_ram_shift & 0x0f)
        );
        header_11 = static_cast<NES::NES_Byte>(
            ((chr_battery_ram_shift & 0x0f) << 4) |
            (chr_ram_shift & 0x0f)
        );
        header_12 = pal ? 0x01 : 0x00;
    }

    return {
        0x4e, 0x45, 0x53, 0x1a,
        static_cast<NES::NES_Byte>(prg_banks & 0xff),
        static_cast<NES::NES_Byte>(chr_banks & 0xff),
        flags_6,
        flags_7,
        header_8,
        header_9,
        header_10,
        header_11,
        header_12,
        0x00,
        0x00,
        0x00,
    };
}

std::vector<NES::NES_Byte> rom_bytes(
    std::uint16_t mapper,
    std::size_t prg_banks,
    std::size_t chr_banks,
    const std::string& mirroring = "horizontal",
    bool trainer = false,
    bool battery = false,
    bool vs_unisystem = false,
    bool play_choice_10 = false,
    bool pal = false,
    NES::NES_Byte prg_ram_banks = 0,
    bool nes2 = false,
    NES::NES_Byte submapper = 0,
    NES::NES_Byte prg_ram_shift = 0,
    NES::NES_Byte prg_battery_ram_shift = 0,
    NES::NES_Byte chr_ram_shift = 0,
    NES::NES_Byte chr_battery_ram_shift = 0
) {
    std::vector<NES::NES_Byte> bytes = ines_header(
        mapper,
        prg_banks,
        chr_banks,
        mirroring,
        trainer,
        battery,
        vs_unisystem,
        play_choice_10,
        pal,
        prg_ram_banks,
        nes2,
        submapper,
        prg_ram_shift,
        prg_battery_ram_shift,
        chr_ram_shift,
        chr_battery_ram_shift
    );
    if (trainer)
        bytes.insert(bytes.end(), 0x200, 0xa5);
    bytes.insert(bytes.end(), prg_banks * NESTest::PRG_BANK_SIZE, 0xea);
    bytes.insert(bytes.end(), chr_banks * NESTest::CHR_BANK_SIZE, 0x7f);
    return bytes;
}

NES::Cartridge load_cartridge(
    const TemporaryBytes& rom,
    bool allow_unsupported_features = true
) {
    NES::Cartridge cartridge;
    cartridge.loadFromFile(rom.filename(), allow_unsupported_features);
    return cartridge;
}

bool loading_throws_with_fragment(
    const TemporaryBytes& rom,
    bool allow_unsupported_features,
    const std::string& fragment
) {
    try {
        NES::Cartridge cartridge;
        cartridge.loadFromFile(rom.filename(), allow_unsupported_features);
    } catch (const std::exception& error) {
        return std::string(error.what()).find(fragment) != std::string::npos;
    }
    return false;
}

}  // namespace

TEST_CASE("native cartridge parses iNES metadata", "[cartridge]") {
    TemporaryBytes rom("ines_metadata", rom_bytes(
        2,
        2,
        0,
        "vertical",
        false,
        true,
        true,
        true,
        false,
        2
    ));
    NES::Cartridge cartridge = load_cartridge(rom);
    const NES::CartridgeMetadata& metadata = cartridge.getMetadata();

    REQUIRE(metadata.mapper_number == 2);
    REQUIRE(metadata.submapper == 0);
    REQUIRE(metadata.prg_rom_size == 2 * NESTest::PRG_BANK_SIZE);
    REQUIRE(metadata.prg_rom_banks == 2);
    REQUIRE(metadata.chr_rom_size == 0);
    REQUIRE(metadata.chr_rom_banks == 0);
    REQUIRE(metadata.prg_ram_size == 16 * 1024);
    REQUIRE(metadata.prg_battery_ram_size == 16 * 1024);
    REQUIRE(metadata.chr_ram_size == NESTest::CHR_BANK_SIZE);
    REQUIRE(metadata.chr_battery_ram_size == 0);
    REQUIRE(metadata.has_battery);
    REQUIRE(metadata.has_vs_unisystem);
    REQUIRE(metadata.has_play_choice_10);
    REQUIRE_FALSE(metadata.has_trainer);
    REQUIRE_FALSE(metadata.is_pal);
    REQUIRE_FALSE(metadata.is_nes2);
    REQUIRE(metadata.name_table_mirroring == NES::CARTRIDGE_MIRROR_VERTICAL);
    REQUIRE(cartridge.getROM().size() == 2 * NESTest::PRG_BANK_SIZE);
    REQUIRE(cartridge.getVROM().empty());
}

TEST_CASE("native cartridge parses NES 2.0 metadata", "[cartridge]") {
    TemporaryBytes rom("nes2_metadata", rom_bytes(
        0x123,
        1,
        0,
        "horizontal",
        false,
        false,
        false,
        false,
        false,
        0,
        true,
        0x07,
        7,
        8,
        7,
        6
    ));
    NES::Cartridge cartridge = load_cartridge(rom);
    const NES::CartridgeMetadata& metadata = cartridge.getMetadata();

    REQUIRE(metadata.is_nes2);
    REQUIRE(metadata.mapper_number == 0x123);
    REQUIRE(metadata.submapper == 0x07);
    REQUIRE(metadata.prg_rom_size == NESTest::PRG_BANK_SIZE);
    REQUIRE(metadata.prg_rom_banks == 1);
    REQUIRE(metadata.chr_rom_size == 0);
    REQUIRE(metadata.prg_ram_size == 8 * 1024);
    REQUIRE(metadata.prg_battery_ram_size == 16 * 1024);
    REQUIRE(metadata.chr_ram_size == 8 * 1024);
    REQUIRE(metadata.chr_battery_ram_size == 4 * 1024);
    REQUIRE(metadata.has_battery);
}

TEST_CASE("native cartridge rejects malformed ROM payloads", "[cartridge]") {
    TemporaryBytes invalid_magic("invalid_magic", {
        0x4e, 0x4f, 0x50, 0x45,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    });
    REQUIRE(loading_throws_with_fragment(
        invalid_magic,
        true,
        "ROM missing magic number in header."
    ));

    TemporaryBytes truncated_header("truncated_header", {
        0x4e, 0x45, 0x53, 0x1a,
    });
    REQUIRE(loading_throws_with_fragment(
        truncated_header,
        true,
        "ROM header is truncated."
    ));

    std::vector<NES::NES_Byte> nonzero_padding = ines_header(0, 1, 0);
    nonzero_padding[12] = 0x01;
    nonzero_padding.insert(
        nonzero_padding.end(),
        NESTest::PRG_BANK_SIZE,
        0xea
    );
    TemporaryBytes padding("padding", nonzero_padding);
    REQUIRE(loading_throws_with_fragment(
        padding,
        true,
        "ROM header zero fill bytes are not zero."
    ));

    TemporaryBytes truncated_prg("truncated_prg", ines_header(0, 1, 0));
    REQUIRE(loading_throws_with_fragment(
        truncated_prg,
        true,
        "failed to read PRG-ROM on ROM."
    ));

    std::vector<NES::NES_Byte> truncated_chr = ines_header(0, 1, 1);
    truncated_chr.insert(truncated_chr.end(), NESTest::PRG_BANK_SIZE, 0xea);
    TemporaryBytes chr("truncated_chr", truncated_chr);
    REQUIRE(loading_throws_with_fragment(
        chr,
        true,
        "failed to read CHR-ROM on ROM."
    ));

    TemporaryBytes no_prg("no_prg", rom_bytes(0, 0, 0));
    REQUIRE(loading_throws_with_fragment(
        no_prg,
        true,
        "ROM has no PRG-ROM banks."
    ));
}

TEST_CASE("native cartridge reports unsupported feature metadata", "[cartridge]") {
    TemporaryBytes trainer("trainer", rom_bytes(
        0,
        1,
        1,
        "horizontal",
        true
    ));
    NES::Cartridge trainer_allowed = load_cartridge(trainer, true);
    REQUIRE(trainer_allowed.getMetadata().has_trainer);
    REQUIRE(trainer_allowed.getMetadata().trainer_start == 0x10);
    REQUIRE(trainer_allowed.getMetadata().trainer_stop == 0x210);
    REQUIRE(loading_throws_with_fragment(
        trainer,
        false,
        "ROM has trainer. trainer is not supported."
    ));

    TemporaryBytes pal("pal", rom_bytes(
        0,
        1,
        1,
        "horizontal",
        false,
        false,
        false,
        false,
        true
    ));
    NES::Cartridge pal_allowed = load_cartridge(pal, true);
    REQUIRE(pal_allowed.getMetadata().is_pal);
    REQUIRE(loading_throws_with_fragment(
        pal,
        false,
        "ROM is PAL. PAL is not supported."
    ));
}

TEST_CASE("native cartridge gives four-screen priority and CHR RAM", "[cartridge]") {
    std::vector<NES::NES_Byte> bytes = ines_header(0, 1, 0, "four-screen");
    bytes[6] |= 0x01;
    bytes.insert(bytes.end(), NESTest::PRG_BANK_SIZE, 0xea);
    TemporaryBytes rom("four_screen_chr_ram", bytes);
    NES::Cartridge cartridge = load_cartridge(rom);
    const NES::CartridgeMetadata& metadata = cartridge.getMetadata();

    REQUIRE(metadata.name_table_mirroring == NES::CARTRIDGE_MIRROR_FOUR_SCREEN);
    REQUIRE(metadata.chr_ram_size == NESTest::CHR_BANK_SIZE);
}
