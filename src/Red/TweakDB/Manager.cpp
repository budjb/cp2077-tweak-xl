#include "Manager.hpp"
#include "Red/TweakDB/Raws.hpp"

namespace
{
constexpr auto OptimizedFlatChunkSize = 16000;
}

Red::TweakDBManager::TweakDBManager()
    : TweakDBManager(TweakDB::Get())
{
}

Red::TweakDBManager::TweakDBManager(TweakDB* aTweakDb)
    : m_tweakDb(aTweakDb)
    , m_buffer(Core::MakeShared<TweakDBBuffer>(m_tweakDb))
    , m_reflection(Core::MakeShared<TweakDBReflection>(m_tweakDb))
{
}

Red::TweakDBManager::TweakDBManager(Core::SharedPtr<TweakDBReflection> aReflection)
    : m_tweakDb(aReflection->GetTweakDB())
    , m_buffer(Core::MakeShared<TweakDBBuffer>(m_tweakDb))
    , m_reflection(std::move(aReflection))
{
}

Red::Value<> Red::TweakDBManager::GetFlat(const TweakDBID aFlatId) const
{
    int32_t offset;

    {
        std::shared_lock flatLockR(m_tweakDb->mutex00);
        const auto* flat = m_tweakDb->flats.Find(aFlatId);

        if (flat == m_tweakDb->flats.End())
            return {};

        offset = flat->ToTDBOffset();
    }

    return m_buffer->GetValue(offset);
}

Red::Value<> Red::TweakDBManager::GetDefault(const CBaseRTTIType* aType) const
{
    if (!TweakDBReflection::IsFlatType(aType))
        return {};

    return m_buffer->GetValue(m_buffer->AllocateDefault(aType));
}

Red::Handle<Red::TweakDBRecord> Red::TweakDBManager::GetRecord(const TweakDBID aRecordId) const
{
    std::shared_lock recordLockR(m_tweakDb->mutex01);
    const auto* record = m_tweakDb->recordsByID.Get(aRecordId);

    if (record == nullptr)
        return {};

    return *reinterpret_cast<const Red::Handle<Red::TweakDBRecord>*>(record);
}

const Red::CClass* Red::TweakDBManager::GetRecordType(const TweakDBID aRecordId) const
{
    std::shared_lock recordLockR(m_tweakDb->mutex01);
    const auto* record = m_tweakDb->recordsByID.Get(aRecordId);
    return record ? record->GetPtr()->GetType() : nullptr;
}

bool Red::TweakDBManager::IsFlatExists(const TweakDBID aFlatId) const
{
    std::shared_lock flatLockR(m_tweakDb->mutex00);
    return m_tweakDb->flats.Find(aFlatId) != m_tweakDb->flats.End();
}

bool Red::TweakDBManager::IsRecordExists(const TweakDBID aRecordId) const
{
    std::shared_lock recordLockR(m_tweakDb->mutex01);
    return m_tweakDb->recordsByID.Get(aRecordId) != nullptr;
}

bool Red::TweakDBManager::SetFlat(const TweakDBID aFlatId, const CBaseRTTIType* aType, const Instance aInstance)
{
    if (!aFlatId.IsValid() || !aInstance || !TweakDBReflection::IsFlatType(aType))
        return false;

    return AssignFlat(m_tweakDb->flats, aFlatId, aType, aInstance, m_tweakDb->mutex00);
}

bool Red::TweakDBManager::SetFlat(const TweakDBID aFlatId, const Value<>& aData)
{
    return SetFlat(aFlatId, aData.type, aData.instance);
}

bool Red::TweakDBManager::CreateRecord(TweakDBID aRecordId, const CClass* aType)
{
    if (!aRecordId.IsValid() || IsRecordExists(aRecordId))
        return false;

    const auto recordSchema = m_reflection->GetRecordSchema(aType);

    if (!recordSchema)
        return false;

    SortedUniqueArray<TweakDBID> propFlats;
    propFlats.Reserve(recordSchema->GetProperties().size());
    InheritFlats(propFlats, aRecordId, recordSchema);

    {
        std::unique_lock flatLockRW(m_tweakDb->mutex00);
        m_tweakDb->flats.Insert(propFlats);
    }

    Raw::CreateRecord(m_tweakDb, recordSchema->GetHash(), aRecordId);

    return true;
}

