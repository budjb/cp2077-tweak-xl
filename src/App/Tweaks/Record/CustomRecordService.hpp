#pragma once

#include "Core/Foundation/Feature.hpp"
#include "Core/Hooking/HookingAgent.hpp"
#include "Core/Logging/LoggingAgent.hpp"
#include "CustomRecordSchema.hpp"
#include "CustomTweakRecord.hpp"
#include "Red/TweakDB/Raws.hpp"

namespace App::Record
{

using CreateRecordFunc = std::remove_cvref_t<decltype(Raw::CreateRecord)>::Callable;

class CustomRecordService
    : public Core::LoggingAgent
    , public Core::Feature
    , public Core::HookingAgent
{
public:
    constexpr static auto baseRecordPrefix = Red::TweakDBID("tweakxl.custom_records.");

    CustomRecordService() = default;

    void AddSchema(CustomRecordSchema&& schema);

    [[nodiscard]] const CustomRecordSchema* GetSchema(const Red::CName& name) const;
    [[nodiscard]] const CustomRecordSchema* GetSchema(uint32_t hash) const;

    const CustomRecordProperty* GetProperty(Red::gamedataTweakDBRecord& record, const Red::CName& functionName) const;

    void RegisterRTTIClasses() const;
    void DescribeRTTIClasses() const;
    void CreateBaseTweakRecords() const;

    [[nodiscard]] static Red::Handle<CustomTweakRecord> CreateInstance(const CustomRecordSchema& schema,
                                                                       const Red::TweakDBID& tweakDBID);

protected:
    void OnBootstrap() override;

private:
    static void RegisterRTTIClass(const CustomRecordSchema& schema);
    static void DescribeRTTIClass(const CustomRecordSchema& schema);
    static void RegisterRTTIProperty(Red::CClass* recordType, const CustomRecordProperty& property);
    static void CreateBaseTweakRecord(const CustomRecordSchema& schema);

    Red::HashMap<Red::CName, Core::SharedPtr<CustomRecordSchema>> recordSchemasByName{};
    Red::HashMap<uint32_t, Core::SharedPtr<CustomRecordSchema>> recordSchemasByHash{};
};

void GetPropertyValue(Red::IScriptable*, Red::CStackFrame*, void*, int64_t);

} // namespace App::Record
