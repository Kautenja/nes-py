//  Program:      nes-py
//  File:         synthetic_rom.hpp
//  Description:  Temporary iNES ROM fixtures for native mapper tests
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#ifndef NES_EMU_TEST_SUPPORT_SYNTHETIC_ROM_HPP
#define NES_EMU_TEST_SUPPORT_SYNTHETIC_ROM_HPP

#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "nes_emu/cartridge.hpp"
#include "nes_emu/common.hpp"

namespace NESTest {

const std::size_t PRG_BANK_SIZE = 0x4000;
const std::size_t CHR_BANK_SIZE = 0x2000;

inline NES::NES_Byte prg_bank_marker(std::size_t bank) {
    return static_cast<NES::NES_Byte>(((bank + 1) * 0x11) & 0xff);
}

inline NES::NES_Byte prg_page_marker(std::size_t page) {
    return static_cast<NES::NES_Byte>((0x30 + page) & 0xff);
}

inline NES::NES_Byte chr_bank_marker(std::size_t bank) {
    return static_cast<NES::NES_Byte>((0x80 + bank) & 0xff);
}

inline NES::NES_Byte chr_page_marker(std::size_t page) {
    return static_cast<NES::NES_Byte>((0xc0 + page) & 0xff);
}

class TemporaryROM {
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

    static std::vector<NES::NES_Byte> ines_header(
        std::uint16_t mapper,
        std::size_t prg_banks,
        std::size_t chr_banks,
        const std::string& mirroring
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

        return {
            0x4e, 0x45, 0x53, 0x1a,
            static_cast<NES::NES_Byte>(prg_banks & 0xff),
            static_cast<NES::NES_Byte>(chr_banks & 0xff),
            flags_6,
            static_cast<NES::NES_Byte>(mapper & 0xf0),
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
        };
    }

 public:
    TemporaryROM(
        const std::string& name,
        std::uint16_t mapper,
        std::size_t prg_banks,
        std::size_t chr_banks,
        const std::string& mirroring = "horizontal",
        bool chr_4k_markers = false,
        bool prg_8k_markers = false
    ) : path(temporary_path(name)) {
        std::vector<NES::NES_Byte> bytes = ines_header(
            mapper,
            prg_banks,
            chr_banks,
            mirroring
        );

        std::vector<NES::NES_Byte> prg;
        if (prg_8k_markers) {
            for (std::size_t page = 0; page < prg_banks * 2; ++page) {
                prg.insert(
                    prg.end(),
                    PRG_BANK_SIZE / 2,
                    prg_page_marker(page)
                );
            }
        } else {
            for (std::size_t bank = 0; bank < prg_banks; ++bank) {
                prg.insert(
                    prg.end(),
                    PRG_BANK_SIZE,
                    prg_bank_marker(bank)
                );
            }
        }
        if (!prg.empty()) {
            prg[0] = 0xea;
            prg[1] = 0x4c;
            prg[2] = 0x00;
            prg[3] = prg_banks > 1 ? 0xc0 : 0x80;
            prg[prg.size() - 4] = 0x00;
            prg[prg.size() - 3] = prg_banks > 1 ? 0xc0 : 0x80;
        }
        bytes.insert(bytes.end(), prg.begin(), prg.end());

        if (chr_4k_markers) {
            for (std::size_t page = 0; page < chr_banks * 2; ++page) {
                bytes.insert(
                    bytes.end(),
                    CHR_BANK_SIZE / 2,
                    chr_page_marker(page)
                );
            }
        } else {
            for (std::size_t bank = 0; bank < chr_banks; ++bank) {
                bytes.insert(
                    bytes.end(),
                    CHR_BANK_SIZE,
                    chr_bank_marker(bank)
                );
            }
        }

        std::ofstream output(path.c_str(), std::ios::binary);
        output.write(
            reinterpret_cast<const char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size())
        );
        if (!output)
            throw std::runtime_error("failed to write synthetic ROM");
    }

    ~TemporaryROM() {
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

}  // namespace NESTest

#endif  // NES_EMU_TEST_SUPPORT_SYNTHETIC_ROM_HPP
