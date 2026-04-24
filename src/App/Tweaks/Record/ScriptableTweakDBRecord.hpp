#pragma once

namespace App
{
class RecordClass;
}

namespace Red
{

class RecordClass;

class ScriptableTweakDBRecord : public TweakDBRecord
{
public:
    static constexpr auto NAME = "gamedataScriptableTweakDBRecord";
    static constexpr auto ALIAS = "ScriptableTweakDBRecord";

    using TYPE = ClassLocator<ScriptableTweakDBRecord>;

    ScriptableTweakDBRecord() = default;
    explicit ScriptableTweakDBRecord(const App::RecordClass* aClass);

    void sub_108() override;
    CClass* GetNativeType() override;
    [[nodiscard]] uint32_t GetTweakBaseHash() const override;

    RTTI_MEMBER_ACCESS(Red::ScriptableTweakDBRecord);
    RTTI_IMPL_ALLOCATOR();
};

RED4EXT_ASSERT_SIZE(ScriptableTweakDBRecord, 0x48);

} // namespace Red

RTTI_DEFINE_CLASS(Red::ScriptableTweakDBRecord, Red::ScriptableTweakDBRecord::NAME, {
    // TODO: mark abstract when the CET fix is merged
    RTTI_ALIAS(Red::ScriptableTweakDBRecord::ALIAS);
    RTTI_PARENT(Red::gamedataTweakDBRecord);
});
