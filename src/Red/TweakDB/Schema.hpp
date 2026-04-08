#pragma once

#include <utility>

#include "Red/TweakDB/Reflection.hpp"

#include "RED4ext/Scripting/Natives/Generated/game/data/UIIcon_Record.hpp"

namespace Red::Schema
{

using namespace Red;

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

enum class ESchemaType
{
    NATIVE,
    CUSTOM
};

#define TWEAKDB_PROPERTY_TYPE(_prop, _name, _array, _fk)                                                               \
    const TweakDBPropertyType TweakDBPropertyType::_prop = TweakDBPropertyType                                         \
    {                                                                                                                  \
        ERTDBFlatType::_prop, _name, _array, _fk                                                                       \
    }

class TweakDBPropertyType
{
public:
    static const TweakDBPropertyType Int;
    static const TweakDBPropertyType Float;
    static const TweakDBPropertyType Bool;
    static const TweakDBPropertyType String;
    static const TweakDBPropertyType CName;
    static const TweakDBPropertyType LocKey;
    static const TweakDBPropertyType ResRef;
    static const TweakDBPropertyType TweakDBID;
    static const TweakDBPropertyType Quaternion;
    static const TweakDBPropertyType EulerAngles;
    static const TweakDBPropertyType Vector3;
    static const TweakDBPropertyType Vector2;
    static const TweakDBPropertyType Color;
    static const TweakDBPropertyType IntArray;
    static const TweakDBPropertyType FloatArray;
    static const TweakDBPropertyType BoolArray;
    static const TweakDBPropertyType StringArray;
    static const TweakDBPropertyType CNameArray;
    static const TweakDBPropertyType LocKeyArray;
    static const TweakDBPropertyType ResRefArray;
    static const TweakDBPropertyType TweakDBIDArray;
    static const TweakDBPropertyType QuaternionArray;
    static const TweakDBPropertyType EulerAnglesArray;
    static const TweakDBPropertyType Vector3Array;
    static const TweakDBPropertyType Vector2Array;
    static const TweakDBPropertyType ColorArray;

    static TweakDBPropertyType* Get(const uint64_t hash)
    {
        if (const auto it = m_hashMap.find(hash); it != m_hashMap.end())
        {
            return it->second;
        }
        return nullptr;
    }

    static TweakDBPropertyType* Get(const std::string& name)
    {
        if (const auto it = m_nameMap.find(name); it != m_nameMap.end())
        {
            return it->second;
        }
        return nullptr;
    }

    explicit operator uint64_t() const
    {
        return m_hash;
    }

    bool operator==(const TweakDBPropertyType& other) const
    {
        return m_hash == other.m_hash;
    }

    [[nodiscard]] uint64_t GetHash() const
    {
        return m_hash;
    }

    [[nodiscard]] const std::string& GetName() const
    {
        return m_name;
    }

    [[nodiscard]] bool IsArray() const
    {
        return m_array;
    }

    [[nodiscard]] bool IsForeignKey() const
    {
        return m_foreignKey;
    }

private:
    inline static std::map<uint64_t, TweakDBPropertyType*> m_hashMap;
    inline static std::map<std::string, TweakDBPropertyType*> m_nameMap;

    TweakDBPropertyType(const uint64_t hash, std::string name, const bool array, const bool foreignKey)
        : m_hash(hash)
        , m_name(std::move(name))
        , m_array(array)
        , m_foreignKey(foreignKey)
    {
        m_hashMap[m_hash] = this;
        m_nameMap[m_name] = this;
    }

