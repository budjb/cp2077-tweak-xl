#pragma once

#include "Red/TweakDB/Alias.hpp"
#include "Red/TweakDB/RecordInfo.hpp"

namespace Red
{
namespace ERTDBFlatType
{
enum : uint64_t
{
    Int = Red::FNV1a64("Int32"),
    Float = Red::FNV1a64("Float"),
    Bool = Red::FNV1a64("Bool"),
    String = Red::FNV1a64("String"),
    CName = Red::FNV1a64("CName"),
    LocKey = Red::FNV1a64("gamedataLocKeyWrapper"),
    ResRef = Red::FNV1a64("raRef:CResource"),
    TweakDBID = Red::FNV1a64("TweakDBID"),
    Quaternion = Red::FNV1a64("Quaternion"),
    EulerAngles = Red::FNV1a64("EulerAngles"),
    Vector3 = Red::FNV1a64("Vector3"),
    Vector2 = Red::FNV1a64("Vector2"),
    Color = Red::FNV1a64("Color"),
    IntArray = Red::FNV1a64("array:Int32"),
    FloatArray = Red::FNV1a64("array:Float"),
    BoolArray = Red::FNV1a64("array:Bool"),
    StringArray = Red::FNV1a64("array:String"),
    CNameArray = Red::FNV1a64("array:CName"),
    LocKeyArray = Red::FNV1a64("array:gamedataLocKeyWrapper"),
    ResRefArray = Red::FNV1a64("array:raRef:CResource"),
    TweakDBIDArray = Red::FNV1a64("array:TweakDBID"),
    QuaternionArray = Red::FNV1a64("array:Quaternion"),
    EulerAnglesArray = Red::FNV1a64("array:EulerAngles"),
    Vector3Array = Red::FNV1a64("array:Vector3"),
    Vector2Array = Red::FNV1a64("array:Vector2"),
    ColorArray = Red::FNV1a64("array:Color"),
};
}


struct TweakDBRecordInfo
{
    Red::CName name;
    const Red::CClass* type;
    const Red::CClass* parent;
    Core::Map<Red::CName, Core::SharedPtr<Red::TweakDBPropertyInfo>> props;
    bool extraFlats;
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
    static const Red::CBaseRTTIType* GetFlatType(Red::CName aTypeName);
    static const Red::CClass* GetRecordType(Red::CName aTypeName);
    static const Red::CClass* GetRecordType(const char* aTypeName);

    static Red::CBaseRTTIType* GetArrayType(Red::CName aTypeName);
    static Red::CBaseRTTIType* GetArrayType(const Red::CBaseRTTIType* aType);

    static Red::CBaseRTTIType* GetElementType(Red::CName aTypeName);
    static Red::CBaseRTTIType* GetElementType(const Red::CBaseRTTIType* aType);

    static bool IsFlatType(Red::CName aTypeName);
    static bool IsFlatType(const Red::CBaseRTTIType* aType);

    static bool IsRecordType(Red::CName aTypeName);
    static bool IsRecordType(const Red::CClass* aType);

    static bool IsArrayType(Red::CName aTypeName);
    static bool IsArrayType(const Red::CBaseRTTIType* aType);

    static bool IsForeignKey(Red::CName aTypeName);
    static bool IsForeignKey(const Red::CBaseRTTIType* aType);

    static bool IsForeignKeyArray(Red::CName aTypeName);
    static bool IsForeignKeyArray(const Red::CBaseRTTIType* aType);

    static bool IsResRefToken(Red::CName aTypeName);
    static bool IsResRefToken(const Red::CBaseRTTIType* aType);

    static bool IsResRefTokenArray(Red::CName aTypeName);
    static bool IsResRefTokenArray(const Red::CBaseRTTIType* aType);

    static Red::CName GetArrayTypeName(Red::CName aTypeName);
    static Red::CName GetArrayTypeName(const Red::CBaseRTTIType* aType);

    static Red::CName GetElementTypeName(Red::CName aTypeName);
    static Red::CName GetElementTypeName(const Red::CBaseRTTIType* aType);

    static Red::CName GetRecordFullName(Red::CName aName);
    static Red::CName GetRecordFullName(const char* aName);

    static std::string GetRecordShortName(Red::CName aName);
    static std::string GetRecordShortName(const char* aName);

    static Red::InstancePtr<> Construct(Red::CName aTypeName);
    static Red::InstancePtr<> Construct(const Red::CBaseRTTIType* aType);

    static bool IsOriginalRecord(Red::TweakDBID aRecordId);
    static bool IsOriginalBaseRecord(Red::TweakDBID aParentId);
    static Red::TweakDBID GetOriginalParent(Red::TweakDBID aRecordId);
    static const Core::Set<Red::TweakDBID>& GetOriginalDescendants(Red::TweakDBID aSourceId);

    static void RegisterExtraFlat(Red::CName aRecordType, const std::string& aPropName, Red::CName aPropType,
                           Red::CName aForeignType);
    static void RegisterDescendants(Red::TweakDBID aParentId, const Core::Set<Red::TweakDBID>& aDescendantIds);

    static std::string ToString(Red::TweakDBID aID);

    static IRTTISystem* GetRTTI();

    explicit TweakDBReflection(Red::TweakDB* aTweakDb);

    const Red::TweakDBRecordInfo* GetRecordInfo(Red::CName aTypeName);
    const Red::TweakDBRecordInfo* GetRecordInfo(const Red::CClass* aType);
    Red::TweakDB* GetTweakDB();
    bool RegisterRecordInfo(const TweakDBRecordInfo& aRecordInfo);
    bool RegisterRecordInfo(TweakDBRecordInfo&& aRecordInfo);

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
    uint32_t GetRecordTypeHash(const Red::CClass* aType);
    std::string ResolvePropertyName(Red::TweakDBID aSampleId, Red::CName aGetterName);
    int32_t ResolveDefaultValue(const Red::CClass* aType, const std::string& aPropName);

    Red::TweakDB* m_tweakDb;
    RecordInfoMap m_resolved;
    std::shared_mutex m_mutex;

    inline static IRTTISystem* s_rtti;
    inline static ParentMap s_parentMap;
    inline static DescendantMap s_descendantMap;
    inline static ExtraFlatMap s_extraFlats;
};
}
