#pragma once

namespace Red
{
struct TweakDBPropertyInfo
{
    Red::CName name;
    const Red::CBaseRTTIType* type;
    const Red::CBaseRTTIType* elementType;
    const Red::CClass* foreignType;
    bool isArray;
    bool isForeignKey;
    bool isExtra;
    std::string appendix;                // The name used to build ID of the property
    std::optional<int32_t> defaultValue; // Offset of the default value in the buffer
};

struct TweakDBRecordInfo
{
    Red::CName name;
    const Red::CClass* type;
    const Red::CClass* parent;
    Core::Map<Red::CName, Core::SharedPtr<Red::TweakDBPropertyInfo>> props;
    std::string shortName;
    uint32_t typeHash;

    [[nodiscard]] const Red::TweakDBPropertyInfo* GetPropInfo(Red::CName aPropName) const
    {
        const auto& propIt = props.find(aPropName);
        return propIt != props.end() ? propIt->second.get() : nullptr;
    }
};

class TweakDBReflection
{
public:
    TweakDBReflection();
    explicit TweakDBReflection(Red::TweakDB* aTweakDb);

    const Red::TweakDBRecordInfo* GetRecordInfo(Red::CName aTypeName);
    const Red::TweakDBRecordInfo* GetRecordInfo(const Red::CClass* aType);

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
    using RecordInfoMap = Core::Map<Red::CName, Core::SharedPtr<Red::TweakDBRecordInfo>>;

    Core::SharedPtr<Red::TweakDBRecordInfo> CollectRecordInfo(const Red::CClass* aType, Red::TweakDBID aSampleId = {});
    Red::TweakDBID GetRecordSampleId(const Red::CClass* aType);
    std::string ResolvePropertyName(Red::TweakDBID aSampleId, Red::CName aGetterName);
    int32_t ResolveDefaultValue(const Red::CClass* aType, const std::string& aPropName);

    Red::TweakDB* m_tweakDb;
    Red::CRTTISystem* m_rtti;
    RecordInfoMap m_resolved;
    std::shared_mutex m_mutex;

    inline static ParentMap s_parentMap;
    inline static DescendantMap s_descendantMap;
    inline static ExtraFlatMap s_extraFlats;
};
} // namespace Red