    const uint64_t m_hash;
    const std::string m_name;
    const bool m_array;
    const bool m_foreignKey;
};

TWEAKDB_PROPERTY_TYPE(Int, "Int32", false, false);
TWEAKDB_PROPERTY_TYPE(Float, "Float", false, false);
TWEAKDB_PROPERTY_TYPE(Bool, "Bool", false, false);
TWEAKDB_PROPERTY_TYPE(String, "String", false, false);
TWEAKDB_PROPERTY_TYPE(CName, "CName", false, false);
TWEAKDB_PROPERTY_TYPE(LocKey, "gamedataLocKeyWrapper", false, false);
TWEAKDB_PROPERTY_TYPE(ResRef, "raRef:CResource", false, false);
TWEAKDB_PROPERTY_TYPE(TweakDBID, "TweakDBID", false, true);
TWEAKDB_PROPERTY_TYPE(Quaternion, "Quaternion", false, false);
TWEAKDB_PROPERTY_TYPE(EulerAngles, "EulerAngles", false, false);
TWEAKDB_PROPERTY_TYPE(Vector3, "Vector3", false, false);
TWEAKDB_PROPERTY_TYPE(Vector2, "Vector2", false, false);
TWEAKDB_PROPERTY_TYPE(Color, "Color", false, false);
TWEAKDB_PROPERTY_TYPE(IntArray, "array:Int32", true, false);
TWEAKDB_PROPERTY_TYPE(FloatArray, "array:Float", true, false);
TWEAKDB_PROPERTY_TYPE(BoolArray, "array:Bool", true, false);
TWEAKDB_PROPERTY_TYPE(StringArray, "array:String", true, false);
TWEAKDB_PROPERTY_TYPE(CNameArray, "array:CName", true, false);
TWEAKDB_PROPERTY_TYPE(LocKeyArray, "array:gamedataLocKeyWrapper", true, false);
TWEAKDB_PROPERTY_TYPE(ResRefArray, "array:raRef:CResource", true, false);
TWEAKDB_PROPERTY_TYPE(TweakDBIDArray, "array:TweakDBID", true, true);
TWEAKDB_PROPERTY_TYPE(QuaternionArray, "array:Quaternion", true, false);
TWEAKDB_PROPERTY_TYPE(EulerAnglesArray, "array:EulerAngles", true, false);
TWEAKDB_PROPERTY_TYPE(Vector3Array, "array:Vector3", true, false);
TWEAKDB_PROPERTY_TYPE(Vector2Array, "array:Vector2", true, false);
TWEAKDB_PROPERTY_TYPE(ColorArray, "array:Color", true, false);

class TweakDBPropertySchema
{
public:
    class Builder
    {
    public:
        Builder(std::string name, const TweakDBPropertyType& type)
            : name(std::move(name))
            , type(type)
        {
        }

        [[nodiscard]] Core::SharedPtr<TweakDBPropertySchema> Build() const
        {
            return Core::SharedPtr<TweakDBPropertySchema>(new TweakDBPropertySchema(name, type));
        }

    private:
        const std::string name;
        const TweakDBPropertyType& type;
    };

    [[nodiscard]] const CName& GetName() const
    {
        return name;
    }

    [[nodiscard]] const CName& GetFlatSuffix() const
    {
        return flatSuffix;
    }

    [[nodiscard]] const CName& GetFunctionName() const
    {
        return functionName;
    }

    [[nodiscard]] const TweakDBPropertyType& GetType() const
    {
        return type;
    }

private:
    TweakDBPropertySchema(const std::string& aName, const TweakDBPropertyType& aType)
        : name(aName | ToCName)
        , flatSuffix("." + aName | ToCName)
        , functionName(aName | Capitalize | ToCName)
        , type(aType)
    {
    }

    const CName name;
    const CName flatSuffix;
    const CName functionName;
    const TweakDBPropertyType& type;
};

class TweakDBRecordSchema
{
public:
    class Builder
    {
    public:
        explicit Builder(const CName& name)
            : Builder(std::string(name.ToString()))
        {
        }

        explicit Builder(const std::string& name)
            : Builder(name, game::data::TweakDBRecord::NAME)
        {
        }

        Builder(std::string name, std::string parent)
            : name(std::move(name))
            , parent(std::move(parent))
        {
        }

        Builder& SchemaType(const ESchemaType aType)
        {
            this->type = aType;
            return *this;
        }

        Builder& Parent(const std::string& aParent)
        {
            this->parent = aParent;
            return *this;
        }

        Builder& Property(const std::string& aName, const TweakDBPropertyType& aType)
        {
            const TweakDBPropertySchema::Builder propBuilder(aName, aType);
            properties.push_back(propBuilder.Build());
            return *this;
        }

        [[nodiscard]] Core::SharedPtr<TweakDBRecordSchema> Build() const
        {
            return Core::SharedPtr<TweakDBRecordSchema>(new TweakDBRecordSchema(name, parent, type, properties));
        }

    private:
        std::string name;
        std::string parent;
        ESchemaType type = ESchemaType::CUSTOM;
        Core::Vector<Core::SharedPtr<TweakDBPropertySchema>> properties;
    };

    [[nodiscard]] const CName& GetFullName() const
    {
        return fullName;
    }

