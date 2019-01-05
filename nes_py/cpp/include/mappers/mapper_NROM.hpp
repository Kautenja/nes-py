#ifndef MAPPERNROM_H
#define MAPPERNROM_H
#include "mapper.hpp"

class MapperNROM : public Mapper {

private:
    /// whether there are 1 or 2 banks
    bool m_oneBank;
    /// whether this mapper uses character RAM
    bool m_usesCharacterRAM;
    /// the character RAM on the mapper
    std::vector<uint8_t> m_characterRAM;

public:

    /// Create a new mapper with a cartridge.
    ///
    /// @param cart a reference to a cartridge for the mapper to access
    ///
    MapperNROM(Cartridge& cart);

    /// Create a new mapper as a copy of another mapper
    ///
    /// @param other the other mapper to clone data from
    /// @param cart the new cartridge to associate with
    ///
    MapperNROM(const MapperNROM other, Cartridge& cart);

    /// Create a clone of this mapper.
    ///
    /// @param cartridge the cartridge to create a clone of the mapper with
    ///
    Mapper* clone(Cartridge& cartridge) const;

    void writePRG (uint16_t addr, uint8_t value);
    uint8_t readPRG (uint16_t addr);
    const uint8_t* getPagePtr(uint16_t addr);
    uint8_t readCHR (uint16_t addr);
    void writeCHR (uint16_t addr, uint8_t value);

};

#endif // MAPPERNROM_H
