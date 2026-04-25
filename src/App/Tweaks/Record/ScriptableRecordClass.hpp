#pragma once

namespace App
{
class ScriptableRecordClass : public Red::CClass
{
public:
    ScriptableRecordClass(RED4ext::CName aName, uint32_t aHash);
    void ConstructCls(void* aMemory) const override;
    void DestructCls(void* aMemory) const override;
    [[nodiscard]] void* AllocMemory() const override;
    const bool IsEqual(const void* aLhs, const void* aRhs, uint32_t a3) override;
    void Assign(void* aLhs, const void* aRhs) const override;
    const uint32_t tweakBaseHash;
};
} // namespace App