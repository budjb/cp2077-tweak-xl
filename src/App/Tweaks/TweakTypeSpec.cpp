#include "TweakTypeSpec.hpp"

namespace
{
constexpr auto ArrayPrefix = Red::GetTypePrefixStr<Red::DynArray>();
constexpr auto ArrayPrefixSize = ArrayPrefix.size() - 1;

using ResRefArrayType = Red::TypeLocator<Red::GetTypeName<Red::DynArray<Red::ResRef>>()>;
using ResRefType = Red::TypeLocator<Red::GetTypeName<Red::ResRef>()>;

using LocKeyArrayType = Red::TypeLocator<Red::ERTDBFlatType::LocKeyArray>;
using LocKeyType = Red::TypeLocator<Red::ERTDBFlatType::LocKey>;
} // namespace

namespace App
{
TweakTypeSpecPtr GetTweakTypeSpec(const std::string& aValue)
{
    using namespace Red::ERTDBFlatType;

    // Attempt to load a spec for non-foreign-key types
    if (auto spec = GetTweakTypeSpec(Red::CName(aValue.c_str())))
        return spec;

    // Attempt to look up foreign key arrays using shorthand syntax (e.g. "array:SomeType")
    if (aValue.starts_with(ArrayPrefix.data()) && aValue.length() > ArrayPrefixSize)
        return GetTweakTypeSpec(TweakDBIDArray, aValue.substr(ArrayPrefixSize));

    // Assume the name is a foreign key weak handle type using shorthand (e.g. "SomeType")
    return GetTweakTypeSpec(TweakDBID, aValue);
}

TweakTypeSpecPtr GetTweakTypeSpec(const char* aValue)
{
    return GetTweakTypeSpec(std::string(aValue));
}

TweakTypeSpecPtr GetTweakTypeSpec(Red::CName aName, const std::optional<std::string>& aForeignType)
{
    using namespace Red::TweakDBUtil;

    static Red::CRTTISystem* rtti = Red::CRTTISystem::Get();

    if (!IsFlatType(aName))
        return nullptr;

    const auto isArray = IsArrayType(aName);
    const auto isForeignKey = isArray ? IsForeignKeyArray(aName) : IsForeignKey(aName);
    const auto isResRef = isArray ? IsResRefTokenArray(aName) : IsResRefToken(aName);
    const auto isLocKey = isArray ? IsLocKeyArray(aName) : IsLocKey(aName);

    if (isForeignKey && !aForeignType.has_value())
        return nullptr;

    if (!isForeignKey && aForeignType.has_value())
        return nullptr;

    auto spec = Core::MakeShared<TweakTypeSpec>();

    spec->flatType = GetFlatType(aName);
    spec->flatTypeName = aName;
    spec->propertyType = spec->flatType;
    spec->propertyTypeName = spec->flatTypeName;

    spec->isArray = isArray;
    spec->isForeignKey = isForeignKey;
    spec->isResRef = isResRef;
    spec->isLocKey = isLocKey;

    if (isForeignKey)
    {
        spec->foreignTypeName = Red::CNamePool::Add(NormalizeRecordName(*aForeignType).c_str());
        spec->foreignType = rtti->GetClass(spec->foreignTypeName);
    }
    else if (isResRef)
    {
        spec->propertyType = isArray ? ResRefArrayType::Get() : ResRefType::Get();
        spec->propertyTypeName = spec->propertyType->GetName();
    }

    return spec;
}
} // namespace App