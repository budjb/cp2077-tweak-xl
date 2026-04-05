#pragma once

namespace App::Record
{

class CustomRecordProperty
{
public:
    CustomRecordProperty(const Red::CName& name, Red::CClass* type, void* defaultValue = nullptr);
    ~CustomRecordProperty() = default;

    [[nodiscard]] Red::CName GetName() const;
    [[nodiscard]] Red::CName GetFunctionName() const;
    [[nodiscard]] std::string_view GetFlatSuffix() const;
    [[nodiscard]] Red::CClass* GetType() const;
    [[nodiscard]] Red::TweakDBID GetTweakDBID(const Red::TweakDBID& id) const;
    [[nodiscard]] Red::Value<> GetValue(const Red::TweakDBID& base) const;

private:
    Red::CName name;
    Red::CName functionName;
    Red::CClass* type;
    std::string flatSuffix;
};

class CustomRecordSchema
{
public:
    explicit CustomRecordSchema(const Red::CName& name);
    CustomRecordSchema(const CustomRecordSchema&) = delete;
    CustomRecordSchema(CustomRecordSchema&&) noexcept = default;
    CustomRecordSchema& operator=(const CustomRecordSchema&) = delete;
    CustomRecordSchema& operator=(CustomRecordSchema&&) noexcept = default;
    ~CustomRecordSchema() = default;

    [[nodiscard]] Red::CName GetAliasName() const;
    [[nodiscard]] Red::CName GetFullName() const;
    [[nodiscard]] Red::CName GetShortName() const;

    [[nodiscard]] Red::CClass* GetType() const;

    void AddProperty(const CustomRecordProperty& property);
    [[nodiscard]] const CustomRecordProperty* GetProperty(const Red::CName& name) const;
    [[nodiscard]] const Red::HashMap<Red::CName, CustomRecordProperty>& GetProperties() const;

    [[nodiscard]] uint32_t GetTweakBaseHash() const;

private:
    Red::CName fullName;
    Red::CName shortName;
    Red::CName aliasName;
    Red::HashMap<Red::CName, CustomRecordProperty> properties;
};

} // namespace App::Record