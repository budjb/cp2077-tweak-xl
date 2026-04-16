#pragma once

#include "App/Tweaks/Record/CustomTweakDBRecord.hpp"
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
        Core::Set<Red::TweakDBID> flats;
        Core::Map<Red::TweakDBID, const Red::TweakDBRecordInfo*> records;
        Core::Map<Red::TweakDBID, const std::string> names;
        std::shared_mutex mutex;
        friend TweakDBManager;
    };

    using BatchPtr = Core::SharedPtr<Batch>;

    explicit TweakDBManager(Core::SharedPtr<Red::TweakDBReflection> aReflection);

    TweakDBManager(const TweakDBManager&) = delete;
    TweakDBManager& operator=(const TweakDBManager&) = delete;

    Red::Value<> GetFlat(Red::TweakDBID aFlatId);
    Red::Value<> GetDefault(const Red::rtti::IType* aType);
    Red::Handle<Red::TweakDBRecord> GetRecord(Red::TweakDBID aRecordId);
    const Red::CClass* GetRecordType(Red::TweakDBID aRecordId);
    bool IsFlatExists(Red::TweakDBID aFlatId);
    bool IsRecordExists(Red::TweakDBID aRecordId);
    bool SetFlat(Red::TweakDBID aFlatId, const Red::rtti::IType* aType, Red::Instance aInstance);
    bool SetFlat(Red::TweakDBID aFlatId, const Red::Value<>& aData);
    bool CreateRecord(Red::TweakDBID aRecordId, const Red::CClass* aType);
    bool CreateCustomRecord(Red::TweakDBID aRecordId, uint32_t aHash) const;
    bool CloneRecord(Red::TweakDBID aRecordId, Red::TweakDBID aSourceId);
    bool InheritProps(Red::TweakDBID aRecordId, Red::TweakDBID aSourceId);
    bool UpdateRecord(Red::TweakDBID aRecordId);
    void RegisterEnum(Red::TweakDBID aRecordId);
    void RegisterName(const std::string& aName);
    void RegisterName(Red::TweakDBID aId, const std::string& aName, const Red::CClass* aType = nullptr);
    const Core::Set<Red::TweakDBID>& GetEnums();
    std::string_view GetName(Red::TweakDBID aId);

    BatchPtr StartBatch();
    const Core::Set<Red::TweakDBID>& GetFlats(const BatchPtr& aBatch);
    Red::Value<> GetFlat(const BatchPtr& aBatch, Red::TweakDBID aFlatId);
    const Red::CClass* GetRecordType(const BatchPtr& aBatch, Red::TweakDBID aRecordId);
    bool IsFlatExists(const BatchPtr& aBatch, Red::TweakDBID aFlatId);
    bool IsRecordExists(const BatchPtr& aBatch, Red::TweakDBID aRecordId);
    bool SetFlat(const BatchPtr& aBatch, Red::TweakDBID aFlatId, const Red::rtti::IType* aType, Red::Instance aValue);
    bool SetFlat(const BatchPtr& aBatch, Red::TweakDBID aFlatId, const Red::Value<>& aData);
    bool CreateRecord(const BatchPtr& aBatch, Red::TweakDBID aRecordId, const Red::CClass* aType);
    bool CloneRecord(const BatchPtr& aBatch, Red::TweakDBID aRecordId, Red::TweakDBID aSourceId);
    bool InheritProps(const BatchPtr& aBatch, Red::TweakDBID aRecordId, Red::TweakDBID aSourceId);
    bool UpdateRecord(const BatchPtr& aBatch, Red::TweakDBID aRecordId);
    void RegisterName(const BatchPtr& aBatch, Red::TweakDBID aId, const std::string& aName);
    void CommitBatch(const BatchPtr& aBatch);

    bool RegisterCustomRecord(const Core::SharedPtr<Red::TweakDBRecordInfo>& aRecordInfo) const;
    bool DescribeCustomRecord(const Core::SharedPtr<Red::TweakDBRecordInfo>& aRecordInfo,
                              Red::ScriptingFunction_t<void*> aGetterFunction);
    void* GetCustomRecordValue(const App::CustomTweakDBRecord* aRecord, CName functionName);

    void Invalidate();

    Red::TweakDB* GetTweakDB();
    Core::SharedPtr<Red::TweakDBReflection>& GetReflection();

private:
    template<class SharedLockable>
    inline bool AssignFlat(Red::SortedUniqueArray<Red::TweakDBID>& aFlats, Red::TweakDBID aFlatId,
                           const Red::rtti::IType* aType, Red::Instance aInstance, SharedLockable& aMutex);
    inline void InheritFlats(Red::SortedUniqueArray<Red::TweakDBID>& aFlats, Red::TweakDBID aRecordId,
                             const Red::TweakDBRecordInfo* aRecordInfo);
    inline void InheritFlats(Red::SortedUniqueArray<Red::TweakDBID>& aFlats, Red::TweakDBID aRecordId,
                             const Red::TweakDBRecordInfo* aRecordInfo, Red::TweakDBID aSourceId);

    inline bool AssignFlat(const Red::TweakDBManager::BatchPtr& aBatch, Red::TweakDBID aFlatId,
                           const Red::Value<>& aValue);
    inline void InheritFlats(const Red::TweakDBManager::BatchPtr& aBatch, Red::TweakDBID aRecordId,
                             const Red::TweakDBRecordInfo* aRecordInfo);
    inline void InheritFlats(const Red::TweakDBManager::BatchPtr& aBatch, Red::TweakDBID aRecordId,
                             const Red::TweakDBRecordInfo* aRecordInfo, Red::TweakDBID aSourceId);

    void CreateBaseName(Red::TweakDBID aId, const std::string& aName);
    void CreateExtraNames(Red::TweakDBID aId, const std::string& aName, const Red::CClass* aType = nullptr);

    void DescribeCustomRecordProperty(Red::CClass* cls,
                                      const Core::SharedPtr<const Red::TweakDBPropertyInfo>& aPropertyInfo,
                                      Red::ScriptingFunction_t<void*> aGetterFunction);
    void InsertPropertyFlat(Red::CName aRecordName,
                            const Core::SharedPtr<const Red::TweakDBPropertyInfo>& aPropertyInfo);

    Red::TweakDB* m_tweakDb;
    Red::CRTTISystem* m_rtti;
    Core::SharedPtr<Red::TweakDBBuffer> m_buffer;
    Core::SharedPtr<Red::TweakDBReflection> m_reflection;
    Core::Map<Red::TweakDBID, std::string> m_knownNames;
    Core::Set<Red::TweakDBID> m_knownEnums;
    std::shared_mutex m_mutex;
};
} // namespace Red