bool Red::TweakDBManager::CloneRecord(TweakDBID aRecordId, TweakDBID aSourceId)
{
    if (!aRecordId.IsValid() || !aSourceId.IsValid())
        return false;

    if (IsRecordExists(aRecordId) || !IsRecordExists(aSourceId))
        return false;

    const auto recordType = GetRecordType(aSourceId);
    const auto recordSchema = m_reflection->GetRecordSchema(recordType);

    if (!recordSchema)
        return false;

    SortedUniqueArray<TweakDBID> propFlats;
    propFlats.Reserve(recordSchema->GetProperties().size());
    InheritFlats(propFlats, aRecordId, recordSchema, aSourceId);

    {
        std::unique_lock flatLockRW(m_tweakDb->mutex00);
        m_tweakDb->flats.Insert(propFlats);
    }

    Raw::CreateRecord(m_tweakDb, recordSchema->GetHash(), aRecordId);

    return true;
}

bool Red::TweakDBManager::InheritProps(TweakDBID aRecordId, TweakDBID aSourceId)
{
    if (!aRecordId.IsValid() || !aSourceId.IsValid())
        return false;

    if (!IsRecordExists(aRecordId) || !IsRecordExists(aSourceId))
        return false;

    const auto recordType = GetRecordType(aRecordId);

    if (const auto sourceType = GetRecordType(aSourceId); recordType != sourceType)
        return false;

    const auto recordSchema = m_reflection->GetRecordSchema(recordType);

    if (!recordSchema)
        return false;

    SortedUniqueArray<TweakDBID> propFlats;
    propFlats.Reserve(recordSchema->GetProperties().size());
    InheritFlats(propFlats, aRecordId, recordSchema, aSourceId);

    {
        std::unique_lock flatLockRW(m_tweakDb->mutex00);
        m_tweakDb->flats.Insert(propFlats);
    }

    return true;
}

bool Red::TweakDBManager::UpdateRecord(const TweakDBID aRecordId) const
{
    if (!aRecordId.IsValid())
        return false;

    const auto record = GetRecord(aRecordId);

    if (!record)
        return false;

    std::unique_lock recordLockRW(m_tweakDb->mutex01);
    return m_tweakDb->UpdateRecord(record);
}

void Red::TweakDBManager::RegisterEnum(const TweakDBID aRecordId)
{
    std::unique_lock _(m_mutex);
    m_knownEnums.insert(aRecordId);
}

void Red::TweakDBManager::RegisterName(const std::string& aName)
{
    RegisterName(aName.data(), aName);
}

void Red::TweakDBManager::RegisterName(const TweakDBID aId, const std::string& aName, const CClass* aType)
{
    CreateBaseName(aId, aName);
    CreateExtraNames(aId, aName, aType);
}

Red::TweakDBManager::BatchPtr Red::TweakDBManager::StartBatch()
{
    return Core::MakeShared<Batch>();
}

const Core::Set<Red::TweakDBID>& Red::TweakDBManager::GetFlats(const BatchPtr& aBatch)
{
    return aBatch->flats;
}

Red::Value<> Red::TweakDBManager::GetFlat(const BatchPtr& aBatch, const TweakDBID aFlatId) const
{
    std::shared_lock flatLockR(aBatch->mutex);
    const auto& flat = aBatch->flats.find(aFlatId);

    if (flat == aBatch->flats.end())
        return {};

    return m_buffer->GetValue(flat->ToTDBOffset());
}

const Red::CClass* Red::TweakDBManager::GetRecordType(const BatchPtr& aBatch, const TweakDBID aRecordId) const
{
    auto recordType = GetRecordType(aRecordId);

    if (!recordType)
    {
        std::shared_lock batchLockR(aBatch->mutex);

        if (const auto it = aBatch->records.find(aRecordId); it != aBatch->records.end())
            recordType = it.value()->GetClass();
    }

    return recordType;
}

bool Red::TweakDBManager::IsFlatExists(const BatchPtr& aBatch, const Red::TweakDBID aFlatId) const
{
    if (IsFlatExists(aFlatId))
        return true;

    std::shared_lock batchLockR(aBatch->mutex);
    return aBatch->flats.contains(aFlatId);
}

bool Red::TweakDBManager::IsRecordExists(const BatchPtr& aBatch, const Red::TweakDBID aRecordId) const
{
    if (IsRecordExists(aRecordId))
        return true;

    std::shared_lock batchLockR(aBatch->mutex);
    return aBatch->records.contains(aRecordId);
}

