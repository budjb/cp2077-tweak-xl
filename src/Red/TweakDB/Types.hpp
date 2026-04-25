#pragma once

namespace Red::ERTDBFlatType
{
enum : uint64_t
{
    Int = Red::FNV1a64("Int32"),
    Float = Red::FNV1a64("Float"),
    Bool = Red::FNV1a64("Bool"),
    String = Red::FNV1a64("String"),
    CName = Red::FNV1a64("CName"),
    LocKey = Red::FNV1a64("gamedataLocKeyWrapper"),
    ResRef = Red::FNV1a64("raRef:CResource"),
    TweakDBID = Red::FNV1a64("TweakDBID"),
    Quaternion = Red::FNV1a64("Quaternion"),
    EulerAngles = Red::FNV1a64("EulerAngles"),
    Vector3 = Red::FNV1a64("Vector3"),
    Vector2 = Red::FNV1a64("Vector2"),
    Color = Red::FNV1a64("Color"),
    IntArray = Red::FNV1a64("array:Int32"),
    FloatArray = Red::FNV1a64("array:Float"),
    BoolArray = Red::FNV1a64("array:Bool"),
    StringArray = Red::FNV1a64("array:String"),
    CNameArray = Red::FNV1a64("array:CName"),
    LocKeyArray = Red::FNV1a64("array:gamedataLocKeyWrapper"),
    ResRefArray = Red::FNV1a64("array:raRef:CResource"),
    TweakDBIDArray = Red::FNV1a64("array:TweakDBID"),
    QuaternionArray = Red::FNV1a64("array:Quaternion"),
    EulerAnglesArray = Red::FNV1a64("array:EulerAngles"),
    Vector3Array = Red::FNV1a64("array:Vector3"),
    Vector2Array = Red::FNV1a64("array:Vector2"),
    ColorArray = Red::FNV1a64("array:Color"),
};

CBaseRTTIType* GetType(const uint64_t aType);
CBaseRTTIType* GetType(Red::CName aTypeName);
CBaseRTTIType* GetArrayType(Red::CName aTypeName);
CBaseRTTIType* GetArrayType(const CBaseRTTIType* aType);
CBaseRTTIType* GetElementType(Red::CName aTypeName);
CBaseRTTIType* GetElementType(const CBaseRTTIType* aType);
bool IsFlatType(Red::CName aTypeName);
bool IsFlatType(const CBaseRTTIType* aType);
bool IsArrayType(Red::CName aTypeName);
bool IsArrayType(const CBaseRTTIType* aType);
bool IsForeignKey(Red::CName aTypeName);
bool IsForeignKey(const CBaseRTTIType* aType);
bool IsForeignKeyArray(Red::CName aTypeName);
bool IsForeignKeyArray(const CBaseRTTIType* aType);
bool IsResRefToken(Red::CName aTypeName);
bool IsResRefToken(const CBaseRTTIType* aType);
bool IsResRefTokenArray(Red::CName aTypeName);
bool IsResRefTokenArray(const CBaseRTTIType* aType);
Red::CName GetArrayTypeName(Red::CName aTypeName);
Red::CName GetArrayTypeName(const CBaseRTTIType* aType);
Red::CName GetElementTypeName(Red::CName aTypeName);
Red::CName GetElementTypeName(const CBaseRTTIType* aType);
InstancePtr<> Construct(Red::CName aTypeName);
InstancePtr<> Construct(const CBaseRTTIType* aType);
ValuePtr<> ConstructValue(const CBaseRTTIType* aType);
ValuePtr<> ConstructValue(Red::CName aTypeName);
} // namespace Red::ERTDBFlatType
