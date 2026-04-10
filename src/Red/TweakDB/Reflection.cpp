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

const TweakDBRecordSchema* TweakDBReflection::GetRecordSchema(const CClass* aType)
{
    if (!IsRecordType(aType))
        return nullptr;

    {
        std::shared_lock lockR(m_mutex);
        if (const auto iter = m_schemas.find(aType->name); iter != m_schemas.end())
            return iter->second.get();
    }

    return CollectRecordInfo(aType).get();
}

const TweakDBRecordSchema* TweakDBReflection::GetRecordSchema(const CName& aTypeName)
{
    {
        std::shared_lock lockR(m_mutex);
        if (const auto iter = m_schemas.find(aTypeName); iter != m_schemas.end())
            return iter->second.get();
    }

    return CollectRecordInfo(GetRTTI()->GetClass(aTypeName)).get();
}

Core::SharedPtr<TweakDBRecordSchema> TweakDBReflection::CollectRecordInfo(const CClass* aType,
                                                                          const TweakDBID aSampleId)
{
    if (!IsRecordType(aType))
        return nullptr;

    auto sampleId = aSampleId;
    if (!sampleId.IsValid())
    {
        sampleId = GetRecordSampleId(aType);

        if (!sampleId.IsValid())
            return nullptr;
    }

    auto recordBuilder = TweakDBRecordSchema::Builder(aType);
    if (aType->flags.isNative)
        recordBuilder.SchemaType(ESchemaType::NATIVE);

    if (const auto parentSchema = CollectRecordInfo(aType->parent, sampleId))
        recordBuilder += *parentSchema;

    const auto baseOffset = aType->parent->size;

    for (uint32_t funcIndex = 0u, numProps = 0; funcIndex < aType->funcs.size; ++funcIndex, numProps++)
    {
        const auto* func = aType->funcs[funcIndex];

        auto propName = ResolvePropertyName(sampleId, func->shortName);

        auto propSchema = TweakDBPropertySchema::Builder(propName);
        propSchema.Offset(baseOffset + (numProps * DataOffsetSize));

        // Case: Foreign Key Array => TweakDBID[]
        if (!func->returnType)
        {
            const auto* arrayType = reinterpret_cast<CRTTIArrayType*>(func->params[0]->type);
            const auto* handleType = reinterpret_cast<CRTTIWeakHandleType*>(arrayType->innerType);
            const auto* recordType = reinterpret_cast<CClass*>(handleType->innerType);

            propSchema.FlatType(TDBFlatType::TweakDBIDArray);
            propSchema.ForeignClass(ResolvableType(recordType));

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
                const auto* handleType = reinterpret_cast<CRTTIWeakHandleType*>(returnType);
                const auto* recordType = reinterpret_cast<CClass*>(handleType->innerType);

                propSchema.FlatType(TDBFlatType::TweakDBID);
                propSchema.ForeignClass(ResolvableType(recordType));

                // Skip related function:
                // func Get[Prop]Handle()
                funcIndex += 1;
                break;
            }
            case ERTTIType::Array:
            {
                if (IsResRefTokenArray(returnType))
                {
                    propSchema.FlatType(TDBFlatType::ResRefArray);

                    // Skip related functions:
                    // func Get[Prop]Count()
                    // func Get[Prop]Item()
                    funcIndex += 2;
                }
                else
                {
                    const auto arrayType = reinterpret_cast<CRTTIArrayType*>(returnType);
                    const auto elementType = arrayType->innerType;

                    if (const auto* flatType = TDBFlatType::GetArrayType(elementType); flatType)
                        propSchema.FlatType(*flatType);
                    else
                        // TODO: log warning
                        continue;

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
                    propSchema.FlatType(TDBFlatType::ResRef);
                }
                else
                {
                    // Getter for LocKey returns CName, so we have to get
                    // the actual property type from the flat value.
                    if (returnType->GetType() == ERTTIType::Name)
                    {
                        const auto propId = sampleId + PropSeparator + propName;

                        TDBFlatType* flatType = nullptr;

                        if (auto* flat = m_tweakDb->GetFlatValue(propId))
                            flatType = const_cast<TDBFlatType*>(TDBFlatType::Get(flat->GetValue().type));

                        if (!flatType)
                            // TODO: log warning
                            continue;
                    }
                    else
                    {
                        if (const auto* flatType = TDBFlatType::Get(returnType); flatType)
                            propSchema.FlatType(*flatType);
                        else
                            // TODO: log warning
                            continue;
                    }
                }
            }
            }
        }

        recordBuilder.Property(propSchema.Build());
    }

    {
        if (auto extraFlatsIt = s_extraFlats.find(aType->name); extraFlatsIt != s_extraFlats.end())
        {
            for (const auto& extraFlat : extraFlatsIt.value())
            {
                if (const auto propInfo = TweakDBPropertySchema::From(extraFlat); propInfo)
                {
                    recordBuilder.Property(propInfo);
                }
                else
                {
                    // TODO: log warning
                }
            }
        }
    }

    const auto schema = recordBuilder.Build();

    {
        std::unique_lock lockRW(m_mutex);
        m_schemas.insert({schema->GetFullName(), schema});
    }

    return schema;
}

