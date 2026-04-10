#include "RedReader.hpp"

Red::CName App::RedReader::GetFlatTypeName(const Core::SharedPtr<Red::TweakFlat>& aFlat)
{
    if (aFlat->isArray)
    {
        switch (aFlat->type)
        {
        case Red::ETweakFlatType::Int:
            return Red::TDBFlatType::IntArray;
        case Red::ETweakFlatType::Float:
            return Red::TDBFlatType::FloatArray;
        case Red::ETweakFlatType::Bool:
            return Red::TDBFlatType::BoolArray;
        case Red::ETweakFlatType::String:
            return Red::TDBFlatType::StringArray;
        case Red::ETweakFlatType::CName:
            return Red::TDBFlatType::CNameArray;
        case Red::ETweakFlatType::ResRef:
            return Red::TDBFlatType::ResRefArray;
        case Red::ETweakFlatType::LocKey:
            return Red::TDBFlatType::LocKeyArray;
        case Red::ETweakFlatType::ForeignKey:
            return Red::TDBFlatType::TweakDBIDArray;
        case Red::ETweakFlatType::Quaternion:
            return Red::TDBFlatType::QuaternionArray;
        case Red::ETweakFlatType::EulerAngles:
            return Red::TDBFlatType::EulerAnglesArray;
        case Red::ETweakFlatType::Vector3:
            return Red::TDBFlatType::Vector3Array;
        case Red::ETweakFlatType::Vector2:
            return Red::TDBFlatType::Vector2Array;
        case Red::ETweakFlatType::Color:
            return Red::TDBFlatType::ColorArray;
        case Red::ETweakFlatType::Undefined:
            break;
        }
    }
    else
    {
        switch (aFlat->type)
        {
        case Red::ETweakFlatType::Int:
            return Red::TDBFlatType::Int;
        case Red::ETweakFlatType::Float:
            return Red::TDBFlatType::Float;
        case Red::ETweakFlatType::Bool:
            return Red::TDBFlatType::Bool;
        case Red::ETweakFlatType::String:
            return Red::TDBFlatType::String;
        case Red::ETweakFlatType::CName:
            return Red::TDBFlatType::CName;
        case Red::ETweakFlatType::ResRef:
            return Red::TDBFlatType::ResRef;
        case Red::ETweakFlatType::LocKey:
            return Red::TDBFlatType::LocKey;
        case Red::ETweakFlatType::ForeignKey:
            return Red::TDBFlatType::TweakDBID;
        case Red::ETweakFlatType::Quaternion:
            return Red::TDBFlatType::Quaternion;
        case Red::ETweakFlatType::EulerAngles:
            return Red::TDBFlatType::EulerAngles;
        case Red::ETweakFlatType::Vector3:
            return Red::TDBFlatType::Vector3;
        case Red::ETweakFlatType::Vector2:
            return Red::TDBFlatType::Vector2;
        case Red::ETweakFlatType::Color:
            return Red::TDBFlatType::Color;
        case Red::ETweakFlatType::Undefined:
            break;
        }
    }

    return {};
}
