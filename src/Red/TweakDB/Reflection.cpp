#include "Reflection.hpp"
#include "Alias.hpp"
#include "Red/TweakDB/Source/Grammar.hpp"
#include "Red/TweakDB/Source/Source.hpp"

namespace
{
constexpr auto NameSeparator = Red::TweakGrammar::Name::Separator;
constexpr auto PropSeparator = std::string_view(NameSeparator);
} // namespace

Red::TweakDBReflection::TweakDBReflection()
    : TweakDBReflection(Red::TweakDB::Get())
{
}

Red::TweakDBReflection::TweakDBReflection(Red::TweakDB* aTweakDb)
    : m_tweakDb(aTweakDb)
    , m_rtti(Red::CRTTISystem::Get())
{
}

Red::RecordInfo Red::TweakDBReflection::GetRecordInfo(Red::CClass* aType, const bool aCollect)
{
    if (!TweakDBUtil::IsRecordType(aType))
    {
        return nullptr;
    }

    {
        std::shared_lock lockR(m_mutex);
        if (const auto iter = m_recordInfoByName.find(aType->GetName()); iter != m_recordInfoByName.end())
            return iter->second;
    }

    if (aCollect)
    {
        return CollectRecordInfo(aType);
    }

    return nullptr;
}

Red::RecordInfo Red::TweakDBReflection::GetRecordInfo(const std::string& aTypeName, const bool aCollect)
{
    return GetRecordInfo(TweakDBUtil::GetRecordFullName<Red::CName>(aTypeName), aCollect);
}

Red::RecordInfo Red::TweakDBReflection::GetRecordInfo(Red::CName aTypeName, const bool aCollect)
{
    {
        std::shared_lock lockR(m_mutex);
        if (const auto iter = m_recordInfoByName.find(aTypeName); iter != m_recordInfoByName.end())
        {
            return iter->second;
        }
    }

    if (aCollect)
    {
        return CollectRecordInfo(m_rtti->GetClass(aTypeName));
    }

    return nullptr;
}

Red::RecordInfo Red::TweakDBReflection::GetRecordInfo(const Red::CClass* aType, const bool aCollect)
{
    return GetRecordInfo(const_cast<Red::CClass*>(aType), aCollect);
}

Red::CName Red::TweakDBReflection::RegisterCName(const std::string& aName) const
{
    return Red::CName(aName.c_str());
}

