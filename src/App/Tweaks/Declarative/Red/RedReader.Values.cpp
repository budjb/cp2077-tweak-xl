#include "RedReader.hpp"
#include "App/Utils/Str.hpp"
#include "Red/Localization.hpp"
#include "Red/TweakDB/Source/Grammar.hpp"

namespace
{
template<typename T>
    requires std::is_integral_v<T>
bool ParseInt(const std::string& aData, T& aResult)
{
    return App::ParseInt(aData, aResult);
}

bool ParseFloat(const std::string& aData, float& aResult)
{
    return App::ParseFloat(aData, aResult, Red::TweakGrammar::Float::Suffix);
}

template<typename T>
Red::InstancePtr<T> ConvertValue(const Red::TweakValuePtr& aValue);

template<>
Red::InstancePtr<int> ConvertValue(const Red::TweakValuePtr& aValue)
{
    if (aValue->type == Red::ETweakValueType::Number)
    {
        const auto& data = aValue->data.front();

        if (int result; ParseInt(data, result))
        {
            return Red::MakeInstance<int>(result);
        }
    }

    return {};
}

template<>
Red::InstancePtr<float> ConvertValue(const Red::TweakValuePtr& aValue)
{
    if (aValue->type == Red::ETweakValueType::Number)
    {
        const auto& data = aValue->data.front();

        if (float result; ParseFloat(data, result))
        {
            return Red::MakeInstance<float>(result);
        }
    }

    return {};
}

template<>
Red::InstancePtr<bool> ConvertValue(const Red::TweakValuePtr& aValue)
{
    if (aValue->type == Red::ETweakValueType::Bool)
    {
        const auto& data = aValue->data.front();

        return Red::MakeInstance<bool>(data == Red::TweakGrammar::Bool::True);
    }

    return {};
}

template<>
Red::InstancePtr<Red::LocKeyWrapper> ConvertValue(const Red::TweakValuePtr& aValue)
{
    if (aValue->type == Red::ETweakValueType::String)
    {
        const auto& data = aValue->data.front();

        if (data.empty())
        {
            return Red::MakeInstance<Red::LocKeyWrapper>();
        }

        if (data.starts_with(Red::LocKeyPrefix))
        {
            const auto key = data.substr(Red::LocKeyPrefixLength);

            if (uint64_t hash; ParseInt(key, hash))
            {
                return Red::MakeInstance<Red::LocKeyWrapper>(hash);
            }

            return Red::MakeInstance<Red::LocKeyWrapper>(key.c_str());
        }
    }

    return {};
}

template<>
Red::InstancePtr<Red::CString> ConvertValue(const Red::TweakValuePtr& aValue)
{
    if (aValue->type == Red::ETweakValueType::String)
    {
        const auto& data = aValue->data.front();

        if (data.empty())
        {
            return Red::MakeInstance<Red::CString>();
        }

        if (const auto locKey = ConvertValue<Red::LocKeyWrapper>(aValue))
        {
            return Red::MakeInstance<Red::CString>(
                std::string(Red::LocKeyPrefix).append(std::to_string(locKey->primaryKey)).c_str());
        }

        return Red::MakeInstance<Red::CString>(data.c_str());
    }

    return {};
}

template<>
Red::InstancePtr<Red::CName> ConvertValue(const Red::TweakValuePtr& aValue)
{
    if (aValue->type == Red::ETweakValueType::String)
    {
        const auto& data = aValue->data.front();

        if (data.empty())
        {
            return Red::MakeInstance<Red::CName>();
        }

        return Red::MakeInstance<Red::CName>(Red::CNamePool::Add(data.c_str()));
    }

    return {};
}

template<>
Red::InstancePtr<Red::ResourceAsyncReference<>> ConvertValue(const Red::TweakValuePtr& aValue)
{
    if (aValue->type == Red::ETweakValueType::String)
    {
        const auto& data = aValue->data.front();

        return Red::MakeInstance<Red::ResourceAsyncReference<>>(data.c_str());
    }

    return {};
}

template<>
Red::InstancePtr<Red::TweakDBID> ConvertValue(const Red::TweakValuePtr& aValue)
{
    if (aValue->type == Red::ETweakValueType::String)
    {
        const auto& data = aValue->data.front();

        if (data.empty())
        {
            return Red::MakeInstance<Red::TweakDBID>();
        }

        return Red::MakeInstance<Red::TweakDBID>(data.c_str());
    }

    return {};
}

template<>
Red::InstancePtr<Red::Quaternion> ConvertValue(const Red::TweakValuePtr& aValue)
{
    if (aValue->type == Red::ETweakValueType::Struct && aValue->data.size() == 4)
    {
        const auto& data = aValue->data;

        if (auto result = Red::MakeInstance<Red::Quaternion>();
            ParseFloat(data[0], result->i) && ParseFloat(data[1], result->j) && ParseFloat(data[2], result->k) &&
            ParseFloat(data[3], result->r))
        {
            return result;
        }
    }

    return {};
}

template<>
Red::InstancePtr<Red::EulerAngles> ConvertValue(const Red::TweakValuePtr& aValue)
{
    if (aValue->type == Red::ETweakValueType::Struct && aValue->data.size() == 3)
    {
        const auto& data = aValue->data;

        if (auto result = Red::MakeInstance<Red::EulerAngles>();
            ParseFloat(data[0], result->Roll) && ParseFloat(data[1], result->Pitch) && ParseFloat(data[2], result->Yaw))
        {
            return result;
        }
    }

    return {};
}

template<>
Red::InstancePtr<Red::Vector3> ConvertValue(const Red::TweakValuePtr& aValue)
{
    if (aValue->type == Red::ETweakValueType::Struct && aValue->data.size() == 3)
    {
        const auto& data = aValue->data;

        if (auto result = Red::MakeInstance<Red::Vector3>();
            ParseFloat(data[0], result->X) && ParseFloat(data[1], result->Y) && ParseFloat(data[2], result->Z))
        {
            return result;
        }
    }

    return {};
}

template<>
Red::InstancePtr<Red::Vector2> ConvertValue(const Red::TweakValuePtr& aValue)
{
    if (aValue->type == Red::ETweakValueType::Struct && aValue->data.size() == 2)
    {
        const auto& data = aValue->data;

        if (auto result = Red::MakeInstance<Red::Vector2>();
            ParseFloat(data[0], result->X) && ParseFloat(data[1], result->Y))
        {
            return result;
        }
    }

    return {};
}

template<>
Red::InstancePtr<Red::Color> ConvertValue(const Red::TweakValuePtr& aValue)
{
    if (aValue->type == Red::ETweakValueType::Struct && aValue->data.size() == 4)
    {
        const auto& data = aValue->data;

        if (auto result = Red::MakeInstance<Red::Color>();
            ParseInt(data[0], result->Red) && ParseInt(data[1], result->Green) && ParseInt(data[2], result->Blue) &&
            ParseInt(data[3], result->Alpha))
        {
            return result;
        }
    }

    return {};
}

template<typename T>
Red::InstancePtr<Red::DynArray<T>> ConvertValue(const Core::Vector<Red::TweakValuePtr>& aValues)
{
    auto array = Red::MakeInstance<Red::DynArray<T>>();

    for (const auto& value : aValues)
    {
        const auto item = ConvertValue<T>(value);

        if (!value)
            return {};

        array->PushBack(*item);
    }

    return array;
}
} // namespace

