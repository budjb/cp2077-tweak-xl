#include "Alias.hpp"

namespace
{
constexpr auto ResRefTypeName = Red::GetTypeName<Red::RaRef<Red::CResource>>();
constexpr auto ResRefArrayTypeName = Red::GetTypeName<Red::DynArray<Red::RaRef<Red::CResource>>>();

constexpr auto ScriptResRefTypeName = Red::GetTypeName<Red::ResRef>();
constexpr auto ScriptResRefArrayTypeName = Red::GetTypeName<Red::DynArray<Red::ResRef>>();
} // namespace

namespace Red::ERTDBFlatType
{

CBaseRTTIType* GetType(const uint64_t aType)
{
    // clang-format off
    switch (aType)
    {
        case Int: return TypeLocator<Int>::Get();
        case Float: return TypeLocator<Float>::Get();
        case Bool: return TypeLocator<Bool>::Get();
        case String: return TypeLocator<String>::Get();
        case CName: return TypeLocator<CName>::Get();
        case LocKey: return TypeLocator<LocKey>::Get();
        case ResRef: return TypeLocator<ResRef>::Get();
        case TweakDBID: return TypeLocator<TweakDBID>::Get();
        case Quaternion: return TypeLocator<Quaternion>::Get();
        case EulerAngles: return TypeLocator<EulerAngles>::Get();
        case Vector3: return TypeLocator<Vector3>::Get();
        case Vector2: return TypeLocator<Vector2>::Get();
        case Color: return TypeLocator<Color>::Get();
        case IntArray: return TypeLocator<IntArray>::Get();
        case FloatArray: return TypeLocator<FloatArray>::Get();
        case BoolArray: return TypeLocator<BoolArray>::Get();
        case StringArray: return TypeLocator<StringArray>::Get();
        case CNameArray: return TypeLocator<CNameArray>::Get();
        case LocKeyArray: return TypeLocator<LocKeyArray>::Get();
        case ResRefArray: return TypeLocator<ResRefArray>::Get();
        case TweakDBIDArray: return TypeLocator<TweakDBIDArray>::Get();
        case QuaternionArray: return TypeLocator<QuaternionArray>::Get();
        case EulerAnglesArray: return TypeLocator<EulerAnglesArray>::Get();
        case Vector3Array: return TypeLocator<Vector3Array>::Get();
        case Vector2Array: return TypeLocator<Vector2Array>::Get();
        case ColorArray: return TypeLocator<ColorArray>::Get();
        default: return nullptr;
    }
    // clang-format on
}

CBaseRTTIType* GetType(Red::CName aTypeName)
{
    CBaseRTTIType* type = CRTTISystem::Get()->GetType(aTypeName);

    if (!IsFlatType(type))
        return nullptr;

    return type;
}

CBaseRTTIType* GetArrayType(Red::CName aTypeName)
{
    return CRTTISystem::Get()->GetType(GetArrayTypeName(aTypeName));
}

CBaseRTTIType* GetArrayType(const CBaseRTTIType* aType)
{
    return CRTTISystem::Get()->GetType(GetArrayTypeName(aType));
}

CBaseRTTIType* GetElementType(Red::CName aTypeName)
{
    return GetElementType(CRTTISystem::Get()->GetType(aTypeName));
}

CBaseRTTIType* GetElementType(const CBaseRTTIType* aType)
{
    if (!aType || aType->GetType() != ERTTIType::Array)
        return nullptr;

    return reinterpret_cast<const CRTTIBaseArrayType*>(aType)->innerType;
}

bool IsFlatType(Red::CName aTypeName)
{
    switch (aTypeName)
    {
    case Int:
    case Float:
    case Bool:
    case String:
    case CName:
    case LocKey:
    case ResRef:
    case TweakDBID:
    case Quaternion:
    case EulerAngles:
    case Vector3:
    case Vector2:
    case Color:
    case IntArray:
    case FloatArray:
    case BoolArray:
    case StringArray:
    case CNameArray:
    case LocKeyArray:
    case ResRefArray:
    case TweakDBIDArray:
    case QuaternionArray:
    case EulerAnglesArray:
    case Vector3Array:
    case Vector2Array:
    case ColorArray:
        return true;
    default:
        return false;
    }
}

bool IsFlatType(const CBaseRTTIType* aType)
{
    return aType && IsFlatType(aType->GetName());
}

bool IsArrayType(Red::CName aTypeName)
{
    switch (aTypeName)
    {
    case IntArray:
    case FloatArray:
    case BoolArray:
    case StringArray:
    case CNameArray:
    case LocKeyArray:
    case ResRefArray:
    case TweakDBIDArray:
    case QuaternionArray:
    case EulerAnglesArray:
    case Vector3Array:
    case Vector2Array:
    case ColorArray:
        return true;
    default:
        return false;
    }
}

bool IsArrayType(const CBaseRTTIType* aType)
{
    return aType && IsArrayType(aType->GetName());
}

bool IsForeignKey(Red::CName aTypeName)
{
    return aTypeName == TweakDBID;
}

bool IsForeignKey(const CBaseRTTIType* aType)
{
    return aType && IsForeignKey(aType->GetName());
}

bool IsForeignKeyArray(Red::CName aTypeName)
{
    return aTypeName == TweakDBIDArray;
}

bool IsForeignKeyArray(const CBaseRTTIType* aType)
{
    return aType && IsForeignKeyArray(aType->GetName());
}

bool IsResRefToken(Red::CName aTypeName)
{
    return aTypeName == ScriptResRefTypeName || aTypeName == ResRefTypeName;
}

bool IsResRefToken(const CBaseRTTIType* aType)
{
    return aType && IsResRefToken(aType->GetName());
}

bool IsResRefTokenArray(Red::CName aTypeName)
{
    return aTypeName == ScriptResRefArrayTypeName || aTypeName == ResRefArrayTypeName;
}

bool IsResRefTokenArray(const CBaseRTTIType* aType)
{
    return aType && IsResRefTokenArray(aType->GetName());
}

Red::CName GetArrayTypeName(Red::CName aTypeName)
{
    // clang-format off
    switch (aTypeName)
    {
    case Int: return IntArray;
    case Float: return FloatArray;
    case Bool: return BoolArray;
    case String: return StringArray;
    case CName: return CNameArray;
    case LocKey: return LocKeyArray;
    case ResRef: return ResRefArray;
    case TweakDBID: return TweakDBIDArray;
    case Quaternion: return QuaternionArray;
    case EulerAngles: return EulerAnglesArray;
    case Vector3: return Vector3Array;
    case Vector2: return Vector2Array;
    case Color: return ColorArray;
    default: return {};
    }
    // clang-format on
}

Red::CName GetArrayTypeName(const CBaseRTTIType* aType)
{
    if (!aType)
        return {};

    return GetArrayTypeName(aType->GetName());
}

Red::CName GetElementTypeName(Red::CName aTypeName)
{
    // clang-format off
    switch (aTypeName)
    {
    case IntArray: return Int;
    case FloatArray: return Float;
    case BoolArray: return Bool;
    case StringArray: return String;
    case CNameArray: return CName;
    case TweakDBIDArray: return TweakDBID;
    case LocKeyArray: return LocKey;
    case ResRefArray: return ResRef;
    case QuaternionArray: return Quaternion;
    case EulerAnglesArray: return EulerAngles;
    case Vector3Array: return Vector3;
    case Vector2Array: return Vector2;
    case ColorArray: return Color;
    default: return {};
    }
    // clang-format on
}

Red::CName GetElementTypeName(const CBaseRTTIType* aType)
{
    if (!aType)
        return {};

    return GetElementTypeName(aType->GetName());
}

InstancePtr<> Construct(Red::CName aTypeName)
{
    // clang-format off
    switch (aTypeName)
    {
    case Int: return MakeInstance<int>();
    case Float: return MakeInstance<float>();
    case Bool: return MakeInstance<bool>();
    case String: return MakeInstance<Red::CString>();
    case CName: return MakeInstance<Red::CName>();
    case LocKey: return MakeInstance<Red::LocKeyWrapper>();
    case ResRef: return MakeInstance<Red::ResourceAsyncReference<>>();
    case TweakDBID: return MakeInstance<Red::TweakDBID>();
    case Quaternion: return MakeInstance<Red::Quaternion>();
    case EulerAngles: return MakeInstance<Red::EulerAngles>();
    case Vector3: return MakeInstance<Red::Vector3>();
    case Vector2: return MakeInstance<Red::Vector2>();
    case Color: return MakeInstance<Red::Color>();
    case IntArray: return MakeInstance<DynArray<int>>();
    case FloatArray: return MakeInstance<DynArray<float>>();
    case BoolArray: return MakeInstance<DynArray<bool>>();
    case StringArray: return MakeInstance<DynArray<Red::CString>>();
    case CNameArray: return MakeInstance<DynArray<Red::CName>>();
    case LocKeyArray: return MakeInstance<DynArray<Red::LocKeyWrapper>>();
    case ResRefArray: return MakeInstance<DynArray<Red::ResourceAsyncReference<>>>();
    case TweakDBIDArray: return MakeInstance<DynArray<Red::TweakDBID>>();
    case QuaternionArray: return MakeInstance<DynArray<Red::Quaternion>>();
    case EulerAnglesArray: return MakeInstance<DynArray<Red::EulerAngles>>();
    case Vector3Array: return MakeInstance<DynArray<Red::Vector3>>();
    case Vector2Array: return MakeInstance<DynArray<Red::Vector2>>();
    case ColorArray: return MakeInstance<DynArray<Red::Color>>();
    default: return {};
    }
    // clang-format on
}

InstancePtr<> Construct(const CBaseRTTIType* aType)
{
    if (!aType)
        return {};

    return Construct(aType->GetName());
}

ValuePtr<> ConstructValue(const CBaseRTTIType* aType)
{
    if (!aType || !IsFlatType(aType))
        return {};

    return MakeValue(aType);
}

ValuePtr<> ConstructValue(Red::CName aTypeName)
{
    return ConstructValue(CRTTISystem::Get()->GetType(aTypeName));
}

} // namespace Red::ERTDBFlatType