#include "TweakPropertySpec.hpp"

namespace
{
template<template<typename> class T>
constexpr auto TypePrefixChars = Red::GetTypePrefixStr<T>();

template<template<typename> class T>
constexpr auto ArrayTypePrefixChars = []() constexpr {
    constexpr auto arrayPrefix = Red::GetTypePrefixStr<Red::DynArray>();
    constexpr auto innerPrefix = Red::GetTypePrefixStr<T>();
    return Red::Detail::ConcatConstStr<arrayPrefix.size() - 1, innerPrefix.size() - 1>(arrayPrefix.data(),
                                                                                       innerPrefix.data());
}();

template<template<typename> class T>
constexpr const char* GetTypePrefixCString()
{
    return TypePrefixChars<T>.data();
}

template<template<typename> class T>
constexpr const char* GetArrayTypePrefixCString()
{
    return ArrayTypePrefixChars<T>.data();
}

template<template<typename> class T>
struct IsRefContainer : std::false_type
{
};

template<>
struct IsRefContainer<Red::Handle> : std::true_type
{
};

template<>
struct IsRefContainer<Red::WeakHandle> : std::true_type
{
};

template<typename T>
struct PropertyPrefix;

template<typename T>
struct TypePrefixResolver;

template<template<typename> class TContainer, typename TValue>
    requires(IsRefContainer<TContainer>::value)
struct TypePrefixResolver<TContainer<TValue>>
{
    static constexpr const char* value = TypePrefixChars<TContainer>.data();
};

template<template<typename> class TContainer, typename TValue>
    requires(IsRefContainer<TContainer>::value)
struct TypePrefixResolver<Red::DynArray<TContainer<TValue>>>
{
    static constexpr const char* value = ArrayTypePrefixChars<TContainer>.data();
};

template<typename T>
constexpr const char* GetTypePrefixCString()
{
    return TypePrefixResolver<T>::value;
}

template<template<typename> class TContainer, typename TValue>
    requires(IsRefContainer<TContainer>::value)
struct PropertyPrefix<TContainer<TValue>>
{
    static constexpr const char* value = GetTypePrefixCString<TContainer<TValue>>();
};

template<template<typename> class TContainer, typename TValue>
    requires(IsRefContainer<TContainer>::value)
struct PropertyPrefix<Red::DynArray<TContainer<TValue>>>
{
    static constexpr const char* value = GetTypePrefixCString<Red::DynArray<TContainer<TValue>>>();
};

template<template<typename> class TRefContainer>
    requires(IsRefContainer<TRefContainer>::value)
std::string GetPropertyTypeName(const std::string& aName)
{
    std::string name = PropertyPrefix<TRefContainer<Red::ISerializable>>::value;
    name.append(Red::TweakDBUtil::GetRecordFullName<std::string>(aName));
    return name;
}

template<template<typename> class TRefContainer>
    requires(IsRefContainer<TRefContainer>::value)
std::string GetArrayPropertyTypeName(const std::string& aName)
{
    std::string name = PropertyPrefix<Red::DynArray<TRefContainer<Red::ISerializable>>>::value;
    name.append(Red::TweakDBUtil::GetRecordFullName<std::string>(aName));
    return name;
}

constexpr auto ArrayPrefix = GetTypePrefixCString<Red::DynArray>();
constexpr auto ArrayPrefixSize = std::char_traits<char>::length(ArrayPrefix);

constexpr auto HandlePrefix = GetTypePrefixCString<Red::Handle>();
constexpr auto HandlePrefixSize = std::char_traits<char>::length(HandlePrefix);

constexpr auto WeakHandlePrefix = GetTypePrefixCString<Red::WeakHandle>();
constexpr auto WeakHandlePrefixSize = std::char_traits<char>::length(WeakHandlePrefix);

constexpr auto HandleArrayPrefix = GetArrayTypePrefixCString<Red::Handle>();
constexpr auto HandleArrayPrefixSize = std::char_traits<char>::length(HandleArrayPrefix);

constexpr auto WeakHandleArrayPrefix = GetArrayTypePrefixCString<Red::WeakHandle>();
constexpr auto WeakHandleArrayPrefixSize = std::char_traits<char>::length(WeakHandleArrayPrefix);
} // namespace