    [[nodiscard]] const CName& GetAliasName() const
    {
        return aliasName;
    }

    [[nodiscard]] const CName& GetShortName() const
    {
        return shortName;
    }

    [[nodiscard]] const CName& GetParent() const
    {
        return parent;
    }

    [[nodiscard]] uint32_t GetTypeHash() const
    {
        return typeHash;
    }

    [[nodiscard]] ESchemaType GetSchemaType() const
    {
        return schemaType;
    }

    [[nodiscard]] const Core::Map<CName, Core::SharedPtr<TweakDBPropertySchema>>& GetProperties() const
    {
        return properties;
    }

private:
    TweakDBRecordSchema(const std::string& name, const std::string& parent, const ESchemaType& type,
                        const Core::Vector<Core::SharedPtr<TweakDBPropertySchema>>& properties)
        : fullName(TweakDBReflection::GetRecordFullName(name.c_str()) | ToCName)
        , aliasName(TweakDBReflection::GetRecordAliasName(name.c_str()) | ToCName)
        , shortName(TweakDBReflection::GetRecordShortName(name.c_str()) | ToCName)
        , parent(TweakDBReflection::GetRecordFullName(parent.c_str()) | ToCName)
        , typeHash(TweakDBReflection::GetRecordTypeHash(shortName))
        , schemaType(type)
        , properties(ToMap(
              properties, [](const Core::SharedPtr<TweakDBPropertySchema>& p) -> const CName& { return p->GetName(); }))
    {
    }

    const CName fullName;
    const CName aliasName;
    const CName shortName;
    const CName parent;
    const uint32_t typeHash;
    const ESchemaType schemaType;
    const Core::Map<CName, Core::SharedPtr<TweakDBPropertySchema>> properties;
};

struct TweakRecordSchemaRegistrar
{
    using Callback = std::function<Core::SharedPtr<TweakDBRecordSchema>()>;

    explicit TweakRecordSchemaRegistrar(const Callback& aCallback)
    {
        s_callbacks.push_back(aCallback);
    }

    static Core::UniquePtr<Core::Set<Core::SharedPtr<TweakDBRecordSchema>>> LoadSchemas()
    {
        auto result = Core::MakeUnique<Core::Set<Core::SharedPtr<TweakDBRecordSchema>>>();

        for (const auto& cb : s_callbacks)
        {
            if (auto schema = cb())
            {
                result->insert(schema);
            }
        }

        return result;
    }

    [[maybe_unused]] static inline std::vector<Callback> s_callbacks;
};

template<Scope>
struct TweakDBRecordBuilder;

template<typename T>
concept IsTweakDBRecord = std::is_base_of_v<game::data::TweakDBRecord, T>;

template<typename TClass>
    requires IsTweakDBRecord<TClass>
struct TweakDBRecordSchemaDefinition
{
    using Configurer = TweakDBRecordBuilder<Scope::For<TClass>()>;

    static Core::SharedPtr<TweakDBRecordSchema> RegisterSchema()
    {
        auto builder = TweakDBRecordSchema::Builder(ResolveTypeName<TClass>());
        builder.SchemaType(ESchemaType::NATIVE);
        Configurer::Configure(builder);
        return builder.Build();
    }

    inline static TweakRecordSchemaRegistrar s_registrar{&RegisterSchema};

    constexpr operator Scope() const noexcept
    {
        return Scope::For<TClass>();
    }
};

} // namespace Red::Schema

#define TWEAKDB_DEFINE_RECORD(_type, _cust)                                                                            \
    template<>                                                                                                         \
    struct Red::Schema::TweakDBRecordBuilder<Red::Schema::TweakDBRecordSchemaDefinition<_type>{}>                      \
    {                                                                                                                  \
        static void Configure(Red::Schema::TweakDBRecordSchema::Builder& builder) _cust;                               \
    }

#define TWEAKDB_RECORD_PARENT(_parent) builder.Parent(_parent)
#define TWEAKDB_PROPERTY(_name, _type) builder.Property(_name, Red::Schema::TweakDBPropertyType::_type);

TWEAKDB_DEFINE_RECORD(RED4ext::game::data::UIIcon_Record, {
    TWEAKDB_RECORD_PARENT("gamedataTweakDBRecord");
    TWEAKDB_PROPERTY("atlasResourcePath", ResRef);
    TWEAKDB_PROPERTY("atlasPartName", CName);
});
