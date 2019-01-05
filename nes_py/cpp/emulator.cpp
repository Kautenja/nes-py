#include "emulator.hpp"
#include "log.hpp"

Emulator::Emulator(std::string rom_path) : cpu(), ppu(), rom_path(rom_path) {
    // raise an error if IO callback setup fails
    if (
        !bus.set_read_callback(PPUSTATUS, [&](void) {return ppu.getStatus();}) ||
        !bus.set_read_callback(PPUDATA, [&](void) {return ppu.getData(picture_bus);}) ||
        !bus.set_read_callback(JOY1, [&](void) {return controller1.read();}) ||
        !bus.set_read_callback(JOY2, [&](void) {return controller2.read();}) ||
        !bus.set_read_callback(OAMDATA, [&](void) {return ppu.getOAMData();})
    ) {
        LOG(Error) << "Critical error: Failed to set I/O callbacks" << std::endl;
    }
    // raise an error if IO callback setup fails
    if (
        !bus.set_write_callback(PPUCTRL, [&](Byte b) {ppu.control(b);}) ||
        !bus.set_write_callback(PPUMASK, [&](Byte b) {ppu.setMask(b);}) ||
        !bus.set_write_callback(OAMADDR, [&](Byte b) {ppu.setOAMAddress(b);}) ||
        !bus.set_write_callback(PPUADDR, [&](Byte b) {ppu.setDataAddress(b);}) ||
        !bus.set_write_callback(PPUSCROL, [&](Byte b) {ppu.setScroll(b);}) ||
        !bus.set_write_callback(PPUDATA, [&](Byte b) {ppu.setData(picture_bus, b);}) ||
        !bus.set_write_callback(OAMDMA, [&](Byte b) {DMA(b);}) ||
        !bus.set_write_callback(JOY1, [&](Byte b) {controller1.strobe(b); controller2.strobe(b);}) ||
        !bus.set_write_callback(OAMDATA, [&](Byte b) {ppu.setOAMData(b);})
    ) {
        LOG(Error) << "Critical error: Failed to set I/O callbacks" << std::endl;
    }
    // set the interrupt callback for the PPU
    ppu.setInterruptCallback([&](){ cpu.interrupt(bus, CPU::NMI); });
}

Emulator::Emulator(Emulator* emulator) {
    rom_path = emulator->rom_path;
    controller1 = emulator->controller1;
    controller2 = emulator->controller2;
    cpu = emulator->cpu;
    ppu = emulator->ppu;
    bus = emulator->bus;
    picture_bus = emulator->picture_bus;
    cartridge = emulator->cartridge;
    // mapper = emulator->mapper;
    // bus.setMapper(mapper);
    // picture_bus.setMapper(mapper);
    // raise an error if IO callback setup fails
    if (
        !bus.set_read_callback(PPUSTATUS, [&](void) {return ppu.getStatus();}) ||
        !bus.set_read_callback(PPUDATA, [&](void) {return ppu.getData(picture_bus);}) ||
        !bus.set_read_callback(JOY1, [&](void) {return controller1.read();}) ||
        !bus.set_read_callback(JOY2, [&](void) {return controller2.read();}) ||
        !bus.set_read_callback(OAMDATA, [&](void) {return ppu.getOAMData();})
    ) {
        LOG(Error) << "Critical error: Failed to set I/O callbacks" << std::endl;
    }
    // raise an error if IO callback setup fails
    if (
        !bus.set_write_callback(PPUCTRL, [&](Byte b) {ppu.control(b);}) ||
        !bus.set_write_callback(PPUMASK, [&](Byte b) {ppu.setMask(b);}) ||
        !bus.set_write_callback(OAMADDR, [&](Byte b) {ppu.setOAMAddress(b);}) ||
        !bus.set_write_callback(PPUADDR, [&](Byte b) {ppu.setDataAddress(b);}) ||
        !bus.set_write_callback(PPUSCROL, [&](Byte b) {ppu.setScroll(b);}) ||
        !bus.set_write_callback(PPUDATA, [&](Byte b) {ppu.setData(picture_bus, b);}) ||
        !bus.set_write_callback(OAMDMA, [&](Byte b) {DMA(b);}) ||
        !bus.set_write_callback(JOY1, [&](Byte b) {controller1.strobe(b); controller2.strobe(b);}) ||
        !bus.set_write_callback(OAMDATA, [&](Byte b) {ppu.setOAMData(b);})
    ) {
        LOG(Error) << "Critical error: Failed to set I/O callbacks" << std::endl;
    }
    // set the interrupt callback for the PPU
    ppu.setInterruptCallback([&](){ cpu.interrupt(bus, CPU::NMI); });
}

void Emulator::reset() {
    // load the ROM from disk and return if the operation fails
    if (!cartridge.loadFromFile(rom_path))
        return;
    // create the mapper based on the mapper ID in the iNES header of the ROM
    mapper = Mapper::createMapper(
        static_cast<Mapper::Type>(cartridge.getMapper()),
        cartridge,
        [&](){ picture_bus.update_mirroring(); }
    );
    // if the mapper is a nullptr, the mapper ID is not supported at this time
    if (!mapper) {
        LOG(Error) << "Creating Mapper failed. Probably unsupported." << std::endl;
        return;
    }
    // if setting the mapper on the buses fails, return
    if (!bus.set_mapper(mapper) || !picture_bus.set_mapper(mapper))
        return;
    // reset the CPU and PPU
    cpu.reset(bus);
    ppu.reset();
}

void Emulator::DMA(Byte page) {
    // skip the DMA cycles on the CPU
    cpu.skipDMACycles();
    // get the pointer to the next page from the bus
    auto page_ptr = bus.get_page_pointer(page);
    // do the DMA page change on the PPU
    ppu.doDMA(page_ptr);
}

void Emulator::step(unsigned char action) {
    // write the controller state to player 1
    controller1.write_buttons(action);
    // approximate a frame
    for (int i = 0; i < 29781; i++) {
        ppu.step(picture_bus);
        cpu.step(bus);
    }
}
