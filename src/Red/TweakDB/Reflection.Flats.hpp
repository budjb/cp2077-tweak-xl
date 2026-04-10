#pragma once

#include "Alias.hpp"

#include <array>
#include <span>

namespace Red
{

class TDBFlatType
{
public:
    using Constructor = InstancePtr<> (*)();

    static const TDBFlatType Int;
    static const TDBFlatType Float;
    static const TDBFlatType Bool;
    static const TDBFlatType String;
    static const TDBFlatType CName;
    static const TDBFlatType LocKey;
    static const TDBFlatType ResRef;
    static const TDBFlatType TweakDBID;
    static const TDBFlatType Quaternion;
    static const TDBFlatType EulerAngles;
    static const TDBFlatType Vector3;
    static const TDBFlatType Vector2;
    static const TDBFlatType Color;
    static const TDBFlatType IntArray;
    static const TDBFlatType FloatArray;
    static const TDBFlatType BoolArray;
    static const TDBFlatType StringArray;
    static const TDBFlatType CNameArray;
    static const TDBFlatType LocKeyArray;
    static const TDBFlatType ResRefArray;
    static const TDBFlatType TweakDBIDArray;
    static const TDBFlatType QuaternionArray;
    static const TDBFlatType EulerAnglesArray;
    static const TDBFlatType Vector3Array;
    static const TDBFlatType Vector2Array;
    static const TDBFlatType ColorArray;

    [[nodiscard]] static const TDBFlatType* Get(uint64_t hash);
    [[nodiscard]] static const TDBFlatType* Get(const Red::CName& name);
    [[nodiscard]] static const TDBFlatType* Get(const CBaseRTTIType* type);
    [[nodiscard]] static const TDBFlatType* GetArrayType(const Red::CName& name);
    [[nodiscard]] static const TDBFlatType* GetArrayType(const TDBFlatType& aType);
    [[nodiscard]] static const TDBFlatType* GetArrayType(const CBaseRTTIType* aType);
    [[nodiscard]] static const TDBFlatType* GetElementType(const Red::CName& name);
    [[nodiscard]] static const TDBFlatType* GetElementType(const TDBFlatType& type);

    constexpr operator uint64_t() const
    {
        return m_hash;
    }

    operator Red::CName() const
    {
        return GetName();
    }

    operator const Red::CBaseRTTIType*() const
    {
        return GetClass();
    }

    constexpr bool operator==(const TDBFlatType& other) const
    {
        return m_hash == other.m_hash;
    }

    bool operator==(const Red::CName& name) const
    {
        return m_hash == static_cast<uint64_t>(name);
    }

    [[nodiscard]] constexpr uint64_t GetHash() const
    {
        return m_hash;
    }

    [[nodiscard]] constexpr const char* GetNameStr() const
    {
        return m_name;
    }

    [[nodiscard]] Red::CName GetName() const
    {
        return {m_hash};
    }

    [[nodiscard]] constexpr bool IsArray() const
    {
        return m_array;
    }

    [[nodiscard]] constexpr bool IsForeignKey() const
    {
        return m_foreignKey;
    }

    [[nodiscard]] InstancePtr<> Construct() const
    {
        return m_constructor ? m_constructor() : InstancePtr{};
    }

    [[nodiscard]] constexpr const TDBFlatType* GetElementType() const
    {
        return m_elementType ? m_elementType : this;
    }

    const CClass* GetClass() const
    {
        return CRTTISystem::Get()->GetClass(GetName());
    }

    [[nodiscard]] static std::span<const TDBFlatType* const> GetTypes();

    [[nodiscard]] static constexpr size_t GetSize()
    {
        return s_types.size();
    }

    template<typename TNative>
    static InstancePtr<> ConstructNative()
    {
        return MakeInstance<TNative>();
    }

private:
    constexpr TDBFlatType(const char* name, const bool array, const bool foreignKey, const Constructor constructor,
                          const TDBFlatType* elementType = nullptr)
        : m_name(name)
        , m_hash(FNV1a64(name))
        , m_array(array)
        , m_foreignKey(foreignKey)
        , m_constructor(constructor)
        , m_elementType(elementType)
    {
    }

