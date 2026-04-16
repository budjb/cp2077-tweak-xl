#pragma once
#include "Alias.hpp"

namespace Red
{

class DeferredType
{
public:
    DeferredType() noexcept = default;
    virtual ~DeferredType() = default;

    DeferredType(const char* aName) noexcept
    {
        if (aName)
        {
            m_hash = CNamePool::Add(aName);
        }
    }

    DeferredType(const std::string& aName) noexcept
    {
        if (!aName.empty())
        {
            m_hash = CNamePool::Add(aName.c_str());
        }
    }

    DeferredType(CName aName) noexcept
    {
        m_hash = aName;
    }

    DeferredType(uint64_t hash) noexcept
    {
        m_hash = hash;
    }

    DeferredType(Red::rtti::IType* type) noexcept
    {
        if (type)
        {
            m_hash = type->GetName();
            m_type = type;
        }
    }

    DeferredType(const Red::rtti::IType* type) noexcept
    {
        if (type)
        {
            m_hash = type->GetName();
            m_type = const_cast<Red::rtti::IType*>(type);
        }
    }

    bool IsResolved() const
    {
        return m_type != nullptr;
    }

    bool TryResolve() const
    {
        if (m_type)
        {
            return true;
        }

        if (auto* type = CRTTISystem::Get()->GetType(m_hash))
            m_type = type;

        return m_type != nullptr;
    }

    const Red::rtti::IType* GetType() const
    {
        if (TryResolve())
            return m_type;
        return nullptr;
    }

    const Red::CClass* GetClass() const
    {
        if (const auto* type = GetType())
        {
            if (type->GetType() == Red::rtti::ERTTIType::Class)
            {
                return static_cast<const Red::CClass*>(type);
            }
        }
        return nullptr;
    }

    uint64_t GetHash() const
    {
        return m_hash;
    }

    CName GetName() const
    {
        return m_hash;
    }

    operator bool() const
    {
        return m_hash != 0;
    }

    operator uint64_t() const
    {
        return GetHash();
    }

    operator CName() const
    {
        return GetHash();
    }

    operator const Red::rtti::IType*() const
    {
        return GetType();
    }

private:
    uint64_t m_hash = 0;
    mutable Red::rtti::IType* m_type = nullptr;
};

class TweakDBReflection;

class TweakDBPropertyInfo
{
public:
    TweakDBPropertyInfo() noexcept = default;

    void SetName(const char* aName);
    void SetName(const std::string& aName);
    void SetType(const DeferredType& aType);
    void SetElementType(const DeferredType& aElementType);
    void SetForeignType(const DeferredType& aType);
    void SetArray(bool aArray = true);
    void SetForeignKey(bool aForeignKey = true);
    void SetDataOffset(uintptr_t aDataOffset);
    void SetDataOffset(std::optional<uintptr_t> aDataOffset);
    void SetDefaultValue(int32_t aDefaultValue);
    void SetDefaultValue(std::optional<int32_t> aDefaultValue);

    [[nodiscard]] CName GetName() const;
    [[nodiscard]] CName GetFunctionName() const;
    [[nodiscard]] DeferredType GetType() const;
    [[nodiscard]] DeferredType GetElementType() const;
    [[nodiscard]] DeferredType GetForeignType() const;
    [[nodiscard]] bool IsArray() const;
    [[nodiscard]] bool IsForeignKey() const;
    [[nodiscard]] const std::string& GetAppendix() const;
    [[nodiscard]] std::optional<uintptr_t> GetDataOffset() const;
    [[nodiscard]] std::optional<int32_t> GetDefaultValue() const;

    [[nodiscard]] bool IsValid() const;

private:
    CName m_name;
    CName m_functionName;
    std::string m_appendix; // The name used to build ID of the property
    DeferredType m_type;
    DeferredType m_elementType;
    DeferredType m_foreignType;
    bool m_isArray{false};
    bool m_isForeignKey{false};
    std::optional<uintptr_t> m_dataOffset; // Offset of the property in record instance
    std::optional<int32_t> m_defaultValue; // Offset of the default value in the buffer
};

class TweakDBRecordInfo
{
public:
    TweakDBRecordInfo() noexcept = default;

    void SetName(const std::string& aName);
    void SetType(const DeferredType& aType);
    void SetParent(const DeferredType& aType);
    void SetCustom(bool aCustom = true);

    Core::SharedPtr<const TweakDBPropertyInfo> AddProperty(Core::SharedPtr<TweakDBPropertyInfo> aProperty);

    [[nodiscard]] CName GetAliasName() const;
    [[nodiscard]] const std::string& GetShortName() const;
    [[nodiscard]] DeferredType GetType() const;
    [[nodiscard]] DeferredType GetParent() const;
    [[nodiscard]] TweakDBRecordHash GetTypeHash() const;
    [[nodiscard]] bool IsCustom() const;

    [[nodiscard]] Core::SharedPtr<const TweakDBPropertyInfo> GetProperty(CName aPropName) const;
    [[nodiscard]] const Core::Map<CName, Core::SharedPtr<const TweakDBPropertyInfo>>& GetProperties() const;

    [[nodiscard]] bool IsValid() const;

    TweakDBRecordInfo& operator+=(const TweakDBRecordInfo* aOther);
    TweakDBRecordInfo& operator+=(const TweakDBRecordInfo& aOther);

private:
    CName m_aliasName;
    std::string m_shortName;
    DeferredType m_type{};
    DeferredType m_parent{};
    TweakDBRecordHash m_typeHash{};
    bool m_isCustom{false};

    Core::Map<CName, Core::SharedPtr<const TweakDBPropertyInfo>> m_props;
};

} // namespace Red
