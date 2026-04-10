#pragma once
#include "Reflection.Flats.hpp"

#include <memory>
#include <string>

namespace Red
{

#pragma region Utilities

template<typename V, typename F, typename K = std::remove_cvref_t<std::invoke_result_t<F, const V&>>>
Core::Map<K, V> ToMap(const Core::Vector<V>& v, F&& k)
{
    Core::Map<K, V> result;

    for (const auto& i : v)
    {
        result.emplace(k(i), i);
    }

    return result;
}

template<typename F, typename K>
    requires(std::is_function_v<std::remove_pointer_t<F>>)
auto operator|(const Core::Vector<K>& v, F f) -> decltype(f(v))
{
    return f(v);
}

inline std::string Capitalize(const std::string& str)
{
    std::string result = str;

    result[0] = static_cast<char>(std::toupper(result[0]));

    return result;
}

inline CName ToCName(const std::string& str)
{
    return CNamePool::Add(str.c_str());
}

template<typename F>
    requires(std::is_function_v<std::remove_pointer_t<F>>)
auto operator|(const std::string& str, F f) -> decltype(f(str))
{
    return f(str);
}

#pragma endregion

#pragma region TweakDBPropertySchema

/**
 * @brief Indicates the ownership type of flat property in relation to its record.
 */
enum class PropertyStorage
{
    /**
     * @brief The flat is a direct property of the record.
     */
    CLASS,

    /**
     * @brief The flat is not part of the record but shares its base record ID.
     */
    FLAT,

    /**
     * @brief The flat is stored in an RTTI value holder.
     */
    VALUE_HOLDER
};

class TweakDBPropertySchema
{
public:
    using PropertyPtr = Core::SharedPtr<TweakDBPropertySchema>;

    class Builder
    {
    public:
        explicit Builder(std::string name);

        Builder& FlatType(const TDBFlatType& aFlatType);
        Builder& Offset(uintptr_t aOffset);
        Builder& ForeignClass(const ResolvableType& aClass);
        Builder& PropertyStorage(PropertyStorage aOwnership);
        Builder& DefaultValueOffset(int32_t aOffset);
        Builder& DefaultValue(void* aValue);

        [[nodiscard]] PropertyPtr Build() const;

    private:
        std::string m_name;
        TDBFlatType* m_flatType = nullptr;
        uintptr_t m_offset = 0;
        ResolvableType m_foreignClass{CName{}};
        Red::PropertyStorage m_propertyStorage = PropertyStorage::CLASS;
        int32_t m_defaultValueOffset = 0;
        Core::SharedPtr<void*> m_defaultValue;
    };

    static PropertyPtr From(const ExtraFlat& aExtraFlat);

    [[nodiscard]] const CName& GetName() const;
    [[nodiscard]] const std::string& GetFlatSuffix() const;
    [[nodiscard]] const CName& GetFunctionName() const;
    [[nodiscard]] const TDBFlatType& GetType() const;
    [[nodiscard]] uintptr_t GetOffset() const;
    [[nodiscard]] PropertyStorage GetPropertyStorage() const;
    [[nodiscard]] const Core::SharedPtr<void*>& GetDefaultValue() const;
    [[nodiscard]] int32_t GetDefaultValueOffset() const;
    [[nodiscard]] const ResolvableType& GetForeignClass() const;

private:
    friend class Builder;

    TweakDBPropertySchema(const std::string& aName, const TDBFlatType& aType, uintptr_t aOffset,
                          PropertyStorage aPropertyStorage, const Core::SharedPtr<void*>& aDefaultValue,
                          const ResolvableType& aForeignClass, int32_t aDefaultValueOffset);

    const CName name;
    const std::string flatSuffix;
    const CName functionName;
    const TDBFlatType& type;
    const uintptr_t m_offset;
    const PropertyStorage m_propertyStorage;
    const Core::SharedPtr<void*> m_defaultValue;

    mutable ResolvableType m_foreignClass;
    int32_t m_defaultValueOffset = 0;
};

enum class ESchemaType
{
    NATIVE,
    CUSTOM
};

#pragma endregion

#pragma region TweakDBRecordSchema

class TweakDBRecordSchema
{
public:
    using PropertyPtr = Core::SharedPtr<TweakDBPropertySchema>;
    using PropertiesList = Core::Vector<PropertyPtr>;
    using PropertiesMap = Core::Map<CName, PropertyPtr>;
    using SchemaPtr = Core::SharedPtr<TweakDBRecordSchema>;

    class Builder
    {
    public:
        explicit Builder(const CClass* aType);
        explicit Builder(const CName& aName);
        explicit Builder(const std::string& aName);

        Builder& SchemaType(ESchemaType aType);
        Builder& Parent(const CClass* aParent);
        Builder& Parent(const TweakDBRecordSchema& aParent);
        Builder& Parent(const std::string& aParent);
        Builder& Property(const PropertyPtr& aProperty);

        [[nodiscard]] SchemaPtr Build() const;

        Builder& operator+=(const TweakDBRecordSchema&);

    private:
        ResolvableType m_class{};
        ResolvableType m_parentClass{};
        ESchemaType m_schemaType = ESchemaType::CUSTOM;
        PropertiesList m_properties;
    };

    [[nodiscard]] const CName& GetFullName() const;
    [[nodiscard]] const CName& GetAliasName() const;
    [[nodiscard]] const CName& GetShortName() const;
    [[nodiscard]] const ResolvableType& GetParentClass() const;
    [[nodiscard]] uint32_t GetHash() const;
    [[nodiscard]] ESchemaType GetSchemaType() const;
    [[nodiscard]] const ResolvableType& GetClass() const;
    [[nodiscard]] PropertyPtr GetProperty(const char* aName) const;
    [[nodiscard]] PropertyPtr GetProperty(const CName& aName) const;
    [[nodiscard]] const PropertiesMap& GetProperties() const;

private:
    friend class Builder;

    TweakDBRecordSchema(const ResolvableType& aClass, const ResolvableType& aParentClass, const ESchemaType& aType,
                        const PropertiesList& aProperties);

    const CName m_fullName;
    const CName m_aliasName;
    const CName m_shortName;
    mutable ResolvableType m_class;
    mutable ResolvableType m_parentClass;
    const uint32_t m_hash;
    const ESchemaType m_schemaType;
    const PropertiesMap m_properties;
};

#pragma endregion

} // namespace Red