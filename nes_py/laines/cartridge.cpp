#include <cstdio>
#include "cartridge.hpp"
#include "mappers/mapper0.hpp"
#include "mappers/mapper1.hpp"
#include "mappers/mapper2.hpp"
#include "mappers/mapper3.hpp"
#include "mappers/mapper4.hpp"

Cartridge::Cartridge() {
    this->mapper = nullptr;
}

Cartridge::~Cartridge() {
    delete this->mapper;
}

void Cartridge::load(const char* fileName) {
    FILE* f = fopen(fileName, "rb");

    fseek(f, 0, SEEK_END);
    int size = ftell(f);
    fseek(f, 0, SEEK_SET);

    u8* rom = new u8[size];
    fread(rom, size, 1, f);
    fclose(f);

    int mapperNum = (rom[7] & 0xF0) | (rom[6] >> 4);
    // delete the existing mapper if their is one
    if (loaded()) delete this->mapper;
    // setup the new mapper
    switch (mapperNum) {
        case 0:  this->mapper = new Mapper0(rom); break;
        case 1:  this->mapper = new Mapper1(rom); break;
        case 2:  this->mapper = new Mapper2(rom); break;
        case 3:  this->mapper = new Mapper3(rom); break;
        case 4:  this->mapper = new Mapper4(rom); break;
    }
}

bool Cartridge::loaded() {
    return this->mapper != nullptr;
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