bool Red::TweakDBManager::SetFlat(const BatchPtr& aBatch, const TweakDBID aFlatId,
                                  const CBaseRTTIType* aType,
                                  const Instance aInstance) const
{
    if (!aFlatId.IsValid() || !aInstance || !TweakDBReflection::IsFlatType(aType))
        return false;

    return AssignFlat(aBatch, aFlatId, {aType, aInstance});
}

bool Red::TweakDBManager::SetFlat(const BatchPtr& aBatch, const TweakDBID aFlatId,
                                  const Value<>& aData) const
{
    if (!aFlatId.IsValid() || !aData.instance || !TweakDBReflection::IsFlatType(aData.type))
        return false;

    return AssignFlat(aBatch, aFlatId, aData);
}

bool Red::TweakDBManager::CreateRecord(const BatchPtr& aBatch, TweakDBID aRecordId,
                                       const CClass* aType)
{
    if (!aRecordId.IsValid() || !aType || IsRecordExists(aBatch, aRecordId))
        return false;

    const auto recordSchema = m_reflection->GetRecordSchema(aType);

    if (!recordSchema)
        return false;

    std::unique_lock batchLockRW(aBatch->mutex);
    InheritFlats(aBatch, aRecordId, recordSchema);
    aBatch->records.emplace(aRecordId, recordSchema);

    return true;
}

bool Red::TweakDBManager::CloneRecord(const BatchPtr& aBatch, TweakDBID aRecordId,
                                      TweakDBID aSourceId)
{
    if (!aRecordId.IsValid() || !aSourceId.IsValid())
        return false;

    if (IsRecordExists(aBatch, aRecordId) || !IsRecordExists(aBatch, aSourceId))
        return false;

    const auto recordType = GetRecordType(aBatch, aSourceId);
    auto recordSchema = m_reflection->GetRecordSchema(recordType);

    if (!recordSchema)
        return false;

    std::unique_lock batchLockRW(aBatch->mutex);
    InheritFlats(aBatch, aRecordId, recordSchema, aSourceId);
    aBatch->records.emplace(aRecordId, recordSchema);

    return true;
}

bool Red::TweakDBManager::InheritProps(const BatchPtr& aBatch, TweakDBID aRecordId,
                                       TweakDBID aSourceId)
{
    if (!aRecordId.IsValid() || !aSourceId.IsValid())
        return false;

    if (!IsRecordExists(aBatch, aRecordId) || !IsRecordExists(aBatch, aSourceId))
        return false;

    const auto recordType = GetRecordType(aBatch, aRecordId);

    if (const auto sourceType = GetRecordType(aBatch, aSourceId); recordType != sourceType)
        return false;

    auto recordSchema = m_reflection->GetRecordSchema(recordType);

    if (!recordSchema)
        return false;

    std::unique_lock batchLockRW(aBatch->mutex);
    InheritFlats(aBatch, aRecordId, recordSchema, aSourceId);

    return true;
}

bool Red::TweakDBManager::UpdateRecord(const BatchPtr& aBatch, TweakDBID aRecordId) const
{
    if (!aRecordId.IsValid() || !IsRecordExists(aRecordId))
        return false;

    std::unique_lock batchLockRW(aBatch->mutex);

    if (aBatch->records.contains(aRecordId))
        return false;

    aBatch->records.emplace(aRecordId, nullptr);

    return true;
}

void Red::TweakDBManager::RegisterName(const BatchPtr& aBatch, TweakDBID aId,
                                       const std::string& aName)
{
    std::unique_lock batchLockRW(aBatch->mutex);
    aBatch->names.emplace(aId, aName);
}

void Red::TweakDBManager::CommitBatch(const BatchPtr& aBatch)
{
    std::unique_lock batchLockRW(aBatch->mutex);

    for (const auto& [id, name] : aBatch->names)
    {
        CreateBaseName(id, name);
    }

    {
        SortedUniqueArray<TweakDBID> flatsChunk;

        for (const auto& flatId : aBatch->flats)
        {
            flatsChunk.InsertOrAssign(flatId);

            if (flatsChunk.size >= OptimizedFlatChunkSize)
            {
                std::unique_lock flatLockRW(m_tweakDb->mutex00);
                m_tweakDb->flats.InsertOrAssign(flatsChunk);
                flatsChunk.Clear();
            }
        }

        if (flatsChunk.size > 0)
        {
            std::unique_lock flatLockRW(m_tweakDb->mutex00);
            m_tweakDb->flats.InsertOrAssign(flatsChunk);
        }
    }

    for (const auto& [recordId, recordSchema] : aBatch->records)
    {
        if (const auto record = GetRecord(recordId))
        {
            std::unique_lock recordLockRW(m_tweakDb->mutex01);
            m_tweakDb->UpdateRecord(record);
        }
        else
        {
            Raw::CreateRecord(m_tweakDb, recordSchema->GetHash(), recordId);
        }
    }

    for (const auto& [id, name] : aBatch->names)
    {
        CreateExtraNames(id, name);
    }

    aBatch->flats.clear();
    aBatch->records.clear();
    aBatch->names.clear();
}

