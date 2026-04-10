#pragma once

namespace Red
{

class TweakDBReflection;

class TweakDBPropertyInfo
{
public:
    TweakDBPropertyInfo() = default;
    TweakDBPropertyInfo(const TweakDBPropertyInfo&) = default;
    TweakDBPropertyInfo(TweakDBPropertyInfo&&) = default;

    void SetName(const char* aName);
    void SetName(const CName& aName);
    void SetType(const CBaseRTTIType* aType);
    void SetElementType(const CBaseRTTIType* aElementType);
    void SetForeignType(const CClass* aForeignType);
    void SetArray(bool aIsArray = true);
    void SetForeignKey(bool aIsForeignKey = true);
    void SetAppendix(const std::string& aAppendix);
    void SetDataOffset(uintptr_t aDataOffset);
    void SetDefaultValue(int32_t aDefaultValue);

    [[nodiscard]] const CName& GetName() const;
    [[nodiscard]] const CBaseRTTIType* GetType() const;
    [[nodiscard]] const CBaseRTTIType* GetElementType() const;
    [[nodiscard]] const CClass* GetForeignType() const;
    [[nodiscard]] bool IsArray() const;
    [[nodiscard]] bool IsForeignKey() const;
    [[nodiscard]] const std::string& GetAppendix() const;
    [[nodiscard]] uintptr_t GetDataOffset() const;
    [[nodiscard]] int32_t GetDefaultValue() const;

    [[nodiscard]] bool Finalize();
    [[nodiscard]] bool IsFinalized() const;
    [[nodiscard]] bool IsValid() const;

private:
    CName m_name{};
    const CBaseRTTIType* m_type = nullptr;
    const CBaseRTTIType* m_elementType = nullptr;
    const CClass* m_foreignType = nullptr;
    bool m_isArray = false;
    bool m_isForeignKey = false;
    std::string m_appendix{}; // The name used to build ID of the property
    uintptr_t m_dataOffset{}; // Offset of the property in record instance
    int32_t m_defaultValue{}; // Offset of the default value in the buffer
    bool m_finalized = false;
};

#define RECORD_INFO_SCALAR_VALID() !m_isArray&& m_isForeignKey

} // namespace Red