Red::RecordInfo Red::TweakDBReflection::CollectRecordInfo(Red::CClass* aType, Red::TweakDBID aSampleId)
{
    if (!TweakDBUtil::IsRecordType(aType))
    {
        return nullptr;
    }

    auto sampleId = aSampleId;
    if (!sampleId.IsValid())
    {
        sampleId = GetRecordSampleId(aType);

        if (!sampleId.IsValid())
            return nullptr;
    }

    const auto recordInfo = CreateRecordInfo(aType);

    if (const auto parentInfo = CollectRecordInfo(aType->parent, sampleId))
    {
        InheritRecordInfo(recordInfo, parentInfo);
    }

    for (uint32_t funcIndex = 0u; funcIndex < aType->funcs.Size(); ++funcIndex)
    {
        const auto func = aType->funcs[funcIndex];

        auto propName = ResolvePropertyName(sampleId, func->shortName);

        // Case: Foreign Key Array => TweakDBID[]
        if (!func->returnType)
        {
            const auto arrayType = reinterpret_cast<Red::CRTTIArrayType*>(func->params[0]->type);
            const auto handleType = reinterpret_cast<Red::CRTTIWeakHandleType*>(arrayType->innerType);
            const auto recordType = reinterpret_cast<Red::CClass*>(handleType->innerType);

            const auto propInfo = CreatePropertyInfo(propName, Red::ERTDBFlatType::TweakDBIDArray);
            propInfo->foreignType = recordType;
            RegisterPropertyInfo(recordInfo, propInfo);

            // Skip related functions:
            // func Get[Prop]Count()
            // func Get[Prop]Item()
            // func Get[Prop]ItemHandle()
            // func [Prop]Contains()
            funcIndex += 4;
        }
        else
        {
            auto returnType = func->returnType->type;

            switch (returnType->GetType())
            {
            case Red::ERTTIType::WeakHandle:
            {
                // Case: Foreign Key => TweakDBID
                const auto handleType = reinterpret_cast<Red::CRTTIWeakHandleType*>(returnType);
                const auto recordType = reinterpret_cast<Red::CClass*>(handleType->innerType);

                const auto propInfo = CreatePropertyInfo(propName, Red::ERTDBFlatType::TweakDBID);
                propInfo->foreignType = recordType;
                RegisterPropertyInfo(recordInfo, propInfo);

                // Skip related function:
                // func Get[Prop]Handle()
                funcIndex += 1;
                break;
            }
            case Red::ERTTIType::Array:
            {
                if (TweakDBUtil::IsResRefTokenArray(returnType))
                {
                    RegisterPropertyInfo(recordInfo, CreatePropertyInfo(propName, Red::ERTDBFlatType::ResRefArray));

                    // Skip related functions:
                    // func Get[Prop]Count()
                    // func Get[Prop]Item()
                    funcIndex += 2;
                }
                else
                {
                    RegisterPropertyInfo(recordInfo, CreatePropertyInfo(propName, returnType));

                    // Skip related functions:
                    // func Get[Prop]Count()
                    // func Get[Prop]Item()
                    // func [Prop]Contains()
                    funcIndex += 3;
                }
                break;
            }
            case Red::ERTTIType::Enum:
            {
                // Some types have additional enum getters,
                // but they're not backed by any flat.
                continue;
            }
            default:
            {
                if (TweakDBUtil::IsResRefToken(returnType))
                {
                    RegisterPropertyInfo(recordInfo, CreatePropertyInfo(propName, Red::ERTDBFlatType::ResRef));
                }
                else if (returnType->GetType() == Red::ERTTIType::Name)
                {
                    // Getter for LocKey returns CName, so we have to get
                    // the actual property type from the flat value.
                    auto propId = sampleId + PropSeparator + propName;
                    auto flat = m_tweakDb->GetFlatValue(propId);

                    RegisterPropertyInfo(recordInfo, CreatePropertyInfo(propName, flat->GetValue().type));
                }
                else
                {
                    RegisterPropertyInfo(recordInfo, CreatePropertyInfo(propName, returnType));
                }
            }
            }
        }
    }

    {
        auto extraFlatsIt = s_extraFlats.find(aType->name);
        if (extraFlatsIt != s_extraFlats.end())
        {
            for (const auto& extraFlat : extraFlatsIt.value())
            {
                auto propInfo = CreatePropertyInfo(extraFlat.appendix.c_str() + 1, extraFlat.typeName);
                propInfo->isExtra = true;

                if (!extraFlat.foreignTypeName.IsNone())
                {
                    propInfo->foreignType = m_rtti->GetClass(extraFlat.foreignTypeName);
                    propInfo->isForeignKey = true;
                }

                RegisterPropertyInfo(recordInfo, propInfo);
            }
        }
    }

    for (const auto& propInfo : recordInfo->props | std::views::values)
    {
        if (!propInfo->isExtra)
        {
            propInfo->defaultValue = ResolveDefaultValue(aType, propInfo->appendix);
        }
    }

    assert(IsValid(recordInfo));

    RegisterRecordInfo(recordInfo);

    return recordInfo;
}

Red::TweakDBID Red::TweakDBReflection::GetRecordSampleId(const Red::CClass* aType)
{
    std::shared_lock<Red::SharedSpinLock> recordLockR(m_tweakDb->mutex01);
    auto* records = m_tweakDb->recordsByType.Get(const_cast<Red::CClass*>(aType));

    if (records == nullptr)
        return {};

    return records->Begin()->GetPtr<Red::TweakDBRecord>()->recordID;
}

std::string Red::TweakDBReflection::ResolvePropertyName(Red::TweakDBID aSampleId, Red::CName aGetterName)
{
    std::string propName = aGetterName.ToString();
    propName[0] = static_cast<char>(std::tolower(propName[0]));

    auto propId = aSampleId + PropSeparator + propName;

    std::shared_lock<Red::SharedSpinLock> flatLockR(m_tweakDb->mutex00);

    auto propFlat = m_tweakDb->flats.Find(propId);
    if (propFlat == m_tweakDb->flats.End())
        propName[0] = static_cast<char>(std::toupper(propName[0]));

    return propName;
}

