//
// Created by budby on 4/12/2026.
//

#include "SyntheticRTTISystem.hpp"

namespace App
{

SClass::SClass(Red::CName aName, uint32_t aSize, Flags aFlags) noexcept
    : Red::CClass(aName, aSize, aFlags)
{

}

void SClass::sub_C0()
{
}

uint32_t SClass::GetMaxAlignment() const
{
    return {};
}

bool SClass::sub_D0() const
{
    return {};
}

void* SClass::AllocMemory() const
{
    return {};
}

bool SClass::sub_78()
{
    return {};
}

bool SClass::sub_A8()
{
    return {};
}

} // namespace App