Red::InstancePtr<> App::RedReader::MakeValue(const FlatStatePtr& aState, const Red::TweakValuePtr& aValue)
{
    switch (const auto& type = aState->isArray ? aState->elementType : aState->resolvedType; type->GetName())
    {
    case Red::TDBFlatType::Int:
        return ConvertValue<int>(aValue);
    case Red::TDBFlatType::Float:
        return ConvertValue<float>(aValue);
    case Red::TDBFlatType::Bool:
        return ConvertValue<bool>(aValue);
    case Red::TDBFlatType::String:
        return ConvertValue<Red::CString>(aValue);
    case Red::TDBFlatType::CName:
        return ConvertValue<Red::CName>(aValue);
    case Red::TDBFlatType::LocKey:
        return ConvertValue<Red::LocKeyWrapper>(aValue);
    case Red::TDBFlatType::ResRef:
        return ConvertValue<Red::ResourceAsyncReference<>>(aValue);
    case Red::TDBFlatType::TweakDBID:
        return ConvertValue<Red::TweakDBID>(aValue);
    case Red::TDBFlatType::Quaternion:
        return ConvertValue<Red::Quaternion>(aValue);
    case Red::TDBFlatType::EulerAngles:
        return ConvertValue<Red::EulerAngles>(aValue);
    case Red::TDBFlatType::Vector3:
        return ConvertValue<Red::Vector3>(aValue);
    case Red::TDBFlatType::Vector2:
        return ConvertValue<Red::Vector2>(aValue);
    case Red::TDBFlatType::Color:
        return ConvertValue<Red::Color>(aValue);
    default:
        return {};
    }
}

Red::InstancePtr<> App::RedReader::MakeValue(const FlatStatePtr& aState,
                                             const Core::Vector<Red::TweakValuePtr>& aValues)
{
    if (!aState->isArray)
    {
        if (aValues.size() != 1)
            return {};

        return MakeValue(aState, aValues.front());
    }

    switch (aState->resolvedType->GetName())
    {
    case Red::TDBFlatType::IntArray:
        return ConvertValue<int>(aValues);
    case Red::TDBFlatType::FloatArray:
        return ConvertValue<float>(aValues);
    case Red::TDBFlatType::BoolArray:
        return ConvertValue<bool>(aValues);
    case Red::TDBFlatType::StringArray:
        return ConvertValue<Red::CString>(aValues);
    case Red::TDBFlatType::CNameArray:
        return ConvertValue<Red::CName>(aValues);
    case Red::TDBFlatType::LocKeyArray:
        return ConvertValue<Red::LocKeyWrapper>(aValues);
    case Red::TDBFlatType::ResRefArray:
        return ConvertValue<Red::ResourceAsyncReference<>>(aValues);
    case Red::TDBFlatType::TweakDBIDArray:
        return ConvertValue<Red::TweakDBID>(aValues);
    case Red::TDBFlatType::QuaternionArray:
        return ConvertValue<Red::Quaternion>(aValues);
    case Red::TDBFlatType::EulerAnglesArray:
        return ConvertValue<Red::EulerAngles>(aValues);
    case Red::TDBFlatType::Vector3Array:
        return ConvertValue<Red::Vector3>(aValues);
    case Red::TDBFlatType::Vector2Array:
        return ConvertValue<Red::Vector2>(aValues);
    case Red::TDBFlatType::ColorArray:
        return ConvertValue<Red::Color>(aValues);
    default:
        return {};
    }
}
