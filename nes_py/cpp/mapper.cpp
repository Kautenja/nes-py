#include "mapper.hpp"
#include "mappers/mapper_NROM.hpp"
#include "mappers/mapper_SxROM.hpp"
#include "mappers/mapper_UxROM.hpp"
#include "mappers/mapper_CNROM.hpp"

NameTableMirroring Mapper::getNameTableMirroring() {
    return static_cast<NameTableMirroring>(m_cartridge.getNameTableMirroring());
}

bool Mapper::hasExtendedRAM() {
    return m_cartridge.hasExtendedRAM();
}

Mapper* Mapper::createMapper(
    Mapper::Type mapper_t,
    Cartridge& cart,
    std::function<void(void)> mirroring_cb
) {
    switch (mapper_t) {
        case NROM:
            return new MapperNROM(cart);
        case SxROM:
            return new MapperSxROM(cart, mirroring_cb);
        case UxROM:
            return new MapperUxROM(cart);
        case CNROM:
            return new MapperCNROM(cart);
        default:
            return nullptr;
    }
}
