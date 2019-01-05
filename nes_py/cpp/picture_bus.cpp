#include "picture_bus.hpp"
#include "log.hpp"

uint8_t PictureBus::read(Address address) {
    if (address < 0x2000) {
        return m_mapper->readCHR(address);
    }
    // Name tables up to 0x3000, then mirrored up to 0x3ff
    else if (address < 0x3eff) {
        auto index = address & 0x3ff;
        // NT0
        if (address < 0x2400)
            return m_RAM[NameTable0 + index];
        // NT1
        else if (address < 0x2800)
            return m_RAM[NameTable1 + index];
        // NT2
        else if (address < 0x2c00)
            return m_RAM[NameTable2 + index];
        // NT3
        else
            return m_RAM[NameTable3 + index];
    }
    else if (address < 0x3fff) {
        return m_palette[address & 0x1f];
    }
    return 0;
}

void PictureBus::write(Address address, uint8_t value) {
    if (address < 0x2000) {
        m_mapper->writeCHR(address, value);
    }
    // Name tables up to 0x3000, then mirrored up to 0x3ff
    else if (address < 0x3eff)  {
        auto index = address & 0x3ff;
        // NT0
        if (address < 0x2400)
            m_RAM[NameTable0 + index] = value;
        // NT1
        else if (address < 0x2800)
            m_RAM[NameTable1 + index] = value;
        // NT2
        else if (address < 0x2c00)
            m_RAM[NameTable2 + index] = value;
        // NT3
        else
            m_RAM[NameTable3 + index] = value;
    }
    else if (address < 0x3fff) {
        if (address == 0x3f10)
            m_palette[0] = value;
        else
            m_palette[address & 0x1f] = value;
   }
}

void PictureBus::update_mirroring() {
    switch (m_mapper->getNameTableMirroring()) {
        case Horizontal:
            NameTable0 = NameTable1 = 0;
            NameTable2 = NameTable3 = 0x400;
            LOG(InfoVerbose) <<
                "Horizontal Name Table mirroring set. (Vertical Scrolling)" <<
                std::endl;
            break;
        case Vertical:
            NameTable0 = NameTable2 = 0;
            NameTable1 = NameTable3 = 0x400;
            LOG(InfoVerbose) <<
                "Vertical Name Table mirroring set. (Horizontal Scrolling)" <<
                std::endl;
            break;
        case OneScreenLower:
            NameTable0 = NameTable1 = NameTable2 = NameTable3 = 0;
            LOG(InfoVerbose) <<
                "Single Screen mirroring set with lower bank." <<
                std::endl;
            break;
        case OneScreenHigher:
            NameTable0 = NameTable1 = NameTable2 = NameTable3 = 0x400;
            LOG(InfoVerbose) <<
                "Single Screen mirroring set with higher bank." <<
                std::endl;
            break;
        default:
            NameTable0 = NameTable1 = NameTable2 = NameTable3 = 0;
            LOG(Error) <<
                "Unsupported Name Table mirroring : " <<
                m_mapper->getNameTableMirroring() <<
                std::endl;
    }
}

bool PictureBus::set_mapper(Mapper *mapper) {
    // if the mapper is a null pointer, raise an error
    if (!mapper) {
        LOG(Error) << "Mapper argument is nullptr" << std::endl;
        return false;
    }
    // set the mapper and update the mirroring mode
    m_mapper = mapper;
    update_mirroring();
    return true;
}