// TODO: move

TweakDBID TweakDBReflection::GetRecordSampleId(const CClass* aType) const
{
    std::shared_lock recordLockR(m_tweakDb->mutex01);
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
    return TDBFlatType::Get(aTypeName);
}

bool TweakDBReflection::IsFlatType(const CBaseRTTIType* aType)
{
    return aType && IsFlatType(aType->GetName());
}

bool TweakDBReflection::IsRecordType(const CName& aTypeName)
{
    return aTypeName && IsRecordType(GetRTTI()->GetClass(aTypeName));
}

// TODO: update this to account for custom records
bool TweakDBReflection::IsRecordType(const CClass* aType)
{
    static CBaseRTTIType* s_baseRecordType = GetRTTI()->GetClass(BaseRecordTypeName);

    return aType && aType != s_baseRecordType && aType->IsA(s_baseRecordType);
}

bool TweakDBReflection::IsArrayType(const CName& aTypeName)
{
    if (const auto* t = TDBFlatType::Get(aTypeName))
        return t->IsArray();
    return false;
}

bool TweakDBReflection::IsArrayType(const CBaseRTTIType* aType)
{
    return aType && IsArrayType(aType->GetName());
}

bool TweakDBReflection::IsForeignKey(const CName& aTypeName)
{
    return aTypeName == TDBFlatType::TweakDBID;
}

bool TweakDBReflection::IsForeignKey(const CBaseRTTIType* aType)
{
    return aType && IsForeignKey(aType->GetName());
}

bool TweakDBReflection::IsForeignKeyArray(const CName& aTypeName)
{
    return aTypeName == TDBFlatType::TweakDBIDArray;
}

bool TweakDBReflection::IsForeignKeyArray(const CBaseRTTIType* aType)
{
    return aType && IsForeignKeyArray(aType->GetName());
}

// TODO: why can't we add this to TDBFlatType?
bool TweakDBReflection::IsResRefToken(const CName& aTypeName)
{
    return aTypeName == ScriptResRefTypeName || aTypeName == ResRefTypeName;
}

bool TweakDBReflection::IsResRefToken(const CBaseRTTIType* aType)
{
    return aType && IsResRefToken(aType->GetName());
}

// TODO: why can't we add this to TDBFlatType?
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
    if (const auto* type = TDBFlatType::GetArrayType(aTypeName))
        return type->GetName();
    return {};
}

CName TweakDBReflection::GetArrayTypeName(const CBaseRTTIType* aType)
{
    if (aType)
        return GetArrayTypeName(aType->GetName());
    return {};
}

CName TweakDBReflection::GetElementTypeName(const CName& aTypeName)
{
    if (const auto* type = TDBFlatType::GetElementType(aTypeName))
        return type->GetName();
    return {};
}

CName TweakDBReflection::GetElementTypeName(const CBaseRTTIType* aType)
{
    if (aType)
        return GetElementTypeName(aType->GetName());
    return {};
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
    if (const auto* type = TDBFlatType::Get(aTypeName))
        return type->Construct();

    return {};
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
    s_extraFlats[aRecordType].push_back({aPropType, aForeignType, aPropName});
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
