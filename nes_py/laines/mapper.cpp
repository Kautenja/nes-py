#include "ppu.hpp"
#include "mapper.hpp"

Mapper::Mapper(u8* rom) : rom(rom) {
    // Read infos from header:
    prgSize = rom[4] * 0x4000;
    chrSize = rom[5] * 0x2000;
    prgRamSize = rom[8] ? rom[8] * 0x2000 : 0x2000;
    PPU::set_mirroring((rom[6] & 1) ? VERTICAL : HORIZONTAL);

    prg = rom + 16;
    prgRam = new u8[prgRamSize];

    // CHR ROM:
    if (chrSize) {
        chr = rom + 16 + prgSize;
        // calculate the ROM size
        romSize = (chr + chrSize) - rom;
    }
    // CHR RAM:
    else {
        chrRam = true;
        chrSize = 0x2000;
        chr = new u8[chrSize];
        // calculate the ROM size
        romSize = (rom + 16 + prgSize) - rom;
    }
}

Mapper::Mapper(Mapper* mapper) {
    // copy the ROM size and create a new array to store its data in
    romSize = mapper->romSize;
    rom = new u8[romSize];
    // copy the ROM data
    memcpy(rom, mapper->rom, romSize * sizeof(u8));
    // copy the flag for whether the mapper has CHR RAM
    chrRam = mapper->chrRam;
    // setup the PRG ROM
    prgSize = mapper->prgSize;
    prg = rom + 16;
    // setup the CHR ROM/RAM
    chrSize = mapper->chrSize;
    // CHR RAM:
    if (chrRam) {
        chr = new u8[chrSize];
        memcpy(chr, mapper->chr, chrSize * sizeof(u8));
    }
    // CHR ROM:
    else {
        chr = rom + 16 + prgSize;
    }
    // setup the PRG RAM
    prgRamSize = mapper->prgRamSize;
    prgRam = new u8[prgRamSize];
    memcpy(prgRam, mapper->prgRam, prgRamSize * sizeof(u8));
    // copy the maps
    std::copy(std::begin(mapper->prgMap), std::end(mapper->prgMap), std::begin(prgMap));
    std::copy(std::begin(mapper->chrMap), std::end(mapper->chrMap), std::begin(chrMap));
}

Mapper* Mapper::copy() {
    return new Mapper(this);
}

Mapper::~Mapper() {
    delete rom;
    delete prgRam;
    if (chrRam)
        delete chr;
}

/* Access to memory */
u8 Mapper::read(u16 addr) {
    if (addr >= 0x8000)
        return prg[prgMap[(addr - 0x8000) / 0x2000] + ((addr - 0x8000) % 0x2000)];
    else
        return prgRam[addr - 0x6000];
}

u8 Mapper::chr_read(u16 addr) {
    return chr[chrMap[addr / 0x400] + (addr % 0x400)];
}

/* PRG mapping functions */
template <int pageKBs> void Mapper::map_prg(int slot, int bank) {
    if (bank < 0)
        bank = (prgSize / (0x400*pageKBs)) + bank;

    for (int i = 0; i < (pageKBs/8); i++)
        prgMap[(pageKBs/8) * slot + i] = (pageKBs*0x400*bank + 0x2000*i) % prgSize;
}
template void Mapper::map_prg<32>(int, int);
template void Mapper::map_prg<16>(int, int);
template void Mapper::map_prg<8> (int, int);

/* CHR mapping functions */
template <int pageKBs> void Mapper::map_chr(int slot, int bank) {
    for (int i = 0; i < pageKBs; i++)
        chrMap[pageKBs*slot + i] = (pageKBs*0x400*bank + 0x400*i) % chrSize;
}
template void Mapper::map_chr<8>(int, int);
template void Mapper::map_chr<4>(int, int);
template void Mapper::map_chr<2>(int, int);
template void Mapper::map_chr<1>(int, int);
