#pragma once

#include "App/Utils/Str.hpp"
#include "Red/Localization.hpp"
#include "Red/TweakDB/Reflection.hpp"

namespace App
{
inline void ConvertScriptValueForFlatValue(Red::Variant& aVariant)
{
    if (const auto& variantType = aVariant.GetType(); Red::TweakDBReflection::IsResRefToken(variantType))
    {
        const auto rtti = Red::CRTTISystem::Get();
        const auto type = rtti->GetType(Red::TDBFlatType::ResRef);

        aVariant = Red::Variant(type, aVariant.GetDataPtr());
    }
    else if (Red::TweakDBReflection::IsResRefTokenArray(variantType))
    {
        const auto rtti = Red::CRTTISystem::Get();
        const auto type = rtti->GetType(Red::TDBFlatType::ResRefArray);

        aVariant = Red::Variant(type, aVariant.GetDataPtr());
    }
    else if (variantType->GetName() == Red::TDBFlatType::CName)
    {
        if (const auto str = static_cast<Red::CName*>(aVariant.GetDataPtr())->ToString();
            strncmp(str, Red::LocKeyPrefix, Red::LocKeyPrefixLength) == 0)
        {
            const auto& value = str + Red::LocKeyPrefixLength;
            const auto& length = strlen(str) - Red::LocKeyPrefixLength;
            Red::LocKeyWrapper wrapper;

            if (!ParseInt(value, length, wrapper.primaryKey))
            {
                wrapper.primaryKey = Red::FNV1a64(value);
            }

            aVariant.Fill(Red::TweakDBReflection::GetFlatType(Red::TDBFlatType::LocKey), &wrapper);
        }
    }
    else if (variantType->GetName() == Red::TDBFlatType::String)
    {
        const auto str = static_cast<Red::CString*>(aVariant.GetDataPtr());

        if (strncmp(str->c_str(), Red::LocKeyPrefix, Red::LocKeyPrefixLength) == 0)
        {
            const std::string_view value{str->c_str() + Red::LocKeyPrefixLength,
                                         str->Length() - Red::LocKeyPrefixLength};
            if (!IsNumeric(value))
            {
                const std::string key = Red::LocKeyPrefix + std::to_string(Red::FNV1a64(value.data()));
                *str = key.data();
            }
        }
    }
}
} // namespace App
