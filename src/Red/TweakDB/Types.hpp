#pragma once

namespace Red::ERTDBFlatType
{
enum : uint64_t
{
    Int = FNV1a64("Int32"),
    Float = FNV1a64("Float"),
    Bool = FNV1a64("Bool"),
    String = FNV1a64("String"),
    CName = FNV1a64("CName"),
    LocKey = FNV1a64("gamedataLocKeyWrapper"),
    ResRef = FNV1a64("raRef:CResource"),
    TweakDBID = FNV1a64("TweakDBID"),
    Quaternion = FNV1a64("Quaternion"),
    EulerAngles = FNV1a64("EulerAngles"),
    Vector3 = FNV1a64("Vector3"),
    Vector2 = FNV1a64("Vector2"),
    Color = FNV1a64("Color"),
    IntArray = FNV1a64("array:Int32"),
    FloatArray = FNV1a64("array:Float"),
    BoolArray = FNV1a64("array:Bool"),
    StringArray = FNV1a64("array:String"),
    CNameArray = FNV1a64("array:CName"),
    LocKeyArray = FNV1a64("array:gamedataLocKeyWrapper"),
    ResRefArray = FNV1a64("array:raRef:CResource"),
    TweakDBIDArray = FNV1a64("array:TweakDBID"),
    QuaternionArray = FNV1a64("array:Quaternion"),
    EulerAnglesArray = FNV1a64("array:EulerAngles"),
    Vector3Array = FNV1a64("array:Vector3"),
    Vector2Array = FNV1a64("array:Vector2"),
    ColorArray = FNV1a64("array:Color"),
};

} // namespace Red::ERTDBFlatType
