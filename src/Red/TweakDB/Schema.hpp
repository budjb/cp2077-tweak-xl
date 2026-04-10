#pragma once

#include "RED4ext/Scripting/Natives/Generated/game/data/UIIcon_Record.hpp"
#include "Red/TweakDB/Reflection.Schema.hpp"
#include <utility>

namespace Red
{

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

} // namespace Red

#define TWEAKDB_DEFINE_RECORD(_type, _cust)                                                                            \
    template<>                                                                                                         \
    struct Red::TweakDBRecordBuilder<Red::TweakDBRecordSchemaDefinition<_type>{}>                                      \
    {                                                                                                                  \
        static void Configure(Red::TweakDBRecordSchema::Builder& builder) _cust;                                       \
    }

#define TWEAKDB_RECORD_PARENT(_parent) builder.Parent(_parent)
#define TWEAKDB_PROPERTY(_name, _type)                                                                                 \
    {                                                                                                                  \
        auto property = Red::TweakDBPropertySchema::Builder(_name);                                                    \
        property.FlatType(Red::TDBFlatType::_type);                                                                    \
        property.PropertyStorage(Red::PropertyStorage::CLASS);                                                         \
        builder.Property(property.Build());                                                                            \
    }

TWEAKDB_DEFINE_RECORD(RED4ext::game::data::UIIcon_Record, {
    TWEAKDB_RECORD_PARENT("gamedataTweakDBRecord");
    TWEAKDB_PROPERTY("atlasResourcePath", ResRef);
    TWEAKDB_PROPERTY("atlasPartName", CName);
});
