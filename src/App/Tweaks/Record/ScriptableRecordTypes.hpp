#pragma once

#include "App/Tweaks/Record/ScriptableRecordClass.hpp"
#include "App/Tweaks/TweakTypeSpec.hpp"
#include "Red/TweakDB/Manager.hpp"

namespace App
{
using RecordArray = Red::DynArray<Red::WeakHandle<Red::TweakDBRecord>>;
using RecordArrayPtr = Red::InstancePtr<RecordArray>;
using RecordWHandle = Red::WeakHandle<Red::TweakDBRecord>;
using RecordHandle = Red::Handle<Red::TweakDBRecord>;

struct Context
{
    std::string appendix;
    TweakTypeSpecPtr typeSpec;
    Core::DeferredPtr<Red::TweakDBManager> tweakManager;
};

using ContextPtr = Core::SharedPtr<Context>;

struct ScriptablePropertySpec
{
    std::string name;
    std::string functionName;
    Red::CName cname;
    std::string appendix;
    TweakTypeSpecPtr typeSpec;
    Red::InstancePtr<> defaultValue;
    bool isDescribed = false;
};

using ScriptablePropertySpecPtr = Core::SharedPtr<ScriptablePropertySpec>;

struct ScriptableRecordSpec
{
    [[nodiscard]] ScriptablePropertySpecPtr FindPropertyByFunctionName(const std::string& aFunctionName) const;

    std::string name;
    std::string aliasName;
    std::string shortName;
    Red::CName cname;
    Red::CName aliasCName;
    Red::CName shortCName;
    uint32_t hash;
    ScriptableRecordClass* type;
    std::optional<std::string> parent;
    Core::Map<Red::CName, ScriptablePropertySpecPtr> props;
    bool isRegistered = false;
    bool isDescribed = false;
    bool isInserted = false;
};

using ScriptableRecordSpecPtr = Core::SharedPtr<ScriptableRecordSpec>;

} // namespace App
