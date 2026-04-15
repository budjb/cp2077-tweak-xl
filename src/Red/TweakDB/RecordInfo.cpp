#include "RecordInfo.hpp"

#include "App/Tweaks/Record/CustomTweakDBRecord.hpp"
#include "Red/TweakDB/Reflection.hpp"

namespace
{

std::string Capitalize(const std::string& aStr)
{
    if (aStr.empty())
        return aStr;

    std::string result = aStr;
    result[0] = static_cast<char>(std::toupper(result[0]));
    return result;
}

} // namespace

namespace Red
{

static constexpr std::string_view NameSeparator = ".";

void TweakDBPropertyInfo::SetName(const char* aName)
{
    SetName(std::string(aName));
}

void TweakDBPropertyInfo::SetName(const std::string& aName)
{
    m_name = CNamePool::Add(aName.c_str());
    m_appendix = std::string(NameSeparator).append(aName);

    const std::string functionName = Capitalize(aName);
    m_functionName = CNamePool::Add(functionName.c_str());
}

void TweakDBPropertyInfo::SetType(const DeferredType& aType)
{
    m_type = aType;
}

void TweakDBPropertyInfo::SetElementType(const DeferredType& aType)
{
    m_elementType = aType;
}

void TweakDBPropertyInfo::SetForeignType(const DeferredType& aType)
{
    m_foreignType = aType;
}

void TweakDBPropertyInfo::SetArray(const bool aArray)
{
    m_isArray = aArray;
}

void TweakDBPropertyInfo::SetForeignKey(const bool aForeignKey)
{
    m_isForeignKey = aForeignKey;
}

void TweakDBPropertyInfo::SetDataOffset(uintptr_t aDataOffset)
{
    SetDataOffset(aDataOffset > 0 ? std::optional{aDataOffset} : std::nullopt);
}

void TweakDBPropertyInfo::SetDataOffset(const std::optional<uintptr_t> aDataOffset)
{
    m_dataOffset = aDataOffset > 0 ? std::optional{aDataOffset} : std::nullopt;
}

void TweakDBPropertyInfo::SetDefaultValue(int32_t aDefaultValue)
{
    SetDefaultValue(aDefaultValue > 0 ? std::optional{aDefaultValue} : std::nullopt);
}

void TweakDBPropertyInfo::SetDefaultValue(const std::optional<int32_t> aDefaultValue)
{
    m_defaultValue = aDefaultValue;
}

CName TweakDBPropertyInfo::GetName() const
{
    return m_name;
}

CName TweakDBPropertyInfo::GetFunctionName() const
{
    return m_functionName;
}

DeferredType TweakDBPropertyInfo::GetType() const
{
    return m_type;
}

DeferredType TweakDBPropertyInfo::GetElementType() const
{
    return m_elementType;
}

DeferredType TweakDBPropertyInfo::GetForeignType() const
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

std::optional<uintptr_t> TweakDBPropertyInfo::GetDataOffset() const
{
    return m_dataOffset;
}

std::optional<int32_t> TweakDBPropertyInfo::GetDefaultValue() const
{
    return m_defaultValue;
}

bool TweakDBPropertyInfo::IsValid() const
{
    if (m_name.IsNone() || m_functionName.IsNone() || !m_type || m_appendix.length() < 2 ||
        !m_appendix.starts_with(NameSeparator))
    {
        return false;
    }

    if (m_type.IsResolved() && !TweakDBReflection::IsFlatType(m_type.GetName()))
    {
        return false;
    }

    switch (m_type.GetHash())
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
               m_elementType.GetHash() == TweakDBReflection::GetElementTypeName(m_type.GetHash());
    case ERTDBFlatType::TweakDBID:
        return !m_isArray && !m_elementType && m_isForeignKey && m_foreignType;
    case ERTDBFlatType::TweakDBIDArray:
        return m_isArray && m_elementType && m_isForeignKey && m_foreignType &&
               m_elementType.GetHash() == ERTDBFlatType::TweakDBID;
    default:
        return false;
    }
}

void TweakDBRecordInfo::SetType(DeferredType aType)
{
    const auto name = TweakDBReflection::GetRecordFullName<std::string>(aType.GetHash());

    m_type = aType;
    m_aliasName = TweakDBReflection::GetRecordAliasName<CName>(name.c_str());
    m_shortName = TweakDBReflection::GetRecordShortName<std::string>(name.c_str());
    m_typeHash = TweakDBReflection::GetRecordTypeHash(m_shortName);
}

void TweakDBRecordInfo::SetParent(DeferredType aType)
{
    m_parent = aType;
}

void TweakDBRecordInfo::SetCustom(const bool aCustom)
{
    m_isCustom = aCustom;

    if (!m_parent)
        m_parent = App::CustomTweakDBRecord::NAME;
}

Core::SharedPtr<const TweakDBPropertyInfo> TweakDBRecordInfo::AddProperty(
    Core::SharedPtr<TweakDBPropertyInfo> aProperty)
{
    if (!aProperty->IsValid())
    {
        return nullptr;
    }

    m_props[aProperty->GetName()] = aProperty;
    return aProperty;
}

CName TweakDBRecordInfo::GetAliasName() const
{
    return m_aliasName;
}

const std::string& TweakDBRecordInfo::GetShortName() const
{
    return m_shortName;
}

DeferredType TweakDBRecordInfo::GetType() const
{
    return m_type;
}

DeferredType TweakDBRecordInfo::GetParent() const
{
    return m_parent;
}

TweakDBRecordHash TweakDBRecordInfo::GetTypeHash() const
{
    return m_typeHash;
}

bool TweakDBRecordInfo::IsCustom() const
{
    return m_isCustom;
}

Core::SharedPtr<const TweakDBPropertyInfo> TweakDBRecordInfo::GetProperty(CName aPropName) const
{
    const auto& propIt = m_props.find(aPropName);
    return propIt != m_props.end() ? propIt->second : nullptr;
}

const Core::Map<CName, Core::SharedPtr<const TweakDBPropertyInfo>>& TweakDBRecordInfo::GetProperties() const
{
    return m_props;
}

bool TweakDBRecordInfo::IsValid() const
{
    if (!m_type || m_aliasName.IsNone() || m_shortName.empty())
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
    m_parent = aOther.m_type;
    m_props.insert(aOther.m_props.begin(), aOther.m_props.end());

    return *this;
}

} // namespace Red