std::optional<int32_t> Red::TweakDBReflection::ResolveDefaultValue(const Red::CClass* aType,
                                                                   const std::string& aPropName)
{
    std::string defaultFlatName = TweakSource::SchemaPackage;
    defaultFlatName.append(NameSeparator);
    defaultFlatName.append(TweakDBUtil::GetRecordShortName(aType->GetName()));

    if (!aPropName.starts_with(NameSeparator))
    {
        defaultFlatName.append(NameSeparator);
    }

    defaultFlatName.append(aPropName);

    const auto defaultFlatId = Red::TweakDBID(defaultFlatName);

    std::shared_lock<Red::SharedSpinLock> flatLockR(m_tweakDb->mutex00);

    auto defaultFlat = m_tweakDb->flats.Find(defaultFlatId);

    if (defaultFlat == m_tweakDb->flats.End())
        return std::nullopt;

    return defaultFlat->ToTDBOffset();
}

void Red::TweakDBReflection::RegisterExtraFlat(Red::CName aRecordType, const std::string& aPropName,
                                               Red::CName aPropType, Red::CName aForeignType)
{
    s_extraFlats[aRecordType].push_back({aPropType, aForeignType, NameSeparator + aPropName});
}

void Red::TweakDBReflection::RegisterDescendants(Red::TweakDBID aParentId,
                                                 const Core::Set<Red::TweakDBID>& aDescendantIds)
{
    s_descendantMap[aParentId].insert(aDescendantIds.begin(), aDescendantIds.end());

    for (const auto& descendantId : aDescendantIds)
    {
        s_parentMap[descendantId] = aParentId;
    }
}

bool Red::TweakDBReflection::RegisterRecordInfo(RecordInfo aRecordInfo)
{
    if (!IsValid(aRecordInfo))
        return false;

    std::unique_lock lockRW(m_mutex);
    if (const auto& [_, success] = m_recordInfoByName.insert({aRecordInfo->name, aRecordInfo}); success)
    {
        m_recordInfoByHash[aRecordInfo->typeHash] = aRecordInfo;
        return true;
    }

    return false;
}

void Red::TweakDBReflection::InheritRecordInfo(const RecordInfo& aRecordInfo, Red::RecordInfo aParentInfo)
{
    if (!aParentInfo)
        return;

    aRecordInfo->parent = aParentInfo->type;

    for (const auto parentProp : aParentInfo->props | std::views::values)
        RegisterPropertyInfo(aRecordInfo, parentProp);
}

bool Red::TweakDBReflection::RegisterPropertyInfo(const RecordInfo& aRecordInfo, const PropertyInfo& aPropertyInfo)
{
    if (!IsValid(aPropertyInfo))
        return false;

    aRecordInfo->props[aPropertyInfo->name] = aPropertyInfo;

    return true;
}

bool Red::TweakDBReflection::IsOriginalRecord(Red::TweakDBID aRecordId)
{
    return s_parentMap.contains(aRecordId);
}

bool Red::TweakDBReflection::IsOriginalBaseRecord(Red::TweakDBID aParentId)
{
    return s_descendantMap.contains(aParentId);
}

Red::TweakDBID Red::TweakDBReflection::GetOriginalParent(Red::TweakDBID aRecordId)
{
    return s_parentMap[aRecordId];
}

const Core::Set<Red::TweakDBID>& Red::TweakDBReflection::GetOriginalDescendants(Red::TweakDBID aSourceId)
{
    return s_descendantMap[aSourceId];
}

std::string Red::TweakDBReflection::ToString(Red::TweakDBID aID)
{
    Red::CString str;
    Red::CallStatic("gamedataTDBIDHelper", "ToStringDEBUG", str, aID);
    return {str.c_str(), str.Length()};
}

Red::TweakDB* Red::TweakDBReflection::GetTweakDB()
{
    return m_tweakDb;
}

