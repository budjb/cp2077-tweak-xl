#pragma once

namespace Red
{

class TweakDBReflection;

class TweakDBPropertyInfo
{
public:
    TweakDBPropertyInfo() noexcept = default;

    void SetName(const char* aName);
    void SetName(std::string aName);
    void SetType(const CBaseRTTIType* aType);
    void SetElementType(const CBaseRTTIType* aElementType);
    void SetForeignType(const CClass* aForeignType);
    void SetArray(bool aIsArray = true);
    void SetForeignKey(bool aIsForeignKey = true);
    void SetDataOffset(uintptr_t aDataOffset);
    void SetDefaultValue(int32_t aDefaultValue);

    [[nodiscard]] CName GetName() const;
    [[nodiscard]] const CBaseRTTIType* GetType() const;
    [[nodiscard]] const CBaseRTTIType* GetElementType() const;
    [[nodiscard]] const CClass* GetForeignType() const;
    [[nodiscard]] bool IsArray() const;
    [[nodiscard]] bool IsForeignKey() const;
    [[nodiscard]] std::string GetAppendix() const;
    [[nodiscard]] uintptr_t GetDataOffset() const;
    [[nodiscard]] int32_t GetDefaultValue() const;

    [[nodiscard]] bool IsValid() const;

private:
    CName m_name;
    const CBaseRTTIType* m_type;
    const CBaseRTTIType* m_elementType;
    const CClass* m_foreignType;
    bool m_isArray;
    bool m_isForeignKey;
    std::string m_appendix; // The name used to build ID of the property
    uintptr_t m_dataOffset; // Offset of the property in record instance
    int32_t m_defaultValue; // Offset of the default value in the buffer
};

class TweakDBRecordInfo
{
public:
    TweakDBRecordInfo() noexcept = default;

    void SetName(const char* aName);
    void SetName(CName aName);
    void SetType(const CClass* aType);
    void SetParent(const CClass* aParent);

    Core::SharedPtr<const TweakDBPropertyInfo> AddProperty(Core::SharedPtr<TweakDBPropertyInfo> aProperty);

    [[nodiscard]] CName GetName() const;
    [[nodiscard]] CName GetAliasName() const;
    [[nodiscard]] std::string GetShortName() const;
    [[nodiscard]] const CClass* GetType() const;
    [[nodiscard]] const CClass* GetParent() const;
    [[nodiscard]] uint32_t GetTypeHash() const;

    [[nodiscard]] Core::SharedPtr<const TweakDBPropertyInfo> GetProperty(CName aPropName) const;
    [[nodiscard]] const Core::Map<CName, Core::SharedPtr<const TweakDBPropertyInfo>>& GetProperties() const;

    [[nodiscard]] bool IsValid() const;

    TweakDBRecordInfo& operator+=(const TweakDBRecordInfo* aOther);
    TweakDBRecordInfo& operator+=(const TweakDBRecordInfo& aOther);

private:
    CName m_name{};
    CName m_aliasName{};
    std::string m_shortName{};
    const CClass* m_type = nullptr;
    const CClass* m_parent = nullptr;
    uint32_t m_typeHash{};

    Core::Map<CName, Core::SharedPtr<const TweakDBPropertyInfo>> m_props{};
};

} // namespace Red
