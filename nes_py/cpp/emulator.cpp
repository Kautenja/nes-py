#include "emulator.hpp"
#include "log.hpp"
#include <thread>
#include <chrono>

Emulator::Emulator(std::string rom_path) :
    cpu(bus),
    ppu(picture_bus, emulator_screen),
    screen_scale(1.f),
    cycle_timer(),
    cpu_cycle_duration(std::chrono::nanoseconds(559)),
    rom_path(rom_path) {
    // raise an error if IO callback setup fails
    if(!bus.setReadCallback(PPUSTATUS, [&](void) {return ppu.getStatus();}) ||
        !bus.setReadCallback(PPUDATA, [&](void) {return ppu.getData();}) ||
        !bus.setReadCallback(JOY1, [&](void) {return controller1.read();}) ||
        !bus.setReadCallback(JOY2, [&](void) {return controller2.read();}) ||
        !bus.setReadCallback(OAMDATA, [&](void) {return ppu.getOAMData();})
    )
        LOG(Error) << "Critical error: Failed to set I/O callbacks" << std::endl;
    // raise an error if IO callback setup fails
    if(!bus.setWriteCallback(PPUCTRL, [&](Byte b) {ppu.control(b);}) ||
        !bus.setWriteCallback(PPUMASK, [&](Byte b) {ppu.setMask(b);}) ||
        !bus.setWriteCallback(OAMADDR, [&](Byte b) {ppu.setOAMAddress(b);}) ||
        !bus.setWriteCallback(PPUADDR, [&](Byte b) {ppu.setDataAddress(b);}) ||
        !bus.setWriteCallback(PPUSCROL, [&](Byte b) {ppu.setScroll(b);}) ||
        !bus.setWriteCallback(PPUDATA, [&](Byte b) {ppu.setData(b);}) ||
        !bus.setWriteCallback(OAMDMA, [&](Byte b) {DMA(b);}) ||
        !bus.setWriteCallback(JOY1, [&](Byte b) {controller1.strobe(b); controller2.strobe(b);}) ||
        !bus.setWriteCallback(OAMDATA, [&](Byte b) {ppu.setOAMData(b);}))
        LOG(Error) << "Critical error: Failed to set I/O callbacks" << std::endl;
    // set the interrupt callback for the PPU
    ppu.setInterruptCallback([&](){ cpu.interrupt(CPU::NMI); });
}

void Emulator::loadRom() {
    if (!cartridge.loadFromFile(rom_path))
        return;

    mapper = Mapper::createMapper(static_cast<Mapper::Type>(cartridge.getMapper()),
                                    cartridge,
                                    [&](){ picture_bus.updateMirroring(); });
    if (!mapper) {
        LOG(Error) << "Creating Mapper failed. Probably unsupported." << std::endl;
        return;
    }

    if (!bus.setMapper(mapper.get()) || !picture_bus.setMapper(mapper.get()))
        return;

    cpu.reset();
    ppu.reset();

    window.create(sf::VideoMode(NESVideoWidth * screen_scale, NESVideoHeight * screen_scale),
                    "SimpleNES", sf::Style::Titlebar | sf::Style::Close);
    window.setVerticalSyncEnabled(true);
    emulator_screen.create(NESVideoWidth, NESVideoHeight, screen_scale, sf::Color::White);

    cycle_timer = std::chrono::high_resolution_clock::now();
    elapsed_time = cycle_timer - cycle_timer;
}

void Emulator::DMA(Byte page) {
    cpu.skipDMACycles();
    auto page_ptr = bus.getPagePtr(page);
    ppu.doDMA(page_ptr);
}

uint32_t* Emulator::get_screen_buffer() {
    // TODO:
}

void Emulator::reset() {
    // TODO:
}

void Emulator::step(unsigned char action) {
    // TODO:
}

void Emulator::backup() {
    // TODO:
}

void Emulator::restore() {
    // TODO:
}

uint8_t Emulator::read_memory(uint16_t address) {
    // TODO:
}

void Emulator::write_memory(uint16_t address, uint8_t value) {
    // TODO:
}

// TODO: Delete this function
void Emulator::run() {
    loadRom();
    sf::Event event;
    bool focus = true, pause = false;
    while (window.isOpen()) {
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed ||
            (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)) {
                window.close();
                return;
            }
            else if (event.type == sf::Event::GainedFocus) {
                focus = true;
                cycle_timer = std::chrono::high_resolution_clock::now();
            }
            else if (event.type == sf::Event::LostFocus)
                focus = false;
            else if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::F2) {
                pause = !pause;
                if (!pause)
                    cycle_timer = std::chrono::high_resolution_clock::now();
            }
            else if (pause && event.type == sf::Event::KeyReleased && event.key.code == sf::Keyboard::F3) {
                //Around one frame
                for (int i = 0; i < 29781; ++i) {
                    //PPU
                    ppu.step();
                    ppu.step();
                    ppu.step();
                    //CPU
                    cpu.step();
                }
            }
            else if (focus && event.type == sf::Event::KeyReleased && event.key.code == sf::Keyboard::F4)
                Log::get().setLevel(Info);
            else if (focus && event.type == sf::Event::KeyReleased && event.key.code == sf::Keyboard::F5)
                Log::get().setLevel(InfoVerbose);
            else if (focus && event.type == sf::Event::KeyReleased && event.key.code == sf::Keyboard::F6)
                Log::get().setLevel(CpuTrace);
        }

        if (focus && !pause) {
            elapsed_time += std::chrono::high_resolution_clock::now() - cycle_timer;
            cycle_timer = std::chrono::high_resolution_clock::now();

            while (elapsed_time > cpu_cycle_duration) {
                //PPU
                ppu.step();
                ppu.step();
                ppu.step();
                //CPU
                cpu.step();

                elapsed_time -= cpu_cycle_duration;
            }

            window.draw(emulator_screen);
            window.display();
        }
        else
            sf::sleep(sf::milliseconds(1000/60));
    }
}