bool Red::TweakDBReflection::IsValid(const PropertyInfo& aPropInfo)
{
    if (aPropInfo->name.IsNone() || !aPropInfo->type || aPropInfo->appendix.length() < 2 ||
        !TweakDBUtil::IsFlatType(aPropInfo->type->GetName()) || !aPropInfo->appendix.starts_with(NameSeparator))
    {
        return false;
    }

    if (!aPropInfo->isExtra && aPropInfo->functionName.IsNone())
    {
        return false;
    }

    switch (aPropInfo->type->GetName())
    {
    case ERTDBFlatType::Int:
    case ERTDBFlatType::Float:
    case ERTDBFlatType::Bool:
    case ERTDBFlatType::String:
    case ERTDBFlatType::CName:
    case ERTDBFlatType::LocKey:
    case ERTDBFlatType::ResRef:
    case ERTDBFlatType::Quaternion:
    case ERTDBFlatType::EulerAngles:
    case ERTDBFlatType::Vector3:
    case ERTDBFlatType::Vector2:
    case ERTDBFlatType::Color:
        return !aPropInfo->isArray && !aPropInfo->elementType && !aPropInfo->isForeignKey && !aPropInfo->foreignType;
    case ERTDBFlatType::IntArray:
    case ERTDBFlatType::FloatArray:
    case ERTDBFlatType::BoolArray:
    case ERTDBFlatType::StringArray:
    case ERTDBFlatType::CNameArray:
    case ERTDBFlatType::LocKeyArray:
    case ERTDBFlatType::ResRefArray:
    case ERTDBFlatType::QuaternionArray:
    case ERTDBFlatType::EulerAnglesArray:
    case ERTDBFlatType::Vector3Array:
    case ERTDBFlatType::Vector2Array:
    case ERTDBFlatType::ColorArray:
        return aPropInfo->isArray && aPropInfo->elementType && !aPropInfo->isForeignKey && !aPropInfo->foreignType &&
               aPropInfo->elementType->GetName() == TweakDBUtil::GetElementTypeName(aPropInfo->type);
    case ERTDBFlatType::TweakDBID:
        return !aPropInfo->isArray && !aPropInfo->elementType && aPropInfo->isForeignKey && aPropInfo->foreignType;
    case ERTDBFlatType::TweakDBIDArray:
        return aPropInfo->isArray && aPropInfo->elementType && aPropInfo->isForeignKey && aPropInfo->foreignType &&
               aPropInfo->elementType->GetName() == ERTDBFlatType::TweakDBID;
    default:
        return false;
    }
}

bool Red::TweakDBReflection::IsValid(const RecordInfo& aRecordInfo)
{
    if (aRecordInfo->name.IsNone() || aRecordInfo->aliasName.IsNone() || aRecordInfo->shortName.empty() ||
        !aRecordInfo->type || aRecordInfo->type->GetName() != aRecordInfo->name)
    {
        return false;
    }

    if (aRecordInfo->parent && !TweakDBUtil::IsRecordType(aRecordInfo->parent))
    {
        return false;
    }

    return true;
}

Red::TweakDBID Red::TweakDBReflection::BuildRTDBID(const std::string& aRecordName, Red::CName aPropertyName)
{
    std::string id = TweakSource::SchemaPackage;
    id.append(NameSeparator);
    id.append(aRecordName);
    id.append(NameSeparator);
    id.append(aPropertyName.ToString());
    return TweakDBID{id};
}

Red::RecordInfo Red::TweakDBReflection::CreateRecordInfo(Red::CClass* aClass)
{
    const auto recordInfo = Core::MakeShared<Red::TweakDBRecordInfo>();
    recordInfo->name = aClass->GetName();
    recordInfo->aliasName = TweakDBUtil::GetRecordAliasName<Red::CName>(recordInfo->name);
    recordInfo->shortName = TweakDBUtil::GetRecordShortName(recordInfo->name);
    recordInfo->type = aClass;
    recordInfo->typeHash = TweakDBUtil::GetRecordTypeHash(recordInfo->type);
    return recordInfo;
}

Red::PropertyInfo Red::TweakDBReflection::CreatePropertyInfo(const std::string& aName, const uint64_t aFlatType)
{
    return CreatePropertyInfo(aName, TweakDBUtil::GetFlatType(aFlatType));
}

Red::PropertyInfo Red::TweakDBReflection::CreatePropertyInfo(const std::string& aName,
                                                             const Red::CBaseRTTIType* aFlatType)
{
    const auto propInfo = Core::MakeShared<Red::TweakDBPropertyInfo>();
    propInfo->name = RegisterCName(aName);
    propInfo->type = aFlatType;
    propInfo->appendix = PropSeparator;
    propInfo->appendix.append(propInfo->name.ToString());
    propInfo->functionName = CNamePool::Add(TweakDBUtil::GetPropertyFunctionName(propInfo->name).c_str());

    if (TweakDBUtil::IsArrayType(propInfo->type))
    {
        propInfo->isArray = true;
        propInfo->elementType = TweakDBUtil::GetElementType(propInfo->type);
    }

    if (TweakDBUtil::IsForeignKey(propInfo->type) || TweakDBUtil::IsForeignKeyArray(propInfo->type))
    {
        propInfo->isForeignKey = true;
    }

    return propInfo;
}
