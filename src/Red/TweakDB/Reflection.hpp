#pragma once

#include "Types.hpp"

namespace Red
{
class TweakDBReflection
{
public:
    TweakDBReflection();
    explicit TweakDBReflection(Red::TweakDB* aTweakDb);

    Red::RecordInfo GetRecordInfo(const std::string& aTypeName, bool aCollect = true);
    Red::RecordInfo GetRecordInfo(Red::CName aTypeName, bool aCollect = true);
    Red::RecordInfo GetRecordInfo(Red::CClass* aType, bool aCollect = true);
    Red::RecordInfo GetRecordInfo(const Red::CClass* aType, bool aCollect = true);

    bool IsOriginalRecord(Red::TweakDBID aRecordId);
    bool IsOriginalBaseRecord(Red::TweakDBID aParentId);
    Red::TweakDBID GetOriginalParent(Red::TweakDBID aRecordId);
    const Core::Set<Red::TweakDBID>& GetOriginalDescendants(Red::TweakDBID aSourceId);

    void RegisterExtraFlat(Red::CName aRecordType, const std::string& aPropName, Red::CName aPropType,
                           Red::CName aForeignType);
    void RegisterDescendants(Red::TweakDBID aParentId, const Core::Set<Red::TweakDBID>& aDescendantIds);

    std::string ToString(Red::TweakDBID aID);

    Red::TweakDB* GetTweakDB();

private:
    struct ExtraFlat
    {
        Red::CName typeName;
        Red::CName foreignTypeName;
        std::string appendix;
    };

    using ParentMap = Core::Map<Red::TweakDBID, Red::TweakDBID>;
    using DescendantMap = Core::Map<Red::TweakDBID, Core::Set<Red::TweakDBID>>;
    using ExtraFlatMap = Core::Map<Red::CName, Core::Vector<ExtraFlat>>;
    using RecordInfoByNameMap = Core::Map<Red::CName, RecordInfo>;
    using RecordInfoByHashMap = Core::Map<uint32_t, RecordInfo>;

    Red::RecordInfo CollectRecordInfo(Red::CClass* aType, Red::TweakDBID aSampleId = {});
    Red::TweakDBID GetRecordSampleId(const Red::CClass* aType);
    std::string ResolvePropertyName(Red::TweakDBID aSampleId, Red::CName aGetterName);
    std::optional<int32_t> ResolveDefaultValue(const Red::CClass* aType, const std::string& aPropName);

    bool IsValid(const PropertyInfo& aPropInfo);
    bool IsValid(const RecordInfo& aRecordInfo);

    bool RegisterRecordInfo(RecordInfo aRecordInfo);
    bool RegisterPropertyInfo(const RecordInfo& aRecordInfo, const PropertyInfo& aPropertyInfo);
    void InheritRecordInfo(const RecordInfo&, Red::RecordInfo aParentInfo);
    RecordInfo CreateRecordInfo(Red::CClass* aClass);
    PropertyInfo CreatePropertyInfo(const std::string& aName, uint64_t aFlatType);
    PropertyInfo CreatePropertyInfo(const std::string& aName, const Red::CBaseRTTIType* aFlatType);

    Red::TweakDB* m_tweakDb;
    Red::CRTTISystem* m_rtti;

    RecordInfoByNameMap m_recordInfoByName;
    RecordInfoByNameMap m_recordInfoByHash;
    std::shared_mutex m_mutex;

    inline static ParentMap s_parentMap;
    inline static DescendantMap s_descendantMap;
    inline static ExtraFlatMap s_extraFlats;
};

} // namespace Red
