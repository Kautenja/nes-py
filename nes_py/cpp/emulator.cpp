#include "emulator.hpp"
#include "log.hpp"
#include <thread>
#include <chrono>

Emulator::Emulator(std::string rom_path) : cpu(), ppu(), rom_path(rom_path) {
    // raise an error if IO callback setup fails
    if(
        !bus.setReadCallback(PPUSTATUS, [&](void) {return ppu.getStatus();}) ||
        !bus.setReadCallback(PPUDATA, [&](void) {return ppu.getData(picture_bus);}) ||
        !bus.setReadCallback(JOY1, [&](void) {return controller1.read();}) ||
        !bus.setReadCallback(JOY2, [&](void) {return controller2.read();}) ||
        !bus.setReadCallback(OAMDATA, [&](void) {return ppu.getOAMData();})
    ) {
        LOG(Error) << "Critical error: Failed to set I/O callbacks" << std::endl;
    }
    // raise an error if IO callback setup fails
    if(
        !bus.setWriteCallback(PPUCTRL, [&](Byte b) {ppu.control(b);}) ||
        !bus.setWriteCallback(PPUMASK, [&](Byte b) {ppu.setMask(b);}) ||
        !bus.setWriteCallback(OAMADDR, [&](Byte b) {ppu.setOAMAddress(b);}) ||
        !bus.setWriteCallback(PPUADDR, [&](Byte b) {ppu.setDataAddress(b);}) ||
        !bus.setWriteCallback(PPUSCROL, [&](Byte b) {ppu.setScroll(b);}) ||
        !bus.setWriteCallback(PPUDATA, [&](Byte b) {ppu.setData(picture_bus, b);}) ||
        !bus.setWriteCallback(OAMDMA, [&](Byte b) {DMA(b);}) ||
        !bus.setWriteCallback(JOY1, [&](Byte b) {controller1.strobe(b); controller2.strobe(b);}) ||
        !bus.setWriteCallback(OAMDATA, [&](Byte b) {ppu.setOAMData(b);})
    ) {
        LOG(Error) << "Critical error: Failed to set I/O callbacks" << std::endl;
    }
    // set the interrupt callback for the PPU
    ppu.setInterruptCallback([&](){ cpu.interrupt(bus, CPU::NMI); });
}

Emulator::Emulator(Emulator* emulator) {
    rom_path = emulator->rom_path;
    bus = emulator->bus;
    picture_bus = emulator->picture_bus;
    cartridge = emulator->cartridge;
    // mapper = emulator->mapper;
    controller1 = emulator->controller1;
    controller2 = emulator->controller2;
    cpu = emulator->cpu;
    ppu = emulator->ppu;
}

void Emulator::loadRom() {
    if (!cartridge.loadFromFile(rom_path))
        return;

    mapper = Mapper::createMapper(
        static_cast<Mapper::Type>(cartridge.getMapper()),
        cartridge,
        [&](){ picture_bus.updateMirroring(); }
    );

    if (!mapper) {
        LOG(Error) << "Creating Mapper failed. Probably unsupported." << std::endl;
        return;
    }

    if (!bus.setMapper(mapper) || !picture_bus.setMapper(mapper))
        return;

    cpu.reset(bus);
    ppu.reset();
}

void Emulator::DMA(Byte page) {
    cpu.skipDMACycles();
    auto page_ptr = bus.getPagePtr(page);
    ppu.doDMA(page_ptr);
}

uint32_t* Emulator::get_screen_buffer() {
    return ppu.get_screen_buffer();
}

void Emulator::reset() {
    loadRom();
}

void Emulator::step(unsigned char action) {
    controller1.write_buttons(action);
    // approximate a frame
    for (int i = 0; i < 29781; i++) {
        // PPU steps 3 times per clock cycle
        ppu.step(picture_bus);
        ppu.step(picture_bus);
        ppu.step(picture_bus);
        // CPU steps once per clock cycle (obviously)
        cpu.step(bus);
    }
}

void Emulator::backup() {
    // TODO:
}

void Emulator::restore() {
    // TODO:
}

uint8_t Emulator::read_memory(uint16_t address) {
    return bus.read(address);
}

void Emulator::write_memory(uint16_t address, uint8_t value) {
    bus.write(address, value);
}
