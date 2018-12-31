#include <cstdio>
#include "cartridge.hpp"
#include "mappers/mapper0.hpp"
#include "mappers/mapper1.hpp"
#include "mappers/mapper2.hpp"
#include "mappers/mapper3.hpp"
#include "mappers/mapper4.hpp"

Cartridge::Cartridge(const char* file_name) {
    // open the ROM file as a binary sequence
    FILE* f = fopen(file_name, "rb");
    // determine the size of the ROM file
    fseek(f, 0, SEEK_END);
    int size = ftell(f);
    fseek(f, 0, SEEK_SET);
    // read the ROM into a local array
    u8* rom = new u8[size];
    fread(rom, size, 1, f);
    fclose(f);
    // determine the mapper number from the iNES header
    int mapper_id = (rom[7] & 0xF0) | (rom[6] >> 4);
    // setup the new mapper
    switch (mapper_id) {
        case 0:  this->mapper = new Mapper0(rom); break;
        case 1:  this->mapper = new Mapper1(rom); break;
        case 2:  this->mapper = new Mapper2(rom); break;
        case 3:  this->mapper = new Mapper3(rom); break;
        case 4:  this->mapper = new Mapper4(rom); break;
    }
}

Cartridge::Cartridge(Cartridge* cart) {
    mapper = cart->mapper->copy();
};

Cartridge::~Cartridge() {
    delete this->mapper;
}

void Cartridge::signal_scanline() {
    this->mapper->signal_scanline();
}

template <bool wr> u8 Cartridge::access(u16 addr, u8 v) {
    if (!wr) return this->mapper->read(addr);
    else     return this->mapper->write(addr, v);
}
template u8 Cartridge::access<0>(u16, u8);
template u8 Cartridge::access<1>(u16, u8);

template <bool wr> u8 Cartridge::chr_access(u16 addr, u8 v) {
    if (!wr) return this->mapper->chr_read(addr);
    else     return this->mapper->chr_write(addr, v);
}
template u8 Cartridge::chr_access<0>(u16, u8);
template u8 Cartridge::chr_access<1>(u16, u8);
