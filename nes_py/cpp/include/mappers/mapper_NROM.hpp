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
    MapperNROM(const MapperNROM& other, Cartridge& cart);

    /// Create a clone of this mapper.
    ///
    /// @param cartridge the cartridge to create a clone of the mapper with
    ///
    Mapper* clone(Cartridge& cartridge) const;

    /// Read a byte from the PRG RAM.
    ///
    /// @param addr the 16-bit address of the byte to read
    /// @return the byte located at the given address in PRG RAM
    ///
    uint8_t readPRG (uint16_t addr);

    /// Write a byte to an address in the PRG RAM.
    ///
    /// @param addr the 16-bit address to write to
    /// @param value the byte to write to the given address
    ///
    void writePRG (uint16_t addr, uint8_t value);

    /// Read a byte from the CHR RAM.
    ///
    /// @param addr the 16-bit address of the byte to read
    /// @return the byte located at the given address in CHR RAM
    ///
    uint8_t readCHR (uint16_t addr);

    /// Write a byte to an address in the CHR RAM.
    ///
    /// @param addr the 16-bit address to write to
    /// @param value the byte to write to the given address
    ///
    void writeCHR (uint16_t addr, uint8_t value);

    /// Return the page pointer for the given address.
    ///
    /// @param addr the address of the page pointer to get
    /// @return the page pointer at the given address
    ///
    const uint8_t* getPagePtr(uint16_t addr);

};

#endif // MAPPERNROM_H
