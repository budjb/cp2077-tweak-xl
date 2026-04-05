#include "CustomRecordSchema.hpp"

#include "Core/Facades/Container.hpp"
#include "Red/TweakDB/Manager.hpp"
#include "Red/TweakDB/Reflection.hpp"

namespace App::Record
{

using namespace App;

static std::string Capitalize(const std::string& value)
{
    auto result = value;

    if (!result.empty())
    {
        result[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(result[0])));
    }

    return result;
}

#pragma region CustomRecordProperty

CustomRecordProperty::CustomRecordProperty(const Red::CName& name, Red::CClass* type, void* defaultValue)
    : name(name)
    , functionName(Red::CNamePool::Add(Capitalize(name.ToString()).c_str()))
    , flatSuffix(std::string(".") + name.ToString())
{
    assert(type && "CustomRecordProperty type cannot be null");
    this->type = type;
}

Red::CName CustomRecordProperty::GetName() const
{
    return name;
}

Red::CName CustomRecordProperty::GetFunctionName() const
{
    return functionName;
}

std::string_view CustomRecordProperty::GetFlatSuffix() const
{
    return flatSuffix;
}

Red::CClass* CustomRecordProperty::GetType() const
{
    return type;
}

Red::TweakDBID CustomRecordProperty::GetTweakDBID(const Red::TweakDBID& id) const
{
    return id + flatSuffix;
}

Red::Value<> CustomRecordProperty::GetValue(const Red::TweakDBID& base) const
{
    if (const auto value = Core::Resolve<Red::TweakDBManager>()->GetFlat(GetTweakDBID(base)))
    {
        if (value.type == type)
        {
            return value;
        }
    }

    return {};
}

#pragma endregion

#pragma region CustomRecordSchema

CustomRecordSchema::CustomRecordSchema(const Red::CName& name)
{
    const Core::SharedPtr<Red::TweakDBReflection> reflection = Core::MakeShared<Red::TweakDBReflection>();

    this->fullName = Red::CNamePool::Add(reflection->GetRecordFullName(name).c_str());
    this->shortName = Red::CNamePool::Add(reflection->GetRecordShortName(name).c_str());
    this->aliasName = Red::CNamePool::Add(reflection->GetRecordAliasName(name).c_str());
}

const Red::HashMap<Red::CName, CustomRecordProperty>& CustomRecordSchema::GetProperties() const
{
    return properties;
}

Red::CClass* CustomRecordSchema::GetType() const
{
    return Red::CRTTISystem::Get()->GetClass(fullName);
}

Red::CName CustomRecordSchema::GetAliasName() const
{
    return aliasName;
}

Red::CName CustomRecordSchema::GetFullName() const
{
    return fullName;
}

Red::CName CustomRecordSchema::GetShortName() const
{
    return shortName;
}

void CustomRecordSchema::AddProperty(const CustomRecordProperty& property)
{
    properties.Emplace(property.GetName(), property);
}

const CustomRecordProperty* CustomRecordSchema::GetProperty(const Red::CName& name) const
{
    const auto* property = properties.Get(name);
    if (!property)
    {
        return nullptr;
    }

    return property;
}

uint32_t CustomRecordSchema::GetTweakBaseHash() const
{
    return Red::Murmur3_32(aliasName.ToString());
}

#pragma endregion

} // namespace App::Record
