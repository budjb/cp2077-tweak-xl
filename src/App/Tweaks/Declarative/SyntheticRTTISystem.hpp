#pragma once
#include "../../../../vendor/RED4ext.SDK/include/RED4ext/RTTISystem.hpp"

namespace App
{
class SyntheticRTTISystem : public RED4ext::IRTTISystem
{

};

class SClass : public Red::CClass
{
public:
    SClass(Red::CName aName, uint32_t aSize, Flags aFlags) noexcept;
    SClass(const SClass& aClass) noexcept = delete;
    SClass(SClass&& aClass) noexcept = delete;

    ~SClass() override = default;

    void sub_C0() override;
    uint32_t GetMaxAlignment() const override;
    bool sub_D0() const override;
    void ConstructCls(RED4ext::ScriptInstance aMemory) const;
    void DestructCls(RED4ext::ScriptInstance aMemory) const;
    void* AllocMemory() const;
    RED4ext::CString GetTypeName() const override;
    const bool IsEqual(RED4ext::ScriptInstance aLhs, RED4ext::ScriptInstance aRhs, uint32_t a3) override;
    void Assign(RED4ext::ScriptInstance aLhs, RED4ext::ScriptInstance aRhs) const override;
    void Move(RED4ext::ScriptInstance aLhs, RED4ext::ScriptInstance aRhs) const override;
    bool FromString(RED4ext::ScriptInstance aInstance, const RED4ext::CString& aString) const override;
    bool sub_78() override;
    bool sub_A8() override;
    RED4ext::Memory::IAllocator* GetAllocator() const override;

    std::string shit;
};

}