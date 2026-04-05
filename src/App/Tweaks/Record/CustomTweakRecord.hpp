#pragma once

namespace App::Record
{

class CustomTweakRecord : public Red::gamedataTweakDBRecord
{
public:
    static constexpr auto NAME = "gamedataCustomTweakRecord";

    RED4ext::CClass* GetNativeType() override;

    void sub_108() override
    {
    }

    [[nodiscard]] uint32_t GetTweakBaseHash() const override;
};
RED4EXT_ASSERT_SIZE(CustomTweakRecord, 0x48);

} // namespace App::Record

RTTI_DEFINE_CLASS(App::Record::CustomTweakRecord, App::Record::CustomTweakRecord::NAME, {
    RTTI_ABSTRACT();
    RTTI_PARENT(Red::gamedataTweakDBRecord);
});
