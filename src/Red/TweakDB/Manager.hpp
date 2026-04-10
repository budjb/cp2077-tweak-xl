#pragma once

#include "Red/TweakDB/Alias.hpp"
#include "Red/TweakDB/Buffer.hpp"
#include "Red/TweakDB/Reflection.hpp"

namespace Red
{
class TweakDBManager
{
public:
    class Batch
    {
        Core::Set<TweakDBID> flats;
        Core::Map<TweakDBID, const TweakDBRecordSchema*> records;
        Core::Map<TweakDBID, const std::string> names;
        std::shared_mutex mutex;
        friend TweakDBManager;
    };

    using BatchPtr = Core::SharedPtr<Batch>;

    TweakDBManager();
    explicit TweakDBManager(TweakDB* aTweakDb);
    explicit TweakDBManager(Core::SharedPtr<TweakDBReflection> aReflection);

    TweakDBManager(const TweakDBManager&) = delete;
    TweakDBManager& operator=(const TweakDBManager&) = delete;

    Value<> GetFlat(TweakDBID aFlatId) const;
    Value<> GetDefault(const CBaseRTTIType* aType) const;
    Handle<TweakDBRecord> GetRecord(TweakDBID aRecordId) const;
    const CClass* GetRecordType(TweakDBID aRecordId) const;
    bool IsFlatExists(TweakDBID aFlatId) const;
    bool IsRecordExists(TweakDBID aRecordId) const;
    bool SetFlat(TweakDBID aFlatId, const CBaseRTTIType* aType, Instance aInstance);
    bool SetFlat(TweakDBID aFlatId, const Value<>& aData);
    bool CreateRecord(TweakDBID aRecordId, const CClass* aType);
    bool CloneRecord(TweakDBID aRecordId, TweakDBID aSourceId);
    bool InheritProps(TweakDBID aRecordId, TweakDBID aSourceId);
    bool UpdateRecord(TweakDBID aRecordId) const;
    void RegisterEnum(TweakDBID aRecordId);
    void RegisterName(const std::string& aName);
    void RegisterName(TweakDBID aId, const std::string& aName, const CClass* aType = nullptr);
    const Core::Set<TweakDBID>& GetEnums();
    std::string_view GetName(TweakDBID aId);

    BatchPtr StartBatch();
    const Core::Set<TweakDBID>& GetFlats(const BatchPtr& aBatch);
    Value<> GetFlat(const BatchPtr& aBatch, TweakDBID aFlatId) const;
    const CClass* GetRecordType(const BatchPtr& aBatch, TweakDBID aRecordId) const;
    bool IsFlatExists(const BatchPtr& aBatch, TweakDBID aFlatId) const;
    bool IsRecordExists(const BatchPtr& aBatch, TweakDBID aRecordId) const;
    bool SetFlat(const BatchPtr& aBatch, TweakDBID aFlatId, const CBaseRTTIType* aType, Instance aInstance) const;
    bool SetFlat(const BatchPtr& aBatch, TweakDBID aFlatId, const Value<>& aData) const;
    bool CreateRecord(const BatchPtr& aBatch, TweakDBID aRecordId, const CClass* aType);
    bool CloneRecord(const BatchPtr& aBatch, TweakDBID aRecordId, TweakDBID aSourceId);
    bool InheritProps(const BatchPtr& aBatch, TweakDBID aRecordId, TweakDBID aSourceId);
    bool UpdateRecord(const BatchPtr& aBatch, TweakDBID aRecordId) const;
    void RegisterName(const BatchPtr& aBatch, TweakDBID aId, const std::string& aName);
    void CommitBatch(const BatchPtr& aBatch);

    void Invalidate() const;

    TweakDB* GetTweakDB() const;
    Core::SharedPtr<TweakDBReflection>& GetReflection();

private:
    template<class SharedLockable>
    inline bool AssignFlat(SortedUniqueArray<TweakDBID>& aFlats, TweakDBID aFlatId,
                           const CBaseRTTIType* aType, Instance aInstance,
                           SharedLockable& aMutex);
    inline void InheritFlats(SortedUniqueArray<TweakDBID>& aFlats, TweakDBID aRecordId,
                             const TweakDBRecordSchema* aRecordSchema) const;
    inline void InheritFlats(SortedUniqueArray<TweakDBID>& aFlats, TweakDBID aRecordId,
                             const TweakDBRecordSchema* aRecordSchema, TweakDBID aSourceId) const;

    inline bool AssignFlat(const BatchPtr& aBatch, TweakDBID aFlatId,
                           const Value<>& aValue) const;
    inline void InheritFlats(const BatchPtr& aBatch, TweakDBID aRecordId,
                             const TweakDBRecordSchema* aRecordSchema) const;
    inline void InheritFlats(const BatchPtr& aBatch, TweakDBID aRecordId,
                             const TweakDBRecordSchema* aRecordSchema, TweakDBID aSourceId) const;

    void CreateBaseName(TweakDBID aId, const std::string& aName);
    void CreateExtraNames(TweakDBID aId, const std::string& aName, const CClass* aType = nullptr);

    TweakDB* m_tweakDb;
    Core::SharedPtr<TweakDBBuffer> m_buffer;
    Core::SharedPtr<TweakDBReflection> m_reflection;
    Core::Map<TweakDBID, std::string> m_knownNames;
    Core::Set<TweakDBID> m_knownEnums;
    std::shared_mutex m_mutex;
};
}
