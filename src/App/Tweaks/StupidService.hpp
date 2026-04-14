#pragma once

#include "App/Tweaks/Custom/CustomTweakRecord.hpp"
#include "Core/Foundation/Feature.hpp"
#include "Red/TweakDB/Reflection.hpp"
#include "spdlog/spdlog.h"

namespace App
{

using namespace Red;

constexpr auto CustomTweakRecordFlags = CClass::Flags{.isScriptedClass = true, .isImportOnly = true};
constexpr auto CustomTweakFunctionFlags = CScriptedFunction::Flags{.isNative = true, .isPublic = true};

class StupidService : public Core::Feature
{
protected:
    static void RegisterCustomType()
    {
        auto* rtti = CRTTISystem::Get();
        auto* customRecordType = rtti->GetClass(CustomTweakRecord::NAME);

        const CName name = CNamePool::Add("gamedataMyCustomType_Record");
        rtti->CreateScriptedClass(name, CustomTweakRecordFlags, customRecordType);
        rtti->RegisterScriptName(name, TweakDBReflection::GetRecordAliasName<CName>(name));

        auto* cls = rtti->GetClass(name);

        auto* func = CClassFunction::Create(cls, "Foo", "Foo", &ExecuteCustomTweakFunction, CustomTweakFunctionFlags);
        func->SetReturnType("CName");

        cls->RegisterFunction(func);

        const auto instance = static_cast<CustomTweakRecord*>(cls->CreateInstance(true));
        spdlog::info("Custom class type: {}", instance->GetType()->GetName().ToString());
        const auto* it = instance->GetNativeType();
        spdlog::info("Custom native class type: {}", it ? it->GetName().ToString() : "null");
    }

    void OnBootstrap() override
    {
        auto* rtti = Red::CRTTISystem::Get();
        rtti->AddPostRegisterCallback(&RegisterCustomType);
    }

    static void ExecuteCustomTweakFunction(IScriptable* aInstance, CStackFrame* aStackFrame, void* out, int64_t aType)
    {

    }
};

} // namespace Red