namespace App
{
TweakPropertySpecPtr GetTweakPropertySpec(const std::string& aValue)
{
    // Attempt to load a spec for non-foreign-key types
    if (auto spec = GetTweakPropertySpec(aValue, Red::CName(aValue.c_str())))
        return spec;

    // Attempt to look up foreign key arrays using shorthand syntax (e.g. "array:SomeType")
    if (aValue.starts_with(ArrayPrefix) && aValue.length() > ArrayPrefixSize)
        return GetTweakPropertySpec(aValue, Red::ERTDBFlatType::TweakDBIDArray, aValue.substr(ArrayPrefixSize));

    // Attempt to look up foreign key arrays of handles using full syntax (e.g. "array:handle:gamedataSomeType_Record")
    if (aValue.starts_with(HandleArrayPrefix) && aValue.length() > HandleArrayPrefixSize)
        return GetTweakPropertySpec(aValue, Red::ERTDBFlatType::TweakDBIDArray, aValue.substr(HandleArrayPrefixSize));

    // Attempt to look up foreign key arrays of weak handles using full syntax (e.g.
    // "array:whandle:gamedataSomeType_Record")
    if (aValue.starts_with(WeakHandleArrayPrefix) && aValue.length() > WeakHandleArrayPrefixSize)
        return GetTweakPropertySpec(aValue, Red::ERTDBFlatType::TweakDBIDArray,
                                    aValue.substr(WeakHandleArrayPrefixSize));

    // Attempt to look up foreign key handle types using full syntax (e.g. "handle:SomeType")
    if (aValue.starts_with(HandlePrefix) && aValue.length() > HandlePrefixSize)
        return GetTweakPropertySpec(aValue, Red::ERTDBFlatType::TweakDBID, aValue.substr(HandlePrefixSize));

    // Attempt to look up foreign key weak handle types using full syntax (e.g. "whandle:SomeType")
    if (aValue.starts_with(WeakHandlePrefix) && aValue.length() > WeakHandlePrefixSize)
        return GetTweakPropertySpec(aValue, Red::ERTDBFlatType::TweakDBID, aValue.substr(WeakHandlePrefixSize));

    // Assume the name is a foreign key weak handle type using shorthand (e.g. "SomeType")
    return GetTweakPropertySpec(aValue, Red::ERTDBFlatType::TweakDBID, aValue);
}

TweakPropertySpecPtr GetTweakPropertySpec(const std::string& aValue, const uint64_t aHash,
                                          const std::optional<std::string>& aForeignType)
{
    static Red::CRTTISystem* rtti = Red::CRTTISystem::Get();

    if (!Red::TweakDBUtil::IsFlatType(aHash))
        return nullptr;

    const auto isArray = Red::TweakDBUtil::IsArrayType(aHash);
    const auto isForeignKey =
        isArray ? Red::TweakDBUtil::IsForeignKeyArray(aHash) : Red::TweakDBUtil::IsForeignKey(aHash);

    if (isForeignKey && !aForeignType.has_value())
        return nullptr;

    if (!isForeignKey && aForeignType.has_value())
        return nullptr;

    auto spec = Core::MakeShared<TweakPropertySpec>();
    spec->foreignName = aValue;
    spec->flatType = Red::TweakDBUtil::GetFlatType(aHash);
    spec->flatTypeName = aHash;
    spec->isArray = isArray;

    if (isForeignKey)
    {
        const auto foreignName = Red::TweakDBUtil::NormalizeRecordName(*aForeignType);

        const bool isExplicitHandleArray = aValue.starts_with(HandleArrayPrefix);
        const bool isExplicitHandle = aValue.starts_with(HandlePrefix);

        const auto propertyTypeName =
            isArray ? (isExplicitHandleArray ? GetArrayPropertyTypeName<Red::Handle>(foreignName)
                                             : GetArrayPropertyTypeName<Red::WeakHandle>(foreignName))
                    : (isExplicitHandle ? GetPropertyTypeName<Red::Handle>(foreignName)
                                        : GetPropertyTypeName<Red::WeakHandle>(foreignName));

        spec->isForeignKey = true;
        spec->propertyTypeName = Red::CNamePool::Add(propertyTypeName.c_str());
        spec->propertyType = rtti->GetType(spec->propertyTypeName);
        spec->foreignTypeName = Red::CNamePool::Add(foreignName.c_str());
        spec->foreignType = rtti->GetClass(spec->foreignTypeName);
    }
    else
    {
        spec->propertyType = spec->flatType;
        spec->propertyTypeName = spec->flatTypeName;
    }

    return spec;
}
} // namespace App