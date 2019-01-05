#ifndef PPU_H
#define PPU_H
#include <functional>
#include <array>
#include "picture_bus.hpp"
#include "main_bus.hpp"
#include "palette_colors.hpp"

const int ScanlineCycleLength = 341;
const int ScanlineEndCycle = 340;
const int VisibleScanlines = 240;
const int ScanlineVisibleDots = 256;
const int FrameEndScanline = 261;

const int AttributeOffset = 0x3C0;

class PPU {
public:
    /// Initialize a new PPU
    PPU() : m_spriteMemory(64 * 4) { };

    /// Perform a single cycle on the PPU
    void cycle(PictureBus& bus);

    /// Perform the number of PPU cycles that fit into a clock cycle (3)
    inline void step(PictureBus& bus) { cycle(bus); cycle(bus); cycle(bus); };

    /// Reset the PPU
    void reset();

    /// Set the interrupt callback for the CPU
    void setInterruptCallback(std::function<void(void)> cb) { m_vblankCallback = cb; };

    void doDMA(const uint8_t* page_ptr);

    //Callbacks mapped to CPU address space
    //Addresses written to by the program
    void control(uint8_t ctrl);
    void setMask(uint8_t mask);
    void setOAMAddress(uint8_t addr) { m_spriteDataAddress = addr; };
    void setDataAddress(uint8_t addr);
    void setScroll(uint8_t scroll);
    void setData(PictureBus& m_bus, uint8_t data);
    //Read by the program
    uint8_t getStatus();
    uint8_t getData(PictureBus& m_bus);
    uint8_t getOAMData() { return readOAM(m_spriteDataAddress); };
    void setOAMData(uint8_t value) { writeOAM(m_spriteDataAddress++, value); };

    /// Return a pointer to the screen buffer.
    std::uint32_t* get_screen_buffer() { return *screen_buffer; };

private:
    uint8_t readOAM(uint8_t addr) { return m_spriteMemory[addr]; };
    void writeOAM(uint8_t addr, uint8_t value) { m_spriteMemory[addr] = value; };

    std::function<void(void)> m_vblankCallback;

    std::vector<uint8_t> m_spriteMemory;

    std::vector<uint8_t> m_scanlineSprites;

    enum State
    {
        PreRender,
        Render,
        PostRender,
        VerticalBlank
    } m_pipelineState;
    int m_cycle;
    int m_scanline;
    bool m_evenFrame;

    bool m_vblank;
    bool m_sprZeroHit;

    //Registers
    Address m_dataAddress;
    Address m_tempAddress;
    uint8_t m_fineXScroll;
    bool m_firstWrite;
    uint8_t m_dataBuffer;

    uint8_t m_spriteDataAddress;

    //Setup flags and variables
    bool m_longSprites;
    bool m_generateInterrupt;

    bool m_greyscaleMode;
    bool m_showSprites;
    bool m_showBackground;
    bool m_hideEdgeSprites;
    bool m_hideEdgeBackground;

    enum CharacterPage
    {
        Low,
        High,
    } m_bgPage,
      m_sprPage;

    Address m_dataAddrIncrement;

    /// The internal screen data structure as a vector representation of a
    /// matrix of height matching the visible scans lines and width matching
    /// the number of visible scan line dots
    std::uint32_t screen_buffer[VisibleScanlines][ScanlineVisibleDots] = {{0}};

};

#endif // PPU_H
