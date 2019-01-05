#ifndef MAPPER_H
#define MAPPER_H
#include "cartridge.hpp"
#include <memory>
#include <functional>

enum NameTableMirroring {
    Horizontal  = 0,
    Vertical    = 1,
    FourScreen  = 8,
    OneScreenLower,
    OneScreenHigher,
};

class Mapper {
public:
    enum Type {
        NROM  = 0,
        SxROM = 1,
        UxROM = 2,
        CNROM = 3,
    };

    Mapper(Cartridge& cart, Type t) : m_cartridge(cart), m_type(t) {};
    virtual void writePRG (Address addr, uint8_t value) = 0;
    virtual uint8_t readPRG (Address addr) = 0;
    virtual const uint8_t* getPagePtr (Address addr) = 0; //for DMAs

    virtual uint8_t readCHR (Address addr) = 0;
    virtual void writeCHR (Address addr, uint8_t value) = 0;

    virtual NameTableMirroring getNameTableMirroring() {
        return static_cast<NameTableMirroring>(m_cartridge.getNameTableMirroring());
    };

    bool hasExtendedRAM() { return m_cartridge.hasExtendedRAM(); };

    static Mapper* createMapper (
        Type mapper_t,
        Cartridge& cart,
        std::function<void(void)> mirroring_cb
    );

protected:
    Cartridge& m_cartridge;
    Type m_type;
};

#endif //MAPPER_H
