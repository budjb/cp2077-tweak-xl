#include "RecordInfo.hpp"

#include "Red/TweakDB/Reflection.hpp"

namespace Red
{

void TweakDBPropertyInfo::SetName(const char* aName)
{
    assert(!m_finalized);

    SetName(CNamePool::Add(aName));
}

void TweakDBPropertyInfo::SetName(const CName& aName)
{
    assert(!m_finalized);

    m_name = aName;
}

void TweakDBPropertyInfo::SetType(const CBaseRTTIType* aType)
{
    assert(!m_finalized);

    m_type = aType;
}

void TweakDBPropertyInfo::SetElementType(const CBaseRTTIType* aElementType)
{
    assert(!m_finalized);

    m_elementType = aElementType;
}

void TweakDBPropertyInfo::SetForeignType(const CClass* aForeignType)
{
    assert(!m_finalized);

    m_foreignType = aForeignType;
}

void TweakDBPropertyInfo::SetArray(bool aIsArray)
{
    assert(!m_finalized);

    m_isArray = aIsArray;
}

void TweakDBPropertyInfo::SetForeignKey(bool aIsForeignKey)
{
    assert(!m_finalized);

    m_isForeignKey = aIsForeignKey;
}

void TweakDBPropertyInfo::SetAppendix(const std::string& aAppendix)
{
    assert(!m_finalized);

    m_appendix = aAppendix;
}

void TweakDBPropertyInfo::SetDataOffset(uintptr_t aDataOffset)
{
    assert(!m_finalized);

    m_dataOffset = aDataOffset;
}

void TweakDBPropertyInfo::SetDefaultValue(int32_t aDefaultValue)
{
    assert(!m_finalized);

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

bool TweakDBPropertyInfo::Finalize()
{
    if (m_finalized)
    {
        return true;
    }

    if (!IsValid())
    {
        return false;
    }

    m_finalized = true;
    return true;
}

bool TweakDBPropertyInfo::IsFinalized() const
{
    return m_finalized;
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

} // namespace Red
