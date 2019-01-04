#include "mapper.hpp"
#include "mappers/mapper_NROM.hpp"
#include "mappers/mapper_SxROM.hpp"
#include "mappers/mapper_UxROM.hpp"
#include "mappers/mapper_CNROM.hpp"

NameTableMirroring Mapper::getNameTableMirroring()
{
    return static_cast<NameTableMirroring>(m_cartridge.getNameTableMirroring());
}

std::unique_ptr<Mapper> Mapper::createMapper(Mapper::Type mapper_t, Cartridge& cart, std::function<void(void)> mirroring_cb)
{
    std::unique_ptr<Mapper> ret(nullptr);
    switch (mapper_t)
    {
        case NROM:
            ret.reset(new MapperNROM(cart));
            break;
        case SxROM:
            ret.reset(new MapperSxROM(cart, mirroring_cb));
            break;
        case UxROM:
            ret.reset(new MapperUxROM(cart));
            break;
        case CNROM:
            ret.reset(new MapperCNROM(cart));
            break;
        default:
            break;
    }
    return ret;
}
