#ifndef MAPPERSXROM_H
#define MAPPERSXROM_H
#include "mapper.hpp"

class MapperSxROM : public Mapper {

private:
    std::function<void(void)> m_mirroringCallback;
    NameTableMirroring m_mirroing;
    bool m_usesCharacterRAM;
    int m_modeCHR;
    int m_modePRG;
    uint8_t m_tempRegister;
    int m_writeCounter;
    uint8_t m_regPRG;
    uint8_t m_regCHR0;
    uint8_t m_regCHR1;
    const uint8_t* m_firstBankPRG;
    const uint8_t* m_secondBankPRG;
    const uint8_t* m_firstBankCHR;
    const uint8_t* m_secondBankCHR;
    std::vector<uint8_t> m_characterRAM;
    void calculatePRGPointers();

public:
    MapperSxROM(Cartridge& cart, std::function<void(void)> mirroring_cb);
    void writePRG (uint16_t addr, uint8_t value);
    uint8_t readPRG (uint16_t addr);
    const uint8_t* getPagePtr(uint16_t addr);
    uint8_t readCHR (uint16_t addr);
    void writeCHR (uint16_t addr, uint8_t value);
    NameTableMirroring getNameTableMirroring() { return m_mirroing; };

};

#endif // MAPPERSXROM_H
