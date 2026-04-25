#include "Alias.hpp"
#include "Source/Grammar.hpp"

namespace
{
constexpr auto RecordTypePrefix = "gamedata";
constexpr auto RecordTypePrefixLength = std::char_traits<char>::length(RecordTypePrefix);
constexpr auto RecordTypeSuffix = "_Record";
constexpr auto RecordTypeSuffixLength = std::char_traits<char>::length(RecordTypeSuffix);

constexpr auto BaseRecordTypeName = Red::GetTypeName<Red::TweakDBRecord>();

constexpr auto NameSeparator = Red::TweakGrammar::Name::Separator;
constexpr auto PropSeparator = std::string_view(NameSeparator);
} // namespace

namespace Red::TweakDBUtil
{

CClass* GetRecordType(CName aTypeName)
{
    return GetRecordType(aTypeName.ToString());
}

CClass* GetRecordType(const char* aTypeName)
{
    const auto aFullName = GetRecordFullName<CName>(aTypeName);

    CClass* type = CRTTISystem::Get()->GetClass(aFullName);

    if (!IsRecordType(type))
        return nullptr;

    return type;
}

bool IsRecordType(CName aTypeName)
{
    return aTypeName && IsRecordType(CRTTISystem::Get()->GetClass(aTypeName));
}

bool IsRecordType(const CClass* aType)
{
    static CBaseRTTIType* s_baseRecordType = CRTTISystem::Get()->GetClass(BaseRecordTypeName);

    return aType && aType != s_baseRecordType && aType->IsA(s_baseRecordType);
}

template<>
std::string GetRecordFullName(const std::string& aName)
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

template<>
std::string GetRecordFullName(const char* aName)
{
    if (aName)
    {
        return GetRecordFullName<std::string>(std::string(aName));
    }
    return {};
}

template<>
std::string GetRecordFullName(const CName aName)
{
    return GetRecordFullName<std::string>(aName.ToString());
}

template<>
std::string GetRecordAliasName(const std::string& aName)
{
    std::string finalName = aName;

    if (finalName.empty())
        return {};

    if (finalName.starts_with(RecordTypePrefix))
        finalName.erase(0, RecordTypePrefixLength);

    if (!finalName.ends_with(RecordTypeSuffix))
        finalName.append(RecordTypeSuffix);

    return finalName;
}

template<>
std::string GetRecordAliasName(const char* aName)
{
    if (aName)
    {
        return GetRecordAliasName<std::string>(std::string(aName));
    }
    return {};
}

template<>
std::string GetRecordAliasName(const CName aName)
{
    return GetRecordAliasName<std::string>(aName.ToString());
}

template<>
CName GetRecordFullName(const std::string& aName)
{
    return {GetRecordFullName<std::string>(aName).c_str()};
}

template<>
CName GetRecordFullName(const char* aName)
{
    if (aName)
    {
        return GetRecordFullName<CName>(std::string(aName));
    }
    return {};
}

template<>
CName GetRecordFullName(const CName aName)
{
    return GetRecordFullName<CName>(aName.ToString());
}

template<>
CName GetRecordAliasName(const std::string& aName)
{
    return CName(GetRecordAliasName<std::string>(aName).c_str());
}

template<>
CName GetRecordAliasName(const char* aName)
{
    if (aName)
    {
        return GetRecordAliasName<CName>(std::string(aName));
    }
    return {};
}

template<>
CName GetRecordAliasName(const CName aName)
{
    return GetRecordAliasName<CName>(aName.ToString());
}

template<>
std::string GetRecordShortName(const std::string& aName)
{
    std::string finalName = aName;

    if (finalName.starts_with(RecordTypePrefix))
        finalName.erase(0, RecordTypePrefixLength);

    if (finalName.ends_with(RecordTypeSuffix))
        finalName.erase(finalName.end() - RecordTypeSuffixLength, finalName.end());

    return finalName;
}

template<>
std::string GetRecordShortName(CName aName)
{
    return GetRecordShortName<std::string>(aName.ToString());
}

template<>
std::string GetRecordShortName(const char* aName)
{
    if (aName)
    {
        return GetRecordShortName<std::string>(std::string(aName));
    }
    return {};
}

template<>
CName GetRecordShortName(const std::string& aName)
{
    return CName{GetRecordShortName<std::string>(aName).c_str()};
}

template<>
CName GetRecordShortName(CName aName)
{
    return GetRecordShortName<CName>(aName.ToString());
}

template<>
CName GetRecordShortName(const char* aName)
{
    if (aName)
    {
        return GetRecordShortName<CName>(std::string(aName));
    }
    return {};
}

std::string GetPropertyFunctionName(CName aName)
{
    std::string propName = aName.ToString();
    propName[0] = static_cast<char>(std::toupper(propName[0]));
    return propName;
}

uint32_t GetRecordTypeHash(CName aName)
{
    return GetRecordTypeHash(aName.ToString());
}

uint32_t GetRecordTypeHash(const std::string& aName)
{
    return GetRecordTypeHash(aName.c_str());
}

uint32_t GetRecordTypeHash(const char* aName)
{
    const auto shortName = GetRecordShortName<std::string>(aName);
    return Murmur3_32(reinterpret_cast<const uint8_t*>(shortName.data()), shortName.size());
}

uint32_t GetRecordTypeHash(const CClass* aType)
{
    std::string_view name(aType->name.ToString());
    name.remove_prefix(RecordTypePrefixLength);
    name.remove_suffix(RecordTypeSuffixLength);

    return Murmur3_32(reinterpret_cast<const uint8_t*>(name.data()), name.size());
}

} // namespace Red::TweakDBUtil