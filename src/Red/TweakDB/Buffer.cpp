#include "Buffer.hpp"

namespace
{
constexpr auto FlatVFTSize = 8u;
constexpr auto FlatAlignment = 8u;
} // namespace

Red::TweakDBBuffer::TweakDBBuffer()
    : TweakDBBuffer(TweakDB::Get())
{
}

Red::TweakDBBuffer::TweakDBBuffer(TweakDB* aTweakDb)
    : m_tweakDb(aTweakDb)
    , m_bufferEnd(0)
    , m_offsetEnd(0)
{
}

int32_t Red::TweakDBBuffer::AllocateValue(const Value<>& aData)
{
    return AllocateValue(aData.type, aData.instance);
}

int32_t Red::TweakDBBuffer::AllocateValue(const CBaseRTTIType* aType, Instance aInstance)
{
    if (m_bufferEnd != m_tweakDb->flatDataBufferEnd)
        SyncBufferData();

    auto& pool = m_pools.at(aType->GetName());
    const auto hash = ComputeHash(aType, aInstance);

    {
        std::shared_lock poolLockR(m_poolMutex);
        if (const auto offsetIt = pool.find(hash); offsetIt != pool.end())
            return offsetIt->second;
    }

    const auto offset = m_tweakDb->CreateFlatValue({const_cast<CBaseRTTIType*>(aType), aInstance});

    if (offset > 0)
    {
        std::unique_lock poolLockRW(m_poolMutex);
        pool.emplace(hash, offset);
    }

    return offset;
}

int32_t Red::TweakDBBuffer::AllocateDefault(const CBaseRTTIType* aType)
{
    if (m_bufferEnd != m_tweakDb->flatDataBufferEnd)
        SyncBufferData();

    return m_defaults.at(aType->GetName());
}

Red::Value<> Red::TweakDBBuffer::GetValue(const int32_t aOffset)
{
    if (aOffset < 0)
        return {};

    if (m_bufferEnd != m_tweakDb->flatDataBufferEnd)
        SyncBufferData();

    return ResolveOffset(aOffset);
}

Red::Instance Red::TweakDBBuffer::GetValuePtr(const int32_t aOffset)
{
    if (aOffset < 0)
        return {};

    if (m_bufferEnd != m_tweakDb->flatDataBufferEnd)
        SyncBufferData();

    return ResolveOffset(aOffset).instance;
}

uint64_t Red::TweakDBBuffer::GetValueHash(const int32_t aOffset)
{
    if (aOffset < 0)
        return {};

    if (m_bufferEnd != m_tweakDb->flatDataBufferEnd)
        SyncBufferData();

    const auto data = ResolveOffset(aOffset);

    return ComputeHash(data.type, data.instance);
}

uint64_t Red::TweakDBBuffer::ComputeHash(const CBaseRTTIType* aType, Instance aInstance, const uint32_t aSize,
                                         const uint64_t aSeed)
{
    // Case 1: Everything is processed as a sequence of bytes and passed to the hash function,
    //         except for an array of strings.
    // Case 2: Arrays of strings are different because of empty strings that don't produce any
    //         hashing value. Therefore hash will be equal for different arrays in cases like:
    //         [] == [""] == ["", ""]
    //         ["", "a", "b"] == ["a", "", "b"] == ["a", "b", ""]
    //         As a workaround, we hash the string length as part of the data.

    uint64_t hash;

    if (aType->GetType() == ERTTIType::Array)
    {
        auto* arrayType = reinterpret_cast<const CRTTIArrayType*>(aType);

        if (const auto* innerType = arrayType->GetInnerType(); innerType->GetName() == "String")
        {
            const auto* array = static_cast<DynArray<CString>*>(aInstance);
            const auto size = aSize ? aSize : array->size;

            hash = aSeed;
            for (uint32_t i = 0; i != size; ++i)
            {
                const auto* str = array->entries + i;
                const auto length = str->Length();
                hash = FNV1a64(reinterpret_cast<const uint8_t*>(&length), sizeof(length), hash);
                hash = FNV1a64(reinterpret_cast<const uint8_t*>(str->c_str()), length, hash);
            }
        }
        else
        {
            const auto* array = static_cast<DynArray<uint8_t>*>(aInstance);
            const auto size = aSize ? aSize : array->size;
            hash = FNV1a64(array->entries, size * innerType->GetSize(), aSeed);
        }
    }
    else if (aType->GetName() == "String")
    {
        const auto* str = static_cast<CString*>(aInstance);
        const auto* data = reinterpret_cast<const uint8_t*>(str->c_str());
        hash = FNV1a64(data, str->Length(), aSeed);
    }
    else
    {
        const auto* data = static_cast<const uint8_t*>(aInstance);
        hash = FNV1a64(data, aType->GetSize(), aSeed);
    }

    return hash;
}

