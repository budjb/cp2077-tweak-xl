#pragma once

namespace App
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

} // namespace App

// RTTI_DEFINE_CLASS(App::CustomTweakRecord, App::CustomTweakRecord::NAME, {
//     RTTI_ABSTRACT();
//     RTTI_ALIAS(App::CustomTweakRecord::ALIAS);
//     RTTI_PARENT(Red::gamedataTweakDBRecord);
// });
