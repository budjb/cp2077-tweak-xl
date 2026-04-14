#pragma once
#include "Red/TweakDB/RecordInfo.hpp"

namespace App
{

class CustomTweakDBRecord : public Red::gamedataTweakDBRecord
{
public:
    static constexpr auto NAME = "gamedataCustomTweakDBRecord";
    static constexpr auto ALIAS = "CustomTweakDBRecord";
    static constexpr Red::ClassLocator<CustomTweakDBRecord> TYPE;

    CustomTweakDBRecord() = default;
    CustomTweakDBRecord(const CustomTweakDBRecord& aRecord) = default;
    CustomTweakDBRecord(CustomTweakDBRecord&& aRecord) = default;

    CustomTweakDBRecord(const Red::TweakDBRecordInfo& aRecordInfo, Red::TweakDBID aTweakDBID);

    Red::CClass* GetNativeType() override;

    void sub_108() override
    {
    }

    [[nodiscard]] uint32_t GetTweakBaseHash() const override;

private:
    const uint32_t m_tweakBaseHash{};
};

RED4EXT_ASSERT_SIZE(CustomTweakDBRecord, 0x50);

} // namespace App

RTTI_DEFINE_CLASS(App::CustomTweakDBRecord, App::CustomTweakDBRecord::NAME, {
    RTTI_ABSTRACT();
    RTTI_ALIAS(App::CustomTweakDBRecord::ALIAS);
    RTTI_PARENT(Red::gamedataTweakDBRecord);
});