void Red::TweakDBManager::Invalidate() const
{
    m_buffer->Invalidate();
}

Red::TweakDB* Red::TweakDBManager::GetTweakDB() const
{
    return m_tweakDb;
}

Core::SharedPtr<Red::TweakDBReflection>& Red::TweakDBManager::GetReflection()
{
    return m_reflection;
}

template<class SharedLockable>
bool Red::TweakDBManager::AssignFlat(SortedUniqueArray<TweakDBID>& aFlats, TweakDBID aFlatId,
                                     const CBaseRTTIType* aType, const Instance aInstance,
                                     SharedLockable& aMutex)
{
    int32_t offset = -1;

    {
        std::shared_lock flatLockR(aMutex);
        if (const auto* flat = aFlats.Find(aFlatId); flat != aFlats.End())
        {
            offset = flat->ToTDBOffset();
        }
    }

    if (offset >= 0)
    {
        const auto value = m_buffer->GetValue(offset);

        if (value.type != aType)
            return false;

        if (value.type->IsEqual(value.instance, aInstance))
            return true;
    }

    LogInfo("Assigning flat with type {}.", aType->GetName().ToString());
    offset = m_buffer->AllocateValue(aType, aInstance);

    if (offset < 0)
        return false;

    aFlatId.SetTDBOffset(offset);

    {
        std::unique_lock flatLockRW(aMutex);
        aFlats.InsertOrAssign(aFlatId);
    }

    return true;
}

void Red::TweakDBManager::InheritFlats(SortedUniqueArray<TweakDBID>& aFlats, const TweakDBID aRecordId,
                                       const TweakDBRecordSchema* aRecordSchema) const
{
    for (const auto propInfo : aRecordSchema->GetProperties() | std::views::values)
    {
        if (!propInfo->GetOffset())
            continue;

        auto propFlat = TweakDBID(aRecordId, propInfo->GetFlatSuffix());
        auto propDefault = propInfo->GetDefaultValueOffset();

        if (propInfo->GetPropertyStorage() == PropertyStorage::FLAT)
        {
            propDefault = m_buffer->AllocateDefault(propInfo->GetType().GetType());
            LogInfo("Inheriting flat with type {} and offset {}.", propInfo->GetType().GetType()->GetName().ToString(), propDefault);
        }

        propFlat.SetTDBOffset(propDefault);
        aFlats.Emplace(propFlat);
    }
}

void Red::TweakDBManager::InheritFlats(SortedUniqueArray<TweakDBID>& aFlats, const TweakDBID aRecordId,
                                       const TweakDBRecordSchema* aRecordSchema, const TweakDBID aSourceId) const
{
    std::shared_lock flatLockR(m_tweakDb->mutex00);

    for (const auto& propInfo : aRecordSchema->GetProperties() | std::views::values)
    {
        const auto baseId = aSourceId + propInfo->GetFlatSuffix();
        const auto* baseFlat = aFlats.Find(baseId);

        if (baseFlat == aFlats.End())
        {
            baseFlat = m_tweakDb->flats.Find(baseId);
            if (baseFlat == m_tweakDb->flats.End())
                continue;
        }

        auto propFlat = aRecordId + propInfo->GetFlatSuffix();
        propFlat.SetTDBOffset(baseFlat->ToTDBOffset());

        aFlats.Emplace(propFlat);
    }
}

bool Red::TweakDBManager::AssignFlat(const BatchPtr& aBatch, TweakDBID aFlatId,
                                     const Value<>& aValue) const
{
    std::unique_lock batchLockRW(aBatch->mutex);

    const auto& flat = aBatch->flats.find(aFlatId);
    int32_t offset;

    if (flat != aBatch->flats.end())
    {
        offset = flat->ToTDBOffset();

        const auto value = m_buffer->GetValue(offset);

        if (value.type != aValue.type)
            return false;

        if (value.type->IsEqual(value.instance, aValue.instance))
            return true;
    }

    LogInfo("Assigning flat with type {}.", aValue.type->GetName().ToString());
    offset = m_buffer->AllocateValue(aValue);

    if (offset < 0)
        return false;

    aFlatId.SetTDBOffset(offset);

    if (flat != aBatch->flats.end())
    {
        const_cast<TweakDBID&>(*flat) = aFlatId;
    }
    else
    {
        aBatch->flats.insert(aFlatId);
    }

    return true;
}

