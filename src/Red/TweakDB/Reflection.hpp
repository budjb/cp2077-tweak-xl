#pragma once

#include "Core/Stl.hpp"

namespace Red
{

struct TweakDBPropertyInfo
{
    CName name;
    const CBaseRTTIType* type;
    const CBaseRTTIType* elementType;
    const CClass* foreignType;
    bool isArray;
    bool isForeignKey;
    std::string appendix; // The name used to build ID of the property
    uintptr_t dataOffset; // Offset of the property in record instance
    int32_t defaultValue; // Offset of the default value in the buffer
};

struct TweakDBRecordInfo
{
    CName name;
    const CClass* type;
    const CClass* parent;
    std::string shortName;
    Core::Map<CName, Core::SharedPtr<TweakDBPropertyInfo>> props{};
    bool extraFlats;
    uint32_t typeHash;

    [[nodiscard]] const TweakDBPropertyInfo* GetPropInfo(const CName& aPropName) const
    {
        const auto& propIt = props.find(aPropName);
        return propIt != props.end() ? propIt->second.get() : nullptr;
    }
};

class TweakDBReflection
{
public:
    static const CBaseRTTIType* GetFlatType(const CName& aTypeName);
    static const CClass* GetRecordType(const CName& aTypeName);
    static const CClass* GetRecordType(const char* aTypeName);

    static CBaseRTTIType* GetArrayType(const CName& aTypeName);
    static CBaseRTTIType* GetArrayType(const CBaseRTTIType* aType);

    static CBaseRTTIType* GetElementType(const CName& aTypeName);
    static CBaseRTTIType* GetElementType(const CBaseRTTIType* aType);

    static bool IsFlatType(const CName& aTypeName);
    static bool IsFlatType(const CBaseRTTIType* aType);

    static bool IsRecordType(const CName& aTypeName);
    static bool IsRecordType(const CClass* aType);

    static bool IsArrayType(const CName& aTypeName);
    static bool IsArrayType(const CBaseRTTIType* aType);

    static bool IsForeignKey(const CName& aTypeName);
    static bool IsForeignKey(const CBaseRTTIType* aType);

    static bool IsForeignKeyArray(const CName& aTypeName);
    static bool IsForeignKeyArray(const CBaseRTTIType* aType);

    static bool IsResRefToken(const CName& aTypeName);
    static bool IsResRefToken(const CBaseRTTIType* aType);

    static bool IsResRefTokenArray(const CName& aTypeName);
    static bool IsResRefTokenArray(const CBaseRTTIType* aType);

    static CName GetArrayTypeName(const CName& aTypeName);
    static CName GetArrayTypeName(const CBaseRTTIType* aType);

    static CName GetElementTypeName(const CName& aTypeName);
    static CName GetElementTypeName(const CBaseRTTIType* aType);

    static std::string GetRecordFullName(const CName& aName);
    static std::string GetRecordFullName(const char* aName);

    static std::string GetRecordShortName(const CName& aName);
    static std::string GetRecordShortName(const char* aName);

    static std::string GetRecordAliasName(const CName& aName);
    static std::string GetRecordAliasName(const char* aName);

    // TODO: Construct means flats
    static InstancePtr<> Construct(const CName& aTypeName);
    static InstancePtr<> Construct(const CBaseRTTIType* aType);

    static bool IsOriginalRecord(TweakDBID aRecordId);
    static bool IsOriginalBaseRecord(TweakDBID aParentId);
    static TweakDBID GetOriginalParent(TweakDBID aRecordId);

    static uint32_t GetRecordTypeHash(const CClass* aType);
    static uint32_t GetRecordTypeHash(const CName& aName);

    static const Core::Set<TweakDBID>& GetOriginalDescendants(TweakDBID aSourceId);
    static void RegisterExtraFlat(const CName& aRecordType, const std::string& aPropName, const CName& aPropType,
                                  const CName& aForeignType);
    static void RegisterDescendants(TweakDBID aParentId, const Core::Set<TweakDBID>& aDescendantIds);

    static std::string ToString(TweakDBID aID);

    TweakDBReflection();
    explicit TweakDBReflection(TweakDB* aTweakDb);

    const TweakDBRecordSchema* GetRecordSchema(const CName& aTypeName);
    const TweakDBRecordSchema* GetRecordSchema(const CClass* aType);

    [[nodiscard]] TweakDB* GetTweakDB() const;

private:
    using ParentMap = Core::Map<TweakDBID, TweakDBID>;
    using DescendantMap = Core::Map<TweakDBID, Core::Set<TweakDBID>>;
    using ExtraFlatMap = Core::Map<CName, Core::Vector<ExtraFlat>>;
    using RecordSchemaMap = Core::Map<CName, Core::SharedPtr<TweakDBRecordSchema>>;

    static CRTTISystem* GetRTTI();

    Core::SharedPtr<TweakDBRecordSchema> CollectRecordInfo(const CClass* aType, TweakDBID aSampleId = {});
    TweakDBID GetRecordSampleId(const CClass* aType) const; // TODO: remove
    [[nodiscard]] std::string ResolvePropertyName(TweakDBID aSampleId,
                                                  const CName& aGetterName) const;        // TODO: do I need this?
    int32_t ResolveDefaultValue(const CClass* aType, const std::string& aPropName) const; // TODO: do I need this?

    TweakDB* m_tweakDb;
    RecordSchemaMap m_schemas;
    std::shared_mutex m_mutex;

    inline static CClass* s_customTweakRecordType;
    inline static ParentMap s_parentMap;
    inline static DescendantMap s_descendantMap;
    inline static ExtraFlatMap s_extraFlats;
};
} // namespace Red
