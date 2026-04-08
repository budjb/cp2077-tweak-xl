#include "Reflection.hpp"

#include "Alias.hpp"
#include "Red/TweakDB/Source/Grammar.hpp"
#include "Red/TweakDB/Source/Source.hpp"

namespace
{

constexpr auto RecordTypePrefix = "gamedata";
constexpr auto RecordTypePrefixLength = std::char_traits<char>::length(RecordTypePrefix);
constexpr auto RecordTypeSuffix = "_Record";
constexpr auto RecordTypeSuffixLength = std::char_traits<char>::length(RecordTypeSuffix);

constexpr auto BaseRecordTypeName = Red::GetTypeName<Red::TweakDBRecord>();

constexpr auto ResRefTypeName = Red::GetTypeName<Red::RaRef<Red::CResource>>();
constexpr auto ResRefArrayTypeName = Red::GetTypeName<Red::DynArray<Red::RaRef<Red::CResource>>>();

constexpr auto ScriptResRefTypeName = Red::GetTypeName<Red::ResRef>();
constexpr auto ScriptResRefArrayTypeName = Red::GetTypeName<Red::DynArray<Red::ResRef>>();

constexpr auto NameSeparator = Red::TweakGrammar::Name::Separator;
constexpr auto PropSeparator = std::string_view(NameSeparator);
constexpr auto DataOffsetSize = 12;

} // namespace

