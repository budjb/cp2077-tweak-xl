#include "CustomRecordService.hpp"

#include "App/Environment.hpp"
#include "Core/Facades/Container.hpp"
#include "CustomTweakRecord.hpp"
#include "Red/TweakDB/Raws.hpp"

#include <spdlog/fmt/bundled/base.h>

namespace App::Record
{

void CustomRecordService::AddSchema(CustomRecordSchema&& schema)
{
    auto ptr = Core::MakeShared<CustomRecordSchema>(std::move(schema));
    recordSchemasByHash.Emplace(ptr->GetTweakBaseHash(), ptr);
    recordSchemasByName.Emplace(ptr->GetFullName(), ptr);
}

const CustomRecordSchema* CustomRecordService::GetSchema(const Red::CName& name) const
{
    const auto schema = recordSchemasByName.Get(name);
    return (schema && *schema) ? schema->get() : nullptr;
}

const CustomRecordSchema* CustomRecordService::GetSchema(const uint32_t hash) const
{
    const auto schema = recordSchemasByHash.Get(hash);
    return (schema && *schema) ? schema->get() : nullptr;
}

const CustomRecordProperty* CustomRecordService::GetProperty(Red::gamedataTweakDBRecord& record,
                                                             const Red::CName& functionName) const
{
    auto* schema = GetSchema(record.GetTweakBaseHash());
    if (!schema)
    {
        LogError("Cannot get the property value {} because the record {} is not registered.", functionName.ToString(),
                 record.GetNativeType()->GetName().ToString());
        return nullptr;
    }

    const auto* property = schema->GetProperty(functionName);

    if (!property)
    {
        LogError("Cannot get the property value {} because it is not registered for record {}.",
                 functionName.ToString(), record.GetNativeType()->GetName().ToString());
        return nullptr;
    }

    return property;
}

void CustomRecordService::RegisterRTTIClasses() const
{
    recordSchemasByHash.ForEach([](const auto&, auto& schema) { CustomRecordService::RegisterRTTIClass(*schema); });
}

void CustomRecordService::DescribeRTTIClasses() const
{
    recordSchemasByHash.ForEach([](const auto&, auto& schema) { CustomRecordService::DescribeRTTIClass(*schema); });
}

void CustomRecordService::RegisterRTTIClass(const CustomRecordSchema& schema)
{
    auto* rtti = Red::CRTTISystem::Get();

    auto* baseRecordType = rtti->GetClass(CustomTweakRecord::NAME);

    if (!baseRecordType)
    {
        LogError("Unable to create custom Tweak record {} because base custom TweakDB record class {} not found",
                 schema.GetFullName().ToString(), CustomTweakRecord::NAME);
    }

    constexpr Red::CClass::Flags flags{.isScriptedClass = true};

    rtti->CreateScriptedClass(schema.GetFullName(), flags, baseRecordType);
}

void CustomRecordService::DescribeRTTIClass(const CustomRecordSchema& schema)
{
    auto* rtti = Red::CRTTISystem::Get();

    auto* recordType = rtti->GetClass(schema.GetFullName());

    if (!recordType)
    {
        LogError("Failed to create custom record type {}.", schema.GetFullName().ToString());
    }

    rtti->RegisterScriptName(schema.GetFullName(), schema.GetAliasName());

    schema.GetProperties().ForEach([recordType](const Red::CName&, const CustomRecordProperty& property) {
        RegisterRTTIProperty(recordType, property);
    });
}

void CustomRecordService::RegisterRTTIProperty(Red::CClass* recordType, const CustomRecordProperty& property)
{
    const auto functionName = property.GetFunctionName();

    constexpr Red::CClassFunction::Flags functionFlags{.isNative = true, .isPublic = true};

    const auto* func = Red::CClassFunction::Create(recordType, functionName.ToString(), functionName.ToString(),
                                                   &GetPropertyValue, functionFlags);

    if (!func)
    {
        LogError("Failed to create function {} for custom record type {}.", functionName.ToString(),
                 recordType->GetName().ToString());
    }
}

void CustomRecordService::CreateBaseTweakRecord(const CustomRecordSchema& schema)
{
    if (const auto record = CreateInstance(schema, baseRecordPrefix + schema.GetFullName()))
    {
        Raw::InsertRecord(Red::TweakDB::Get(), record->recordID, record->GetNativeType(), record);
    }
}

void CustomRecordService::CreateBaseTweakRecords() const
{
    recordSchemasByHash.ForEach(
        [](const auto&, const auto& schema) { CustomRecordService::CreateBaseTweakRecord(*schema); });
}

Red::Handle<CustomTweakRecord> CustomRecordService::CreateInstance(const CustomRecordSchema& schema,
                                                                   const Red::TweakDBID& tweakDBID)
{
    const auto* type = schema.GetType();
    auto* record = static_cast<CustomTweakRecord*>(type->CreateInstance());

    if (!record)
    {
        LogError("Failed to create an instance of custom record type {}.", schema.GetFullName().ToString());
        return {};
    }

    record->recordID = tweakDBID;

    return Red::Handle(record);
}

void CustomRecordService::OnBootstrap()
{
    HookWrap<Raw::CreateRecord>(
        [this](const CreateRecordFunc original, Red::TweakDB* tweakDB, const uint32_t hash, Red::TweakDBID tweakDBID) {
            if (const auto* schema = this->GetSchema(hash))
            {
                if (const auto record = CreateInstance(*schema, tweakDBID))
                {
                    Raw::InsertRecord(tweakDB, tweakDBID, record->GetNativeType(), record);
                }
                else
                {
                    LogError("Failed to create an instance of custom record {}.", schema->GetFullName().ToString());
                }
            }
            else
            {
                original(tweakDB, hash, tweakDBID);
            }
        });

    auto* rtti = Red::CRTTISystem::Get();
    rtti->AddRegisterCallback([this]() { this->RegisterRTTIClasses(); });
    rtti->AddPostRegisterCallback([this]() { this->DescribeRTTIClasses(); });

    auto schema = CustomRecordSchema(Red::CNamePool::Add("CustomTweakExample"));

    assert(schema.GetAliasName() == "CustomTweakExample_Record" && "Alias should be CustomTweakExample_Record");
    assert(schema.GetFullName() == "gamedataCustomRecordExample_Record" &&
           "Full name should be gamedataCustomRecordExample_Record");
    assert(schema.GetShortName() == "CustomRecordExample" && "Short name should be CustomRecordExample");

    const auto fooProp = CustomRecordProperty(Red::CNamePool::Add("foo"), rtti->GetClass("CName"));
    assert(fooProp.GetName() == "foo" && "Property name should be foo");
    assert(fooProp.GetFunctionName() == "Foo" && "Function name should be Foo");
    assert(fooProp.GetFlatSuffix() == ".foo" && "Flat suffix should be .foo");
    assert(fooProp.GetType() == rtti->GetClass("CName") && "Property type should be CName");

    schema.AddProperty(fooProp);
    AddSchema(std::move(schema));
}

void GetPropertyValue(Red::IScriptable* instance, Red::CStackFrame* frame, void* out, int64_t)
{
    frame->code++;

    if (!out)
    {
        return;
    }

    auto& record =
        static_cast<Red::gamedataTweakDBRecord&>(*instance); // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)

    auto* property = Core::Resolve<CustomRecordService>()->GetProperty(record, frame->func->fullName);

    if (!property)
    {
        return;
    }

    if (const auto value = property->GetValue(record.recordID))
    {
        property->GetType()->Assign(out, value);
    }
}

} // namespace App::Record