Red::Value<> Red::TweakDBBuffer::ResolveOffset(const int32_t aOffset)
{
    // This method uses VFTs to determine the flat type.
    // It's 11% to 33% faster than calling GetValue() every time.

    const auto addr = m_tweakDb->flatDataBuffer + aOffset;
    const auto vft = *reinterpret_cast<uintptr_t*>(addr);

    // For a known VFT we can immediately get RTTI type and data pointer.
    if (const auto it = m_types.find(vft); it != m_types.end())
        return {it->second.type, reinterpret_cast<void*>(addr + it->second.offset)};

    // For an unknown VFT, we call the virtual GetValue() once to get the type.
    const auto data = reinterpret_cast<TweakDBFlatValue*>(addr)->GetValue();

    // Add type info to the map.
    // In addition to the RTTI type, we also store the data offset considering alignment.
    // Quaternion is 16-byte aligned, so there is 8-byte padding between the VFT and the data:
    // [ 8B VFT ][ 8B PAD ][ 16B QUATERNION ]
    m_types.insert({vft, {data.type, std::max(data.type->GetAlignment(), FlatAlignment)}});

    return data;
}

void Red::TweakDBBuffer::CreatePools()
{
    if (m_pools.size() != TDBFlatType::GetSize())
    {
        m_pools.reserve(TDBFlatType::GetSize());

        for (const auto* flatType : TDBFlatType::GetTypes())
        {
            m_pools.emplace(*flatType, 0);
        }
    }
}

void Red::TweakDBBuffer::FillDefaults()
{
    if (m_defaults.size() != TDBFlatType::GetSize())
    {
        for (const auto* flatType : TDBFlatType::GetTypes())
        {
            auto& pool = m_pools.at(*flatType);

            const auto value = MakeValue(flatType->GetClass());
            const auto hash = ComputeHash(value->type, value->instance);
            const auto it = pool.find(hash);

            int32_t offset;

            if (it != pool.end())
            {
                offset = it->second;
            }
            else
            {
                offset = m_tweakDb->CreateFlatValue(*value);
                pool.emplace(hash, offset);
            }

            m_defaults.emplace(*flatType, offset);

            ResolveOffset(offset);
        }
    }
}

void Red::TweakDBBuffer::SyncBufferData()
{
    std::unique_lock poolLockRW(m_poolMutex);

    const auto offsetEnd = m_tweakDb->flatDataBufferEnd - m_tweakDb->flatDataBuffer;

    if (m_offsetEnd == offsetEnd)
    {
        SyncBufferBounds();
        return;
    }

    if (m_offsetEnd == 0)
        CreatePools();

    const auto startTimePoint = std::chrono::steady_clock::now();

    {
        std::shared_lock flatLockR(m_tweakDb->mutex00);

        auto offset = AlignUp(static_cast<uint32_t>(m_offsetEnd), FlatAlignment);
        while (offset < offsetEnd)
        {
            // The current offset should always point to the VFT of the next flat.
            // If there's zero instead, that means the next value is 16-byte aligned,
            // and we need to skip the 8-byte padding to get to the flat.
            if (*reinterpret_cast<uint64_t*>(m_tweakDb->flatDataBuffer + offset) == 0ull)
                offset += 8u;

            const auto data = ResolveOffset(static_cast<int32_t>(offset));
            const auto hash = ComputeHash(data.type, data.instance);

            // Check for duplicates...
            // (Original game's blob has ~24K duplicates)
            if (auto& pool = m_pools.at(data.type->GetName()); !pool.contains(hash))
                pool.emplace(hash, offset);

            // Step {vft + data_size} aligned by {max(data_align, 8)}
            offset += AlignUp(FlatVFTSize + data.type->GetSize(), std::max(FlatAlignment, data.type->GetAlignment()));
        }
    }

    const auto endTimePoint = std::chrono::steady_clock::now();
    const auto updateTime =
        std::chrono::duration_cast<std::chrono::duration<float>>(endTimePoint - startTimePoint).count();

    if (m_offsetEnd == 0)
        FillDefaults();

    SyncBufferBounds();

    UpdateStats(updateTime);
}

void Red::TweakDBBuffer::SyncBufferBounds()
{
    m_bufferEnd = m_tweakDb->flatDataBufferEnd;
    m_offsetEnd = m_tweakDb->flatDataBufferEnd - m_tweakDb->flatDataBuffer;
}

void Red::TweakDBBuffer::UpdateStats(const float updateTime)
{
    if (updateTime != 0)
    {
        if (m_stats.initTime == 0)
            m_stats.initTime = updateTime;
        else
            m_stats.updateTime = updateTime;
    }

    size_t totalValues = 0;
    for (const auto& val : m_pools | std::views::values)
        totalValues += val.size();

    m_stats.poolSize = m_offsetEnd;
    m_stats.poolValues = totalValues;
    m_stats.knownTypes = m_types.size();
    m_stats.flatEntries = m_tweakDb->flats.size;

#ifdef VERBOSE
    Red::Log::Debug("[Red::TweakDBFlatPool] init {:.3f}s | update {:.6f}s | {} KiB | {} values | {} flats | {} types",
                    m_stats.initTime, m_stats.updateTime, m_stats.poolSize / 1024, m_stats.poolValues,
                    m_stats.flatEntries, m_stats.knownTypes);
#endif
}

Red::TweakDBBuffer::BufferStats Red::TweakDBBuffer::GetStats() const
{
    return m_stats;
}

void Red::TweakDBBuffer::Invalidate()
{
    std::unique_lock poolLockRW(m_poolMutex);

    m_bufferEnd = 0;
    m_offsetEnd = 0;

    for (const auto* flatType : TDBFlatType::GetTypes())
    {
        m_pools.at(*flatType).clear();
    }

    m_stats = {};
}
