#include "Reflection.hpp"
#include "Red/TweakDB/Source/Grammar.hpp"
#include "Red/TweakDB/Source/Source.hpp"

namespace
{
constexpr auto NameSeparator = Red::TweakGrammar::Name::Separator;
constexpr auto PropSeparator = std::string_view(NameSeparator);
constexpr auto DataOffsetSize = 12;
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

const Red::TweakDBRecordInfo* Red::TweakDBReflection::GetRecordInfo(const Red::CClass* aType)
{
    if (!TweakDBUtil::IsRecordType(aType))
        return nullptr;

    {
        std::shared_lock lockR(m_mutex);
        auto iter = m_resolved.find(aType->GetName());
        if (iter != m_resolved.end())
            return iter->second.get();
    }

    return CollectRecordInfo(aType).get();
}

const Red::TweakDBRecordInfo* Red::TweakDBReflection::GetRecordInfo(Red::CName aTypeName)
{
    {
        std::shared_lock lockR(m_mutex);
        auto iter = m_resolved.find(aTypeName);
        if (iter != m_resolved.end())
            return iter->second.get();
    }

    return CollectRecordInfo(m_rtti->GetClass(aTypeName)).get();
}

Core::SharedPtr<Red::TweakDBRecordInfo> Red::TweakDBReflection::CollectRecordInfo(const Red::CClass* aType,
                                                                                  Red::TweakDBID aSampleId)
{
    if (!TweakDBUtil::IsRecordType(aType))
        return nullptr;

    auto sampleId = aSampleId;
    if (!sampleId.IsValid())
    {
        sampleId = GetRecordSampleId(aType);

        if (!sampleId.IsValid())
            return nullptr;
    }

    auto recordInfo = Red::MakeInstance<Red::TweakDBRecordInfo>();
    recordInfo->name = aType->name;
    recordInfo->type = aType;
    recordInfo->typeHash = TweakDBUtil::GetRecordTypeHash(aType);
    recordInfo->shortName = TweakDBUtil::GetRecordShortName<std::string>(aType->name);

    const auto parentInfo = CollectRecordInfo(aType->parent, sampleId);
    if (parentInfo)
    {
        recordInfo->parent = aType->parent;
        recordInfo->props.insert(parentInfo->props.begin(), parentInfo->props.end());
    }

    const auto baseOffset = aType->parent->size;

    for (uint32_t funcIndex = 0u; funcIndex < aType->funcs.Size(); ++funcIndex)
    {
        const auto func = aType->funcs[funcIndex];

        auto propName = ResolvePropertyName(sampleId, func->shortName);

        auto propInfo = Red::MakeInstance<Red::TweakDBPropertyInfo>();
        propInfo->name = Red::CName(propName.c_str());

        // Case: Foreign Key Array => TweakDBID[]
        if (!func->returnType)
        {
            const auto arrayType = reinterpret_cast<Red::CRTTIArrayType*>(func->params[0]->type);
            const auto handleType = reinterpret_cast<Red::CRTTIWeakHandleType*>(arrayType->innerType);
            const auto recordType = reinterpret_cast<Red::CClass*>(handleType->innerType);

            propInfo->type = m_rtti->GetType(Red::ERTDBFlatType::TweakDBIDArray);
            propInfo->isArray = true;
            propInfo->elementType = m_rtti->GetType(Red::ERTDBFlatType::TweakDBID);
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
            auto returnType = func->returnType->type;

            switch (returnType->GetType())
            {
            case Red::ERTTIType::WeakHandle:
            {
                // Case: Foreign Key => TweakDBID
                const auto handleType = reinterpret_cast<Red::CRTTIWeakHandleType*>(returnType);
                const auto recordType = reinterpret_cast<Red::CClass*>(handleType->innerType);

                propInfo->type = m_rtti->GetType(Red::ERTDBFlatType::TweakDBID);
                propInfo->isForeignKey = true;
                propInfo->foreignType = recordType;

                // Skip related function:
                // func Get[Prop]Handle()
                funcIndex += 1;
                break;
            }
            case Red::ERTTIType::Array:
            {
                if (TweakDBUtil::IsResRefTokenArray(returnType))
                {
                    propInfo->type = m_rtti->GetType(Red::ERTDBFlatType::ResRefArray);
                    propInfo->isArray = true;
                    propInfo->elementType = m_rtti->GetType(Red::ERTDBFlatType::ResRef);

                    // Skip related functions:
                    // func Get[Prop]Count()
                    // func Get[Prop]Item()
                    funcIndex += 2;
                }
                else
                {
                    const auto arrayType = reinterpret_cast<Red::CRTTIArrayType*>(returnType);
                    const auto elementType = reinterpret_cast<Red::CBaseRTTIType*>(arrayType->innerType);

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
                    propInfo->type = m_rtti->GetType(Red::ERTDBFlatType::ResRef);
                }
                else
                {
                    // Getter for LocKey returns CName, so we have to get
                    // the actual property type from the flat value.
                    if (returnType->GetType() == Red::ERTTIType::Name)
                    {
                        auto propId = sampleId + PropSeparator + propName;
                        auto flat = m_tweakDb->GetFlatValue(propId);
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
            for (const auto& extraFlat : extraFlatsIt.value())
            {
                auto propInfo = Red::MakeInstance<Red::TweakDBPropertyInfo>();
                propInfo->name = Red::CName(extraFlat.appendix.c_str() + 1);
                propInfo->appendix = extraFlat.appendix;
                propInfo->type = m_rtti->GetType(extraFlat.typeName);

                if (propInfo->type->GetType() == Red::ERTTIType::Array)
                {
                    const auto arrayType = reinterpret_cast<const Red::CRTTIArrayType*>(propInfo->type);
                    propInfo->elementType = arrayType->innerType;
                    propInfo->isArray = true;
                }

                if (!extraFlat.foreignTypeName.IsNone())
                {
                    propInfo->foreignType = m_rtti->GetClass(extraFlat.foreignTypeName);
                    propInfo->isForeignKey = true;
                }

                propInfo->isExtra = true;
                recordInfo->props[propInfo->name] = propInfo;
            }
        }
    }

    for (auto& [_, propInfo] : recordInfo->props)
    {
        if (!propInfo->isExtra)
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

int32_t Red::TweakDBReflection::ResolveDefaultValue(const Red::CClass* aType, const std::string& aPropName)
{
    std::string defaultFlatName = TweakSource::SchemaPackage;
    defaultFlatName.append(NameSeparator);
    defaultFlatName.append(TweakDBUtil::GetRecordShortName<std::string>(aType->GetName()));

    if (!aPropName.starts_with(NameSeparator))
    {
        defaultFlatName.append(NameSeparator);
    }

    defaultFlatName.append(aPropName);

    const auto defaultFlatId = Red::TweakDBID(defaultFlatName);

    std::shared_lock<Red::SharedSpinLock> flatLockR(m_tweakDb->mutex00);

    auto defaultFlat = m_tweakDb->flats.Find(defaultFlatId);

    if (defaultFlat == m_tweakDb->flats.End())
        return -1;

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
