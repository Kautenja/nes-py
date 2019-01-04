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

class PPU
{
public:
    PPU(PictureBus &bus);
    void step();
    void reset();

    void setInterruptCallback(std::function<void(void)> cb);

    void doDMA(const Byte* page_ptr);

    //Callbacks mapped to CPU address space
    //Addresses written to by the program
    void control(Byte ctrl);
    void setMask(Byte mask);
    void setOAMAddress(Byte addr);
    void setDataAddress(Byte addr);
    void setScroll(Byte scroll);
    void setData(Byte data);
    //Read by the program
    Byte getStatus();
    Byte getData();
    Byte getOAMData();
    void setOAMData(Byte value);

    /// Return a pointer to the screen buffer.
    std::uint32_t* get_screen_buffer() { return *screen_buffer; };

private:
    Byte readOAM(Byte addr);
    void writeOAM(Byte addr, Byte value);
    Byte read(Address addr);
    PictureBus &m_bus;

    std::function<void(void)> m_vblankCallback;

    std::vector<Byte> m_spriteMemory;

    std::vector<Byte> m_scanlineSprites;

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
    Byte m_fineXScroll;
    bool m_firstWrite;
    Byte m_dataBuffer;

    Byte m_spriteDataAddress;

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