void Red::TweakDBManager::InheritFlats(const BatchPtr& aBatch, const TweakDBID aRecordId,
                                       const TweakDBRecordSchema* aRecordSchema) const
{
    for (const auto& propInfo : aRecordSchema->GetProperties() | std::views::values)
    {
        if (!propInfo->GetOffset())
            continue;

        if (auto propFlat = TweakDBID(aRecordId, propInfo->GetFlatSuffix()); !aBatch->flats.contains(propFlat))
        {
            auto propDefault = propInfo->GetDefaultValueOffset();

            if (propInfo->GetPropertyStorage() == PropertyStorage::FLAT)
            {
                propDefault = m_buffer->AllocateDefault(propInfo->GetType().GetType());
                LogInfo("Inheriting flat with type {} and offset {}.", propInfo->GetType().GetType()->GetName().ToString(), propDefault);
            }

            propFlat.SetTDBOffset(propDefault);

            aBatch->flats.insert(propFlat);
        }
    }
}

void Red::TweakDBManager::InheritFlats(const BatchPtr& aBatch, const TweakDBID aRecordId,
                                       const TweakDBRecordSchema* aRecordSchema, const TweakDBID aSourceId) const
{
    std::shared_lock flatLockR(m_tweakDb->mutex00);

    for (const auto& propInfo : aRecordSchema->GetProperties() | std::views::values)
    {
        const auto baseId = aSourceId + propInfo->GetFlatSuffix();

        if (const auto baseFlat = aBatch->flats.find(baseId); baseFlat != aBatch->flats.end())
        {
            auto propFlat = aRecordId + propInfo->GetFlatSuffix();
            propFlat.SetTDBOffset(baseFlat->ToTDBOffset());

            aBatch->flats.insert(propFlat);
        }
        else
        {
            if (const auto commitedFlat = m_tweakDb->flats.Find(baseId); commitedFlat != m_tweakDb->flats.End())
            {
                auto propFlat = aRecordId + propInfo->GetFlatSuffix();
                propFlat.SetTDBOffset(commitedFlat->ToTDBOffset());

                aBatch->flats.insert(propFlat);
            }
        }
    }
}

void Red::TweakDBManager::CreateBaseName(const TweakDBID aId, const std::string& aName)
{
    const TweakDBID empty;
    Raw::CreateTweakDBID(&empty, &aId, aName.c_str());

    {
        std::unique_lock _(m_mutex);
        m_knownNames[aId] = aName;
    }
}

void Red::TweakDBManager::CreateExtraNames(TweakDBID aId, const std::string& aName, const CClass* aType)
{
    const auto recordSchema = m_reflection->GetRecordSchema(aType ? aType : GetRecordType(aId));

    if (!recordSchema)
        return;

    std::unique_lock _(m_mutex);

    for (const auto& propInfo : recordSchema->GetProperties() | std::views::values)
    {
        const auto propId = aId + propInfo->GetFlatSuffix();
        const auto propName = aName + propInfo->GetFlatSuffix();

        if (propInfo->GetOffset())
        {
            Raw::CreateTweakDBID(&aId, &propId, propInfo->GetFlatSuffix().c_str());
        }
        else
        {
            TweakDBID empty;
            Raw::CreateTweakDBID(&empty, &propId, propName.c_str());
        }

        m_knownNames[propId] = propName;
    }
}

std::string_view Red::TweakDBManager::GetName(TweakDBID aId)
{
    {
        std::shared_lock _(m_mutex);

        const auto it = m_knownNames.find(aId);
        if (it != m_knownNames.end())
            return it->second;
    }

    std::unique_lock _(m_mutex);

    if (auto debugName = Red::TweakDBReflection::ToString(aId); !debugName.empty())
    {
        const auto it = m_knownNames.emplace(aId, debugName).first;
        return it->second;
    }

    auto hashName = std::format("<TDBID:{:08X}:{:02X}>", aId.name.hash, aId.name.length);
    const auto it = m_knownNames.emplace(aId, hashName).first;
    return it->second;
}

const Core::Set<Red::TweakDBID>& Red::TweakDBManager::GetEnums()
{
    std::shared_lock _(m_mutex);

    return m_knownEnums;
}