namespace Red
{

TweakDBReflection::TweakDBReflection()
    : TweakDBReflection(TweakDB::Get())
{
}

TweakDBReflection::TweakDBReflection(TweakDB* aTweakDb)
    : m_tweakDb(aTweakDb)
{
}

const TweakDBRecordInfo* TweakDBReflection::GetRecordInfo(const CClass* aType)
{
    if (!IsRecordType(aType))
        return nullptr;

    {
        std::shared_lock lockR(m_mutex);
        if (const auto iter = m_resolved.find(aType->GetName()); iter != m_resolved.end())
            return iter->second.get();
    }

    return CollectRecordInfo(aType).get();
}

const TweakDBRecordInfo* TweakDBReflection::GetRecordInfo(const CName& aTypeName)
{
    {
        std::shared_lock lockR(m_mutex);
        if (const auto iter = m_resolved.find(aTypeName); iter != m_resolved.end())
            return iter->second.get();
    }

    return CollectRecordInfo(GetRTTI()->GetClass(aTypeName)).get();
}

Core::SharedPtr<TweakDBRecordInfo> TweakDBReflection::CollectRecordInfo(const CClass* aType, const TweakDBID aSampleId)
{
    if (!IsRecordType(aType))
        return nullptr;

    return CollectNativeRecordInfo(aType, aSampleId);
}

Core::SharedPtr<TweakDBRecordInfo> TweakDBReflection::CollectNativeRecordInfo(const CClass* aType,
                                                                              const TweakDBID aSampleId)
{
    auto sampleId = aSampleId;
    if (!sampleId.IsValid())
    {
        sampleId = GetRecordSampleId(aType);

        if (!sampleId.IsValid())
            return nullptr;
    }

    auto recordInfo = MakeInstance<TweakDBRecordInfo>();
    recordInfo->name = aType->name;
    recordInfo->type = aType;
    recordInfo->typeHash = GetRecordTypeHash(aType);
    recordInfo->shortName = GetRecordShortName(aType->name);

    if (const auto parentInfo = CollectRecordInfo(aType->parent, sampleId))
    {
        recordInfo->parent = aType->parent;
        recordInfo->props.insert(parentInfo->props.begin(), parentInfo->props.end());
        recordInfo->extraFlats = parentInfo->extraFlats;
    }

    const auto baseOffset = aType->parent->size;

    for (uint32_t funcIndex = 0u; funcIndex < aType->funcs.size; ++funcIndex)
    {
        const auto func = aType->funcs[funcIndex];

        auto propName = ResolvePropertyName(sampleId, func->shortName);

        auto propInfo = MakeInstance<TweakDBPropertyInfo>();
        propInfo->name = CName(propName.c_str());
        propInfo->dataOffset = baseOffset + (recordInfo->props.size() * DataOffsetSize);

        // Case: Foreign Key Array => TweakDBID[]
        if (!func->returnType)
        {
            const auto arrayType = reinterpret_cast<CRTTIArrayType*>(func->params[0]->type);
            const auto handleType = reinterpret_cast<CRTTIWeakHandleType*>(arrayType->innerType);
            const auto recordType = reinterpret_cast<CClass*>(handleType->innerType);

            propInfo->type = GetRTTI()->GetType(ERTDBFlatType::TweakDBIDArray);
            propInfo->isArray = true;
            propInfo->elementType = GetRTTI()->GetType(ERTDBFlatType::TweakDBID);
            propInfo->isForeignKey = true;
            propInfo->foreignType = recordType;

            // Skip related functions:
            // func Get[Prop]Count()
            // func Get[Prop]Item()
            // func Get[Prop]ItemHandle()
            // func [Prop]Contains()
            funcIndex += 4;
        }
        else
        {
            switch (auto returnType = func->returnType->type; returnType->GetType())
            {
            case ERTTIType::WeakHandle:
            {
                // Case: Foreign Key => TweakDBID
                const auto handleType = reinterpret_cast<CRTTIWeakHandleType*>(returnType);
                const auto recordType = reinterpret_cast<CClass*>(handleType->innerType);

                propInfo->type = GetRTTI()->GetType(ERTDBFlatType::TweakDBID);
                propInfo->isForeignKey = true;
                propInfo->foreignType = recordType;

                // Skip related function:
                // func Get[Prop]Handle()
                funcIndex += 1;
                break;
            }
            case ERTTIType::Array:
            {
                if (IsResRefTokenArray(returnType))
                {
                    propInfo->type = GetRTTI()->GetType(ERTDBFlatType::ResRefArray);
                    propInfo->isArray = true;
                    propInfo->elementType = GetRTTI()->GetType(ERTDBFlatType::ResRef);

                    // Skip related functions:
                    // func Get[Prop]Count()
                    // func Get[Prop]Item()
                    funcIndex += 2;
                }
                else
                {
                    const auto arrayType = reinterpret_cast<CRTTIArrayType*>(returnType);
                    const auto elementType = arrayType->innerType;

                    propInfo->type = returnType;
                    propInfo->isArray = true;
                    propInfo->elementType = elementType;

                    // Skip related functions:
                    // func Get[Prop]Count()
                    // func Get[Prop]Item()
                    // func [Prop]Contains()
                    funcIndex += 3;
                }
                break;
            }
            case ERTTIType::Enum:
            {
                // Some types have additional enum getters,
                // but they're not backed by any flat.
                continue;
            }
            default:
            {
                if (IsResRefToken(returnType))
                {
                    propInfo->type = GetRTTI()->GetType(ERTDBFlatType::ResRef);
                }
                else
                {
                    // Getter for LocKey returns CName, so we have to get
                    // the actual property type from the flat value.
                    if (returnType->GetType() == ERTTIType::Name)
                    {
                        const auto propId = sampleId + PropSeparator + propName;
                        const auto flat = m_tweakDb->GetFlatValue(propId);
                        returnType = flat->GetValue().type;
                    }

                    propInfo->type = returnType;
                }
            }
            }
        }

        assert(propInfo->type);

        propInfo->appendix = PropSeparator;
        propInfo->appendix.append(propName);

        recordInfo->props[propInfo->name] = propInfo;
    }

    {
        auto extraFlatsIt = s_extraFlats.find(aType->name);
        if (extraFlatsIt != s_extraFlats.end())
        {
            recordInfo->extraFlats = true;

            for (const auto& [typeName, foreignTypeName, appendix] : extraFlatsIt.value())
            {
                const auto propInfo = MakeInstance<TweakDBPropertyInfo>();
                propInfo->name = CName(appendix.c_str() + 1);
                propInfo->appendix = appendix;
                propInfo->type = GetRTTI()->GetType(typeName);

                if (propInfo->type->GetType() == ERTTIType::Array)
                {
                    const auto arrayType = reinterpret_cast<const CRTTIArrayType*>(propInfo->type);
                    propInfo->elementType = arrayType->innerType;
                    propInfo->isArray = true;
                }

                if (!foreignTypeName.IsNone())
                {
                    propInfo->foreignType = GetRTTI()->GetClass(foreignTypeName);
                    propInfo->isForeignKey = true;
                }

                propInfo->dataOffset = 0;
                propInfo->defaultValue = -1;

                recordInfo->props[propInfo->name] = propInfo;
            }
        }
    }

    for (const auto& propInfo : recordInfo->props | std::views::values)
    {
        if (propInfo->dataOffset)
        {
            propInfo->defaultValue = ResolveDefaultValue(aType, propInfo->appendix);
        }
    }

    {
        std::unique_lock lockRW(m_mutex);
        m_resolved.insert({recordInfo->name, recordInfo});
    }

    return recordInfo;
}

// TODO: remove

TweakDBID TweakDBReflection::GetRecordSampleId(const CClass* aType) const
{
    std::shared_lock<SharedSpinLock> recordLockR(m_tweakDb->mutex01);
    auto* records = m_tweakDb->recordsByType.Get(const_cast<CClass*>(aType));

    if (records == nullptr)
        return {};

    return records->Begin()->GetPtr<TweakDBRecord>()->recordID;
}

uint32_t TweakDBReflection::GetRecordTypeHash(const CClass* aType)
{
    return GetRecordTypeHash(aType->GetName());
}

uint32_t TweakDBReflection::GetRecordTypeHash(const CName& aName)
{
    return Murmur3_32(GetRecordShortName(aName).c_str());
}

// TODO: do I need this?

std::string TweakDBReflection::ResolvePropertyName(const TweakDBID aSampleId, const CName& aGetterName) const
{
    std::string propName = aGetterName.ToString();
    propName[0] = static_cast<char>(std::tolower(propName[0]));

    const auto propId = aSampleId + PropSeparator + propName;

    std::shared_lock flatLockR(m_tweakDb->mutex00);

    if (const auto propFlat = m_tweakDb->flats.Find(propId); propFlat == m_tweakDb->flats.End())
        propName[0] = static_cast<char>(std::toupper(propName[0]));

    return propName;
}

// TODO: RTDB thangs
int32_t TweakDBReflection::ResolveDefaultValue(const CClass* aType, const std::string& aPropName) const
{
    std::string defaultFlatName = TweakSource::SchemaPackage;
    defaultFlatName.append(NameSeparator);
    defaultFlatName.append(GetRecordShortName(aType->GetName()));

    if (!aPropName.starts_with(NameSeparator))
    {
        defaultFlatName.append(NameSeparator);
    }

    defaultFlatName.append(aPropName);

    const auto defaultFlatId = TweakDBID(defaultFlatName);

    std::shared_lock flatLockR(m_tweakDb->mutex00);

    const auto defaultFlat = m_tweakDb->flats.Find(defaultFlatId);

    if (defaultFlat == m_tweakDb->flats.End())
        return -1;

    return defaultFlat->ToTDBOffset();
}

CRTTISystem* TweakDBReflection::GetRTTI()
{
    static CRTTISystem* rtti;

    if (!rtti)
    {
        rtti = CRTTISystem::Get();
    }
    return rtti;
}

const CBaseRTTIType* TweakDBReflection::GetFlatType(const CName& aTypeName)
{
    const CBaseRTTIType* type = GetRTTI()->GetType(aTypeName);

    if (!IsFlatType(type))
        return nullptr;

    return type;
}

const CClass* TweakDBReflection::GetRecordType(const CName& aTypeName)
{
    return GetRecordType(aTypeName.ToString());
}

const CClass* TweakDBReflection::GetRecordType(const char* aTypeName)
{
    const auto aFullName = CName(GetRecordFullName(aTypeName).c_str());

    const CClass* type = GetRTTI()->GetClass(aFullName);

    if (!IsRecordType(type))
        return nullptr;

    return type;
}

CBaseRTTIType* TweakDBReflection::GetArrayType(const CName& aTypeName)
{
    return GetRTTI()->GetType(GetArrayTypeName(aTypeName));
}

CBaseRTTIType* TweakDBReflection::GetArrayType(const CBaseRTTIType* aType)
{
    return GetRTTI()->GetType(GetArrayTypeName(aType));
}

CBaseRTTIType* TweakDBReflection::GetElementType(const CName& aTypeName)
{
    return GetElementType(GetRTTI()->GetType(aTypeName));
}

CBaseRTTIType* TweakDBReflection::GetElementType(const CBaseRTTIType* aType)
{
    if (!aType || aType->GetType() != ERTTIType::Array)
        return nullptr;

    return reinterpret_cast<const CRTTIBaseArrayType*>(aType)->innerType;
}

bool TweakDBReflection::IsFlatType(const CName& aTypeName)
{
    switch (aTypeName)
    {
    case ERTDBFlatType::Int:
    case ERTDBFlatType::Float:
    case ERTDBFlatType::Bool:
    case ERTDBFlatType::String:
    case ERTDBFlatType::CName:
    case ERTDBFlatType::LocKey:
    case ERTDBFlatType::ResRef:
    case ERTDBFlatType::TweakDBID:
    case ERTDBFlatType::Quaternion:
    case ERTDBFlatType::EulerAngles:
    case ERTDBFlatType::Vector3:
    case ERTDBFlatType::Vector2:
    case ERTDBFlatType::Color:
    case ERTDBFlatType::IntArray:
    case ERTDBFlatType::FloatArray:
    case ERTDBFlatType::BoolArray:
    case ERTDBFlatType::StringArray:
    case ERTDBFlatType::CNameArray:
    case ERTDBFlatType::LocKeyArray:
    case ERTDBFlatType::ResRefArray:
    case ERTDBFlatType::TweakDBIDArray:
    case ERTDBFlatType::QuaternionArray:
    case ERTDBFlatType::EulerAnglesArray:
    case ERTDBFlatType::Vector3Array:
    case ERTDBFlatType::Vector2Array:
    case ERTDBFlatType::ColorArray:
        return true;
    default:
        return false;
    }
}

bool TweakDBReflection::IsFlatType(const CBaseRTTIType* aType)
{
    return aType && IsFlatType(aType->GetName());
}

bool TweakDBReflection::IsRecordType(const CName& aTypeName)
{
    return aTypeName && IsRecordType(GetRTTI()->GetClass(aTypeName));
}

bool TweakDBReflection::IsRecordType(const CClass* aType)
{
    static CBaseRTTIType* s_baseRecordType = GetRTTI()->GetClass(BaseRecordTypeName);

    return aType && aType != s_baseRecordType && aType->IsA(s_baseRecordType);
}

bool TweakDBReflection::IsArrayType(const CName& aTypeName)
{
    switch (aTypeName)
    {
    case ERTDBFlatType::IntArray:
    case ERTDBFlatType::FloatArray:
    case ERTDBFlatType::BoolArray:
    case ERTDBFlatType::StringArray:
    case ERTDBFlatType::CNameArray:
    case ERTDBFlatType::LocKeyArray:
    case ERTDBFlatType::ResRefArray:
    case ERTDBFlatType::TweakDBIDArray:
    case ERTDBFlatType::QuaternionArray:
    case ERTDBFlatType::EulerAnglesArray:
    case ERTDBFlatType::Vector3Array:
    case ERTDBFlatType::Vector2Array:
    case ERTDBFlatType::ColorArray:
        return true;
    default:
        return false;
    }
}

bool TweakDBReflection::IsArrayType(const CBaseRTTIType* aType)
{
    return aType && IsArrayType(aType->GetName());
}

bool TweakDBReflection::IsForeignKey(const CName& aTypeName)
{
    return aTypeName == ERTDBFlatType::TweakDBID;
}

bool TweakDBReflection::IsForeignKey(const CBaseRTTIType* aType)
{
    return aType && IsForeignKey(aType->GetName());
}

bool TweakDBReflection::IsForeignKeyArray(const CName& aTypeName)
{
    return aTypeName == ERTDBFlatType::TweakDBIDArray;
}

bool TweakDBReflection::IsForeignKeyArray(const CBaseRTTIType* aType)
{
    return aType && IsForeignKeyArray(aType->GetName());
}

bool TweakDBReflection::IsResRefToken(const CName& aTypeName)
{
    return aTypeName == ScriptResRefTypeName || aTypeName == ResRefTypeName;
}

bool TweakDBReflection::IsResRefToken(const CBaseRTTIType* aType)
{
    return aType && IsResRefToken(aType->GetName());
}

bool TweakDBReflection::IsResRefTokenArray(const CName& aTypeName)
{
    return aTypeName == ScriptResRefArrayTypeName || aTypeName == ResRefArrayTypeName;
}

bool TweakDBReflection::IsResRefTokenArray(const CBaseRTTIType* aType)
{
    return aType && IsResRefTokenArray(aType->GetName());
}

CName TweakDBReflection::GetArrayTypeName(const CName& aTypeName)
{
    switch (aTypeName)
    {
    case ERTDBFlatType::Int:
        return ERTDBFlatType::IntArray;
    case ERTDBFlatType::Float:
        return ERTDBFlatType::FloatArray;
    case ERTDBFlatType::Bool:
        return ERTDBFlatType::BoolArray;
    case ERTDBFlatType::String:
        return ERTDBFlatType::StringArray;
    case ERTDBFlatType::CName:
        return ERTDBFlatType::CNameArray;
    case ERTDBFlatType::LocKey:
        return ERTDBFlatType::LocKeyArray;
    case ERTDBFlatType::ResRef:
        return ERTDBFlatType::ResRefArray;
    case ERTDBFlatType::TweakDBID:
        return ERTDBFlatType::TweakDBIDArray;
    case ERTDBFlatType::Quaternion:
        return ERTDBFlatType::QuaternionArray;
    case ERTDBFlatType::EulerAngles:
        return ERTDBFlatType::EulerAnglesArray;
    case ERTDBFlatType::Vector3:
        return ERTDBFlatType::Vector3Array;
    case ERTDBFlatType::Vector2:
        return ERTDBFlatType::Vector2Array;
    case ERTDBFlatType::Color:
        return ERTDBFlatType::ColorArray;
    default:
        return {};
    }
}

CName TweakDBReflection::GetArrayTypeName(const CBaseRTTIType* aType)
{
    if (!aType)
        return {};

    return GetArrayTypeName(aType->GetName());
}

CName TweakDBReflection::GetElementTypeName(const CName& aTypeName)
{
    switch (aTypeName)
    {
    case ERTDBFlatType::IntArray:
        return ERTDBFlatType::Int;
    case ERTDBFlatType::FloatArray:
        return ERTDBFlatType::Float;
    case ERTDBFlatType::BoolArray:
        return ERTDBFlatType::Bool;
    case ERTDBFlatType::StringArray:
        return ERTDBFlatType::String;
    case ERTDBFlatType::CNameArray:
        return ERTDBFlatType::CName;
    case ERTDBFlatType::TweakDBIDArray:
        return ERTDBFlatType::TweakDBID;
    case ERTDBFlatType::LocKeyArray:
        return ERTDBFlatType::LocKey;
    case ERTDBFlatType::ResRefArray:
        return ERTDBFlatType::ResRef;
    case ERTDBFlatType::QuaternionArray:
        return ERTDBFlatType::Quaternion;
    case ERTDBFlatType::EulerAnglesArray:
        return ERTDBFlatType::EulerAngles;
    case ERTDBFlatType::Vector3Array:
        return ERTDBFlatType::Vector3;
    case ERTDBFlatType::Vector2Array:
        return ERTDBFlatType::Vector2;
    case ERTDBFlatType::ColorArray:
        return ERTDBFlatType::Color;
    default:
        return {};
    }
}

CName TweakDBReflection::GetElementTypeName(const CBaseRTTIType* aType)
{
    if (!aType)
        return {};

    return GetElementTypeName(aType->GetName());
}

std::string TweakDBReflection::GetRecordFullName(const CName& aName)
{
    return GetRecordFullName(aName.ToString());
}

std::string TweakDBReflection::GetRecordFullName(const char* aName)
{
    std::string finalName = aName;

    if (finalName.empty())
        return {};

    if (!finalName.starts_with(RecordTypePrefix))
        finalName.insert(0, RecordTypePrefix);

    if (!finalName.ends_with(RecordTypeSuffix))
        finalName.append(RecordTypeSuffix);

    return finalName;
}

std::string TweakDBReflection::GetRecordShortName(const CName& aName)
{
    return GetRecordShortName(aName.ToString());
}

std::string TweakDBReflection::GetRecordShortName(const char* aName)
{
    std::string finalName = aName;

    if (finalName.starts_with(RecordTypePrefix))
        finalName.erase(0, RecordTypePrefixLength);

    if (finalName.ends_with(RecordTypeSuffix))
        finalName.erase(finalName.end() - RecordTypeSuffixLength, finalName.end());

    return finalName;
}

std::string TweakDBReflection::GetRecordAliasName(const CName& aName)
{
    return GetRecordAliasName(aName.ToString());
}

std::string TweakDBReflection::GetRecordAliasName(const char* aName)
{
    std::string finalName = aName;

    if (finalName.starts_with(RecordTypePrefix))
        finalName.erase(0, RecordTypePrefixLength);

    if (!finalName.ends_with(RecordTypeSuffix))
        finalName.append(RecordTypeSuffix);

    return finalName;
}

InstancePtr<> TweakDBReflection::Construct(const CName& aTypeName)
{
    switch (aTypeName)
    {
    case ERTDBFlatType::Int:
        return MakeInstance<int>();
    case ERTDBFlatType::Float:
        return MakeInstance<float>();
    case ERTDBFlatType::Bool:
        return MakeInstance<bool>();
    case ERTDBFlatType::String:
        return MakeInstance<CString>();
    case ERTDBFlatType::CName:
        return MakeInstance<CName>();
    case ERTDBFlatType::LocKey:
        return MakeInstance<LocKeyWrapper>();
    case ERTDBFlatType::ResRef:
        return MakeInstance<ResourceAsyncReference<>>();
    case ERTDBFlatType::TweakDBID:
        return MakeInstance<TweakDBID>();
    case ERTDBFlatType::Quaternion:
        return MakeInstance<Quaternion>();
    case ERTDBFlatType::EulerAngles:
        return MakeInstance<EulerAngles>();
    case ERTDBFlatType::Vector3:
        return MakeInstance<Vector3>();
    case ERTDBFlatType::Vector2:
        return MakeInstance<Vector2>();
    case ERTDBFlatType::Color:
        return MakeInstance<Color>();
    case ERTDBFlatType::IntArray:
        return MakeInstance<DynArray<int>>();
    case ERTDBFlatType::FloatArray:
        return MakeInstance<DynArray<float>>();
    case ERTDBFlatType::BoolArray:
        return MakeInstance<DynArray<bool>>();
    case ERTDBFlatType::StringArray:
        return MakeInstance<DynArray<CString>>();
    case ERTDBFlatType::CNameArray:
        return MakeInstance<DynArray<CName>>();
    case ERTDBFlatType::LocKeyArray:
        return MakeInstance<DynArray<LocKeyWrapper>>();
    case ERTDBFlatType::ResRefArray:
        return MakeInstance<DynArray<ResourceAsyncReference<>>>();
    case ERTDBFlatType::TweakDBIDArray:
        return MakeInstance<DynArray<TweakDBID>>();
    case ERTDBFlatType::QuaternionArray:
        return MakeInstance<DynArray<Quaternion>>();
    case ERTDBFlatType::EulerAnglesArray:
        return MakeInstance<DynArray<EulerAngles>>();
    case ERTDBFlatType::Vector3Array:
        return MakeInstance<DynArray<Vector3>>();
    case ERTDBFlatType::Vector2Array:
        return MakeInstance<DynArray<Vector2>>();
    case ERTDBFlatType::ColorArray:
        return MakeInstance<DynArray<Color>>();
    default:
        return {};
    }
}

InstancePtr<> TweakDBReflection::Construct(const CBaseRTTIType* aType)
{
    if (!aType)
        return {};

    return Construct(aType->GetName());
}

void TweakDBReflection::RegisterExtraFlat(const CName& aRecordType, const std::string& aPropName,
                                          const CName& aPropType, const CName& aForeignType)
{
    s_extraFlats[aRecordType].push_back({aPropType, aForeignType, NameSeparator + aPropName});
}

void TweakDBReflection::RegisterDescendants(const TweakDBID aParentId, const Core::Set<TweakDBID>& aDescendantIds)
{
    s_descendantMap[aParentId].insert(aDescendantIds.begin(), aDescendantIds.end());

    for (const auto& descendantId : aDescendantIds)
    {
        s_parentMap[descendantId] = aParentId;
    }
}

bool TweakDBReflection::IsOriginalRecord(const TweakDBID aRecordId)
{
    return s_parentMap.contains(aRecordId);
}

bool TweakDBReflection::IsOriginalBaseRecord(const TweakDBID aParentId)
{
    return s_descendantMap.contains(aParentId);
}

TweakDBID TweakDBReflection::GetOriginalParent(const TweakDBID aRecordId)
{
    return s_parentMap[aRecordId];
}

const Core::Set<TweakDBID>& TweakDBReflection::GetOriginalDescendants(const TweakDBID aSourceId)
{
    return s_descendantMap[aSourceId];
}

std::string TweakDBReflection::ToString(TweakDBID aID)
{
    CString str;
    CallStatic("gamedataTDBIDHelper", "ToStringDEBUG", str, aID);
    return {str.c_str(), str.Length()};
}

TweakDB* TweakDBReflection::GetTweakDB() const
{
    return m_tweakDb;
}

} // namespace Red
