#include "RecordInfo.hpp"

#include "Red/TweakDB/Reflection.hpp"

namespace Red
{
static constexpr std::string_view NameSeparator = ".";

void TweakDBPropertyInfo::SetName(const char* aName)
{
    m_name = CNamePool::Add(aName);
    m_appendix = std::string(NameSeparator) + aName;
}

void TweakDBPropertyInfo::SetName(const CName& aName)
{
    m_name = aName;
    m_appendix = std::string(NameSeparator) + aName.ToString();
}

void TweakDBPropertyInfo::SetType(const CBaseRTTIType* aType)
{
    m_type = aType;
}

void TweakDBPropertyInfo::SetElementType(const CBaseRTTIType* aElementType)
{
    m_elementType = aElementType;
}

void TweakDBPropertyInfo::SetForeignType(const CClass* aForeignType)
{
    m_foreignType = aForeignType;
}

void TweakDBPropertyInfo::SetArray(bool aIsArray)
{
    m_isArray = aIsArray;
}

void TweakDBPropertyInfo::SetForeignKey(bool aIsForeignKey)
{
    m_isForeignKey = aIsForeignKey;
}

void TweakDBPropertyInfo::SetDataOffset(uintptr_t aDataOffset)
{
    m_dataOffset = aDataOffset;
}

void TweakDBPropertyInfo::SetDefaultValue(int32_t aDefaultValue)
{
    m_defaultValue = aDefaultValue;
}

const CName& TweakDBPropertyInfo::GetName() const
{
    return m_name;
}

const CBaseRTTIType* TweakDBPropertyInfo::GetType() const
{
    return m_type;
}

const CBaseRTTIType* TweakDBPropertyInfo::GetElementType() const
{
    return m_elementType;
}

const CClass* TweakDBPropertyInfo::GetForeignType() const
{
    return m_foreignType;
}

bool TweakDBPropertyInfo::IsArray() const
{
    return m_isArray;
}

bool TweakDBPropertyInfo::IsForeignKey() const
{
    return m_isForeignKey;
}

const std::string& TweakDBPropertyInfo::GetAppendix() const
{
    return m_appendix;
}

uintptr_t TweakDBPropertyInfo::GetDataOffset() const
{
    return m_dataOffset;
}

int32_t TweakDBPropertyInfo::GetDefaultValue() const
{
    return m_defaultValue;
}

bool TweakDBPropertyInfo::IsValid() const
{
    if (m_name.IsNone() || !m_type || m_appendix.empty())
    {
        return false;
    }

    switch (m_type->GetName())
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
        return !m_isArray && !m_elementType && !m_isForeignKey && !m_foreignType;
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
        return m_isArray && m_elementType && !m_isForeignKey && !m_foreignType &&
               m_elementType->GetName() == TweakDBReflection::GetElementTypeName(m_type);
    case ERTDBFlatType::TweakDBID:
        return !m_isArray && !m_elementType && m_isForeignKey && m_foreignType;
    case ERTDBFlatType::TweakDBIDArray:
        return m_isArray && m_elementType && m_isForeignKey && m_foreignType &&
               m_elementType->GetName() == ERTDBFlatType::TweakDBID;
    default:
        return false;
    }
}

void TweakDBRecordInfo::SetName(const char* aName)
{
    m_name = TweakDBReflection::GetRecordFullName<CName>(aName);
    m_aliasName = TweakDBReflection::GetRecordAliasName<CName>(aName);
    m_shortName = TweakDBReflection::GetRecordShortName<CName>(aName);
    m_typeHash = TweakDBReflection::GetRecordTypeHash(m_shortName);
}

void TweakDBRecordInfo::SetName(const CName& aName)
{
    SetName(aName.ToString());
}

void TweakDBRecordInfo::SetType(const CClass* aType)
{
    m_type = aType;
}

void TweakDBRecordInfo::SetParent(const CClass* aParent)
{
    m_parent = aParent;
}

void TweakDBRecordInfo::SetExtraFlats(bool aExtraFlats)
{
    m_extraFlats = aExtraFlats;
}

bool TweakDBRecordInfo::AddProperty(const TweakDBPropertyInfo& aProperty)
{
    if (!aProperty.IsValid())
    {
        return false;
    }

    return m_props.insert({aProperty.GetName(), Core::MakeShared<TweakDBPropertyInfo>(aProperty)}).second;
}

bool TweakDBRecordInfo::AddProperty(TweakDBPropertyInfo&& aProperty)
{
    if (!aProperty.IsValid())
    {
        return false;
    }

    return m_props.insert({aProperty.GetName(), Core::MakeShared<TweakDBPropertyInfo>(std::move(aProperty))}).second;
}

const CName& TweakDBRecordInfo::GetName() const
{
    return m_name;
}

const CName& TweakDBRecordInfo::GetAliasName() const
{
    return m_aliasName;
}

const CName& TweakDBRecordInfo::GetShortName() const
{
    return m_shortName;
}

const CClass* TweakDBRecordInfo::GetType() const
{
    return m_type;
}

const CClass* TweakDBRecordInfo::GetParent() const
{
    return m_parent;
}

bool TweakDBRecordInfo::HasExtraFlats() const
{
    return m_extraFlats;
}

uint32_t TweakDBRecordInfo::GetTypeHash() const
{
    return m_typeHash;
}

const TweakDBPropertyInfo* TweakDBRecordInfo::GetProperty(const CName& aPropName) const
{
    const auto& propIt = m_props.find(aPropName);
    return propIt != m_props.end() ? propIt->second.get() : nullptr;
}

const Core::Map<CName, Core::SharedPtr<const TweakDBPropertyInfo>>& TweakDBRecordInfo::GetProperties() const
{
    return m_props;
}

bool TweakDBRecordInfo::IsValid() const
{
    if (m_name.IsNone() || m_aliasName.IsNone() || m_shortName.IsNone() || !m_type || !m_typeHash)
    {
        return false;
    }

    if (m_parent && !TweakDBReflection::IsRecordType(m_parent))
    {
        return false;
    }

    return true;
}

TweakDBRecordInfo& TweakDBRecordInfo::operator+=(const TweakDBRecordInfo* aOther)
{
    return operator+=(*aOther);
}

TweakDBRecordInfo& TweakDBRecordInfo::operator+=(const TweakDBRecordInfo& aOther)
{
    assert(m_type->parent == aOther.m_type);

    m_parent = aOther.m_type;
    m_extraFlats |= aOther.m_extraFlats;
    m_props.insert(aOther.m_props.begin(), aOther.m_props.end());

    return *this;
}

} // namespace Red
