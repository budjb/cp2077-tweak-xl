#include "ScriptableRecordClass.hpp"
#include "ScriptableTweakDBRecord.hpp"

namespace
{
constexpr auto ScriptableRecordSize = sizeof(Red::ScriptableTweakDBRecord);
constexpr auto ScriptableRecordAlignment = alignof(Red::ScriptableTweakDBRecord);
} // namespace

namespace App
{
RecordClass::RecordClass(Red::CName aName, const uint32_t aHash)
    : CClass(aName, ScriptableRecordSize, {.isNative = true})
    , tweakBaseHash(aHash)
{
    alignment = ScriptableRecordAlignment;
}

void RecordClass::ConstructCls(void* aMemory) const
{
    new (aMemory) Red::ScriptableTweakDBRecord(this);
}

void RecordClass::DestructCls(void* aMemory) const
{
    static_cast<Red::ScriptableTweakDBRecord*>(aMemory)->~ScriptableTweakDBRecord();
}

void* RecordClass::AllocMemory() const
{
    const auto alignedSize = Red::AlignUp(size, alignment);

    const auto allocator = GetAllocator();
    auto [memory, size] = allocator->AllocAligned(alignedSize, alignment);

    std::memset(memory, 0, size);
    return memory;
}

const bool RecordClass::IsEqual(const void* aLhs, const void* aRhs, const uint32_t a3)
{
    using func_t = bool (*)(CClass*, const void*, const void*, uint32_t);
#if defined(RED4EXT_V1_SDK_VERSION_CURRENT) || defined(RED4EXT_SDK_0_5_0)
    static RED4ext::UniversalRelocFunc<func_t> func(RED4ext::Detail::AddressHashes::TTypedClass_IsEqual);
#else
    static RelocFunc<func_t> func(RED4ext::Addresses::TTypedClass_IsEqual);
#endif
    return func(this, aLhs, aRhs, a3);
}

void RecordClass::Assign(void* aLhs, const void* aRhs) const
{
    new (aLhs) Red::ScriptableTweakDBRecord(*static_cast<const Red::ScriptableTweakDBRecord*>(aRhs));
}
} // namespace App