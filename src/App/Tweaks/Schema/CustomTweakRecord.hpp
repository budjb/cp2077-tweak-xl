#pragma once

namespace App::Schema
{

class CustomTweakRecord : public Red::gamedataTweakDBRecord
{
public:
    static constexpr auto NAME = "gamedataCustomTweakRecord";
    static constexpr auto ALIAS = "CustomTweakRecord";

    RED4ext::CClass* GetNativeType() override;

    void sub_108() override
    {
    }

    [[nodiscard]] uint32_t GetTweakBaseHash() const override;
};
RED4EXT_ASSERT_SIZE(CustomTweakRecord, 0x48);

} // namespace App::Schema

RTTI_DEFINE_CLASS(App::Schema::CustomTweakRecord, App::Schema::CustomTweakRecord::NAME, {
    RTTI_ABSTRACT();
    RTTI_PARENT(Red::gamedataTweakDBRecord);
});