    static constexpr std::array s_types{
        &Int,
        &Float,
        &Bool,
        &String,
        &CName,
        &LocKey,
        &ResRef,
        &TweakDBID,
        &Quaternion,
        &EulerAngles,
        &Vector3,
        &Vector2,
        &Color,
        &IntArray,
        &FloatArray,
        &BoolArray,
        &StringArray,
        &CNameArray,
        &LocKeyArray,
        &ResRefArray,
        &TweakDBIDArray,
        &QuaternionArray,
        &EulerAnglesArray,
        &Vector3Array,
        &Vector2Array,
        &ColorArray,
    };

    static std::span<const TDBFlatType* const> GetTypeList();

    const char* m_name;
    const uint64_t m_hash;
    const bool m_array;
    const bool m_foreignKey;
    const Constructor m_constructor;
    const TDBFlatType* m_elementType;
};

#define TDB_FLAT_TYPE(_prop, _name, _fk, _type)                                                                        \
    inline constexpr TDBFlatType TDBFlatType::_prop =                                                                  \
        TDBFlatType(_name, false, _fk, &TDBFlatType::ConstructNative<_type>);                                          \
    inline constexpr TDBFlatType TDBFlatType::_prop##Array =                                                           \
        TDBFlatType("array:" _name, true, _fk, &TDBFlatType::ConstructNative<DynArray<_type>>, &TDBFlatType::_prop)

TDB_FLAT_TYPE(Int, "Int32", false, int);
TDB_FLAT_TYPE(Float, "Float", false, float);
TDB_FLAT_TYPE(Bool, "Bool", false, bool);
TDB_FLAT_TYPE(String, "String", false, Red::CString);
TDB_FLAT_TYPE(CName, "CName", false, Red::CName);
TDB_FLAT_TYPE(LocKey, "gamedataLocKeyWrapper", false, Red::LocKeyWrapper);
TDB_FLAT_TYPE(ResRef, "raRef:CResource", false, Red::ResourceAsyncReference<>);
TDB_FLAT_TYPE(TweakDBID, "TweakDBID", true, Red::TweakDBID);
TDB_FLAT_TYPE(Quaternion, "Quaternion", false, Red::Quaternion);
TDB_FLAT_TYPE(EulerAngles, "EulerAngles", false, Red::EulerAngles);
TDB_FLAT_TYPE(Vector3, "Vector3", false, Red::Vector3);
TDB_FLAT_TYPE(Vector2, "Vector2", false, Red::Vector2);
TDB_FLAT_TYPE(Color, "Color", false, Red::Color);

class ExtraFlat
{
public:
    ExtraFlat(const CName& flatType, const CName& foreignTypeName, const std::string& name)
        : flatType(flatType)
        , foreignTypeName(foreignTypeName)
        , name(name)
    {
    }

    const CName& flatType;
    const CName& foreignTypeName;
    const std::string& name;
};

inline std::span<const TDBFlatType* const> TDBFlatType::GetTypeList()
{
    return s_types;
}

inline std::span<const TDBFlatType* const> TDBFlatType::GetTypes()
{
    return GetTypeList();
}

inline const TDBFlatType* TDBFlatType::Get(const uint64_t hash)
{
    for (const auto* type : GetTypeList())
    {
        if (type->m_hash == hash)
            return type;
    }

    return nullptr;
}

inline const TDBFlatType* TDBFlatType::Get(const Red::CName& name)
{
    return Get(static_cast<uint64_t>(name));
}

inline const TDBFlatType* TDBFlatType::Get(const CBaseRTTIType* type)
{
    if (type)
        return Get(type->GetName());

    return nullptr;
}

inline const TDBFlatType* TDBFlatType::GetArrayType(const TDBFlatType& aType)
{
    if (aType.IsArray())
        return &aType;

    for (const auto* candidate : GetTypeList())
    {
        if (candidate->IsArray() && candidate->m_elementType == &aType)
            return candidate;
    }

    return nullptr;
}

inline const TDBFlatType* TDBFlatType::GetArrayType(const Red::CName& name)
{
    if (const auto* type = Get(name))
        return GetArrayType(*type);

    return nullptr;
}

inline const TDBFlatType* TDBFlatType::GetArrayType(const CBaseRTTIType* aType)
{
    if (aType)
        return GetArrayType(aType->GetName());

    return nullptr;
}

inline const TDBFlatType* TDBFlatType::GetElementType(const Red::CName& name)
{
    if (const auto* type = Get(name))
        return GetElementType(*type);

    return nullptr;
}

inline const TDBFlatType* TDBFlatType::GetElementType(const TDBFlatType& type)
{
    return type.GetElementType();
}

} // namespace Red