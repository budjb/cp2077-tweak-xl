#include "ScriptablePropertyHandler.hpp"
#include "ScriptableTweakDBRecord.hpp"
#include <RED4ext/Scripting/Natives/Generated/game/data/Vehicle_Record.hpp>
#include <spdlog/fmt/bundled/format.h>
#include <spdlog/fmt/bundled/ranges.h>

namespace App
{
std::string ScriptablePropertyHandler::GetFunctionBaseName(const std::string& aName) const
{
    return aName.substr(m_prefixLength, aName.length() - m_prefixLength - m_suffixLength);
}

Red::CGlobalFunction* ScriptablePropertyHandler::GetRTTIFunction() const
{
    return m_rttiFunction;
}

Context* ScriptablePropertyHandler::GetContext(Red::CStackFrame* aFrame)
{
    static constexpr auto PtrSize = sizeof(void*);

    auto* context = *reinterpret_cast<Context**>(aFrame->code);
    aFrame->code += PtrSize;
    return context;
}

Red::TweakDBID ScriptablePropertyHandler::GetFlatID(Red::Instance aInstance, const Context* aContext)
{
    return static_cast<ScriptableTweakDBRecord*>(aInstance)->recordID + aContext->appendix;
}

template<template<typename> typename THandle>
    requires(std::is_same_v<THandle<Red::TweakDBRecord>, Red::Handle<Red::TweakDBRecord>> ||
             std::is_same_v<THandle<Red::TweakDBRecord>, Red::WeakHandle<Red::TweakDBRecord>>)
auto ScriptablePropertyHandler::GetRecordArray(const Red::Value<>& aValue, const Context* aContext)
    -> Core::SharedPtr<Red::DynArray<THandle<Red::TweakDBRecord>>>
{
    if (!aContext->typeSpec->foreignType || aValue.type->GetType() != Red::rtti::ERTTIType::Array)
        return nullptr;

    const auto* flatArrayType = reinterpret_cast<const Red::CRTTIBaseArrayType*>(aValue.type);

    if (!Red::TweakDBUtil::IsArrayType(flatArrayType))
        return nullptr;

    const auto count = flatArrayType->GetLength(aValue.instance);
    const auto outArray = Core::MakeShared<Red::DynArray<THandle<Red::TweakDBRecord>>>(count);

    for (uint32_t i = 0; i < count; ++i)
    {
        const auto recordID = *static_cast<Red::TweakDBID*>(flatArrayType->GetElement(aValue.instance, i));
        const auto record = aContext->tweakManager->GetRecord(recordID);

        if (record && record->GetType()->IsA(aContext->typeSpec->foreignType))
            outArray->At(i) = record;
        else
            outArray->At(i) = nullptr;
    }

    return outArray;
}

Red::CName GetRecordArrayHandler::GetFunctionHash(const ScriptableRecordSpecPtr& aRecordSpec,
                                                  const ScriptablePropertySpecPtr& aPropSpec) const
{
    static constexpr auto VoidName = Red::GetTypeNameStr<void>();

    std::vector<std::string> segments;
    segments.emplace_back(aRecordSpec->name);
    segments.emplace_back(VoidName.data());
    segments.emplace_back(aPropSpec->functionName);
    segments.emplace_back(
        Red::TweakDBUtil::GetWHandleArrayType(aPropSpec->typeSpec->foreignType)->GetName().ToString());

    return fmt::format("{}", fmt::join(segments, ";")).c_str();
}

void GetRecordArrayHandler::Register()
{
    if (m_rttiFunction)
        return;

    const std::string name = std::string(ScriptablePropertyHandlerPrefix) + "_GetRecordArrayHandler";
    auto* function = Red::CGlobalFunction::Create(name.c_str(), name.c_str(), &HandleInvocation);
    Red::CRTTISystem::Get()->RegisterFunction(function);
    m_rttiFunction = function;
}

void GetRecordArrayHandler::HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                                             int64_t a4)
{
    (void)aOut;
    (void)a4;

    if (!aInstance || !aFrame)
        return;

    void* out;
    Red::GetParameter(aFrame, &out);

    ++aFrame->code; // ParamEnd

    const Context* context = GetContext(aFrame);

    const auto flat = context->tweakManager->GetFlat(GetFlatID(aInstance, context));

    if (const auto result = GetRecordArray<Red::WeakHandle>(flat, context))
        *static_cast<RecordArray*>(out) = *result;
}

Red::CName RecordArrayContainsHandler::GetFunctionHash(const ScriptableRecordSpecPtr& aRecordSpec,
                                                       const ScriptablePropertySpecPtr& aPropSpec) const
{
    static constexpr auto BoolName = Red::GetTypeNameStr<bool>();

    std::string funcName = aPropSpec->functionName + "Contains";

    std::vector<std::string> segments;
    segments.emplace_back(aRecordSpec->name);
    segments.emplace_back(BoolName.data());
    segments.emplace_back(funcName);
    segments.emplace_back(Red::TweakDBUtil::GetWHandleType(aPropSpec->typeSpec->foreignType)->GetName().ToString());

    return fmt::format("{}", fmt::join(segments, ";")).c_str();
}

void RecordArrayContainsHandler::Register()
{
    if (m_rttiFunction)
        return;

    const std::string name = std::string(ScriptablePropertyHandlerPrefix) + "_RecordArrayContainsHandler";
    auto* function = Red::CGlobalFunction::Create(name.c_str(), name.c_str(), &HandleInvocation);
    Red::CRTTISystem::Get()->RegisterFunction(function);
    m_rttiFunction = function;
}

void RecordArrayContainsHandler::HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                                                  int64_t a4)
{
    (void)a4;

    if (!aInstance || !aFrame)
        return;

    Red::WeakHandle<Red::TweakDBRecord>* item;
    Red::GetParameter(aFrame, &item);

    ++aFrame->code; // ParamEnd

    const Context* context = GetContext(aFrame);

    const auto flat = context->tweakManager->GetFlat(GetFlatID(aInstance, context));

    if (const auto result = GetRecordArray<Red::WeakHandle>(flat, context))
    {
        for (const auto& record : *result)
        {
            if (record.instance->recordID == item->instance->recordID &&
                record.instance->GetType() == item->instance->GetType())
            {
                *static_cast<bool*>(aOut) = true;
                return;
            }
        }
    }

    *static_cast<bool*>(aOut) = false;
}

Red::CName GetRecordItemHandler::GetFunctionHash(const ScriptableRecordSpecPtr& aRecordSpec,
                                                 const ScriptablePropertySpecPtr& aPropSpec) const
{
    static constexpr auto IntName = Red::GetTypeNameStr<int>();

    std::string funcName = "Get" + aPropSpec->functionName + "Item";

    std::vector<std::string> segments;
    segments.emplace_back(aRecordSpec->name);
    segments.emplace_back(Red::TweakDBUtil::GetWHandleType(aPropSpec->typeSpec->foreignType)->GetName().ToString());
    segments.emplace_back(funcName);
    segments.emplace_back(IntName.data());

    return fmt::format("{}", fmt::join(segments, ";")).c_str();
}

void GetRecordItemHandler::Register()
{
    if (m_rttiFunction)
        return;

    const std::string name = std::string(ScriptablePropertyHandlerPrefix) + "_GetRecordItemHandler";
    auto* function = Red::CGlobalFunction::Create(name.c_str(), name.c_str(), &HandleInvocation);
    Red::CRTTISystem::Get()->RegisterFunction(function);
    m_rttiFunction = function;
}

void GetRecordItemHandler::HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                                            int64_t a4)
{
    (void)a4;

    if (!aInstance || !aFrame)
        return;

    int index;
    Red::GetParameter(aFrame, &index);

    ++aFrame->code; // ParamEnd

    const Context* context = GetContext(aFrame);

    if (index < 0)
        return;

    if (const auto flat = context->tweakManager->GetFlat(GetFlatID(aInstance, context)))
    {
        if (const auto result = GetRecordArray<Red::WeakHandle>(flat, context))
        {
            if (index < result->Size())
            {
                if (const auto record = result->At(index);
                    record.instance->GetType()->IsA(context->typeSpec->foreignType))
                {
                    *static_cast<RecordWHandle*>(aOut) = result->At(index);
                }
            }
        }
    }
}

Red::CName GetRecordItemHandleHandler::GetFunctionHash(const ScriptableRecordSpecPtr& aRecordSpec,
                                                       const ScriptablePropertySpecPtr& aPropSpec) const
{
    static constexpr auto IntName = Red::GetTypeNameStr<int>();

    std::string funcName = "Get" + aPropSpec->functionName + "ItemHandle";

    std::vector<std::string> segments;
    segments.emplace_back(aRecordSpec->name);
    segments.emplace_back(Red::TweakDBUtil::GetHandleType(aPropSpec->typeSpec->foreignType)->GetName().ToString());
    segments.emplace_back(funcName);
    segments.emplace_back(IntName.data());

    return fmt::format("{}", fmt::join(segments, ";")).c_str();
}

void GetRecordItemHandleHandler::Register()
{
    if (m_rttiFunction)
        return;

    const std::string name = std::string(ScriptablePropertyHandlerPrefix) + "_GetRecordItemHandleHandler";
    auto* function = Red::CGlobalFunction::Create(name.c_str(), name.c_str(), &HandleInvocation);
    Red::CRTTISystem::Get()->RegisterFunction(function);
    m_rttiFunction = function;
}

void GetRecordItemHandleHandler::HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                                                  int64_t a4)
{
    (void)a4;

    if (!aInstance || !aFrame)
        return;

    int index;
    Red::GetParameter(aFrame, &index);

    ++aFrame->code; // ParamEnd

    const Context* context = GetContext(aFrame);

    if (index < 0)
        return;

    if (const auto flat = context->tweakManager->GetFlat(GetFlatID(aInstance, context)))
    {
        if (const auto result = GetRecordArray<Red::Handle>(flat, context))
        {
            if (index < result->Size())
            {
                if (const auto record = result->At(index);
                    record.instance->GetType()->IsA(context->typeSpec->foreignType))
                {
                    *static_cast<RecordHandle*>(aOut) = result->At(index);
                }
            }
        }
    }
}

Red::CName GetRecordHandler::GetFunctionHash(const ScriptableRecordSpecPtr& aRecordSpec,
                                             const ScriptablePropertySpecPtr& aPropSpec) const
{
    std::vector<std::string> segments;
    segments.emplace_back(aRecordSpec->name);
    segments.emplace_back(Red::TweakDBUtil::GetWHandleType(aPropSpec->typeSpec->foreignType)->GetName().ToString());
    segments.emplace_back(aPropSpec->functionName);

    return fmt::format("{}", fmt::join(segments, ";")).c_str();
}

void GetRecordHandler::Register()
{
    if (m_rttiFunction)
        return;

    const std::string name = std::string(ScriptablePropertyHandlerPrefix) + "_GetRecordHandler";
    auto* function = Red::CGlobalFunction::Create(name.c_str(), name.c_str(), &HandleInvocation);
    Red::CRTTISystem::Get()->RegisterFunction(function);
    m_rttiFunction = function;
}

void GetRecordHandler::HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut, int64_t a4)
{
    (void)a4;

    if (!aInstance || !aFrame)
        return;

    ++aFrame->code;

    const Context* context = GetContext(aFrame);

    if (const auto flat = context->tweakManager->GetFlat(GetFlatID(aInstance, context)))
    {
        const auto* id = static_cast<Red::TweakDBID*>(flat.instance);

        if (const auto record = context->tweakManager->GetRecord(*id);
            record && record->GetType()->IsA(context->typeSpec->foreignType))
        {
            *static_cast<RecordWHandle*>(aOut) = record;
        }
    }
}

Red::CName GetRecordHandleHandler::GetFunctionHash(const ScriptableRecordSpecPtr& aRecordSpec,
                                                   const ScriptablePropertySpecPtr& aPropSpec) const
{
    std::string funcName = aPropSpec->functionName + "Handle";

    std::vector<std::string> segments;
    segments.emplace_back(aRecordSpec->name);
    segments.emplace_back(Red::TweakDBUtil::GetHandleType(aPropSpec->typeSpec->foreignType)->GetName().ToString());
    segments.emplace_back(funcName);

    return fmt::format("{}", fmt::join(segments, ";")).c_str();
}

void GetRecordHandleHandler::Register()
{
    if (m_rttiFunction)
        return;

    const std::string name = std::string(ScriptablePropertyHandlerPrefix) + "_GetRecordHandleHandler";
    auto* function = Red::CGlobalFunction::Create(name.c_str(), name.c_str(), &HandleInvocation);
    Red::CRTTISystem::Get()->RegisterFunction(function);
    m_rttiFunction = function;
}

void GetRecordHandleHandler::HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                                              int64_t a4)
{
    (void)a4;

    if (!aInstance || !aFrame)
        return;

    ++aFrame->code;

    const Context* context = GetContext(aFrame);

    if (const auto flat = context->tweakManager->GetFlat(GetFlatID(aInstance, context)))
    {
        const auto* id = static_cast<Red::TweakDBID*>(flat.instance);

        if (const auto record = context->tweakManager->GetRecord(*id);
            record && record->GetType()->IsA(context->typeSpec->foreignType))
        {
            *static_cast<RecordHandle*>(aOut) = record;
        }
    }
}

Red::CName GetArrayCountHandler::GetFunctionHash(const ScriptableRecordSpecPtr& aRecordSpec,
                                                 const ScriptablePropertySpecPtr& aPropSpec) const
{
    static constexpr auto IntName = Red::GetTypeNameStr<int>();

    std::string funcName = "Get" + aPropSpec->functionName + "Count";

    std::vector<std::string> segments;
    segments.emplace_back(aRecordSpec->name);
    segments.emplace_back(IntName.data());
    segments.emplace_back(funcName);

    return fmt::format("{}", fmt::join(segments, ";")).c_str();
}

void GetArrayCountHandler::Register()
{
    if (m_rttiFunction)
        return;

    const std::string name = std::string(ScriptablePropertyHandlerPrefix) + "_GetArrayCountHandler";
    auto* function = Red::CGlobalFunction::Create(name.c_str(), name.c_str(), &HandleInvocation);
    Red::CRTTISystem::Get()->RegisterFunction(function);
    m_rttiFunction = function;
}

void GetArrayCountHandler::HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                                            int64_t a4)
{
    (void)a4;

    if (!aInstance || !aFrame)
        return;

    ++aFrame->code;

    const Context* context = GetContext(aFrame);

    if (const auto flat = context->tweakManager->GetFlat(GetFlatID(aInstance, context));
        flat.type->GetType() == Red::rtti::ERTTIType::Array)
    {
        const auto* arrayType = reinterpret_cast<const Red::CRTTIBaseArrayType*>(flat.type);
        *static_cast<uint32_t*>(aOut) = arrayType->GetLength(flat.instance);
    }
    else
    {
        *static_cast<int*>(aOut) = 0;
    }
}

Red::CName GetArrayItemHandler::GetFunctionHash(const ScriptableRecordSpecPtr& aRecordSpec,
                                                const ScriptablePropertySpecPtr& aPropSpec) const
{
    static constexpr auto IntName = Red::GetTypeNameStr<int>();

    std::string funcName = "Get" + aPropSpec->functionName + "Item";

    std::vector<std::string> segments;
    segments.emplace_back(aRecordSpec->name);
    segments.emplace_back(aPropSpec->typeSpec->elementType->GetName().ToString());
    segments.emplace_back(funcName);
    segments.emplace_back(IntName.data());

    return fmt::format("{}", fmt::join(segments, ";")).c_str();
}

void GetArrayItemHandler::Register()
{
    if (m_rttiFunction)
        return;

    const std::string name = std::string(ScriptablePropertyHandlerPrefix) + "_GetArrayItemHandler";
    auto* function = Red::CGlobalFunction::Create(name.c_str(), name.c_str(), &HandleInvocation);
    Red::CRTTISystem::Get()->RegisterFunction(function);
    m_rttiFunction = function;
}

void GetArrayItemHandler::HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                                           int64_t a4)
{
    (void)a4;

    if (!aInstance || !aFrame)
        return;

    int index;
    Red::GetParameter(aFrame, &index);

    ++aFrame->code; // ParamEnd

    const Context* context = GetContext(aFrame);

    const auto flat = context->tweakManager->GetFlat(GetFlatID(aInstance, context));

    if (flat.type != context->typeSpec->propertyType || flat.type->GetType() != Red::rtti::ERTTIType::Array)
        return;

    const auto* arrayType = reinterpret_cast<const Red::CRTTIBaseArrayType*>(flat.type);
    const auto* innerType = arrayType->GetInnerType();
    const auto length = arrayType->GetLength(flat.instance);

    if (index >= 0 && static_cast<uint32_t>(index) < length)
    {
        innerType->Assign(aOut, arrayType->GetElement(flat.instance, index));
    }
}

Red::CName ArrayContainsHandler::GetFunctionHash(const ScriptableRecordSpecPtr& aRecordSpec,
                                                 const ScriptablePropertySpecPtr& aPropSpec) const
{
    static constexpr auto BoolName = Red::GetTypeNameStr<bool>();

    std::string funcName = aPropSpec->functionName + "Contains";

    std::vector<std::string> segments;
    segments.emplace_back(aRecordSpec->name);
    segments.emplace_back(BoolName.data());
    segments.emplace_back(funcName);
    segments.emplace_back(aPropSpec->typeSpec->elementType->GetName().ToString());

    return fmt::format("{}", fmt::join(segments, ";")).c_str();
}

void ArrayContainsHandler::Register()
{
    if (m_rttiFunction)
        return;

    const std::string name = std::string(ScriptablePropertyHandlerPrefix) + "_ArrayContainsHandler";
    auto* function = Red::CGlobalFunction::Create(name.c_str(), name.c_str(), &HandleInvocation);
    Red::CRTTISystem::Get()->RegisterFunction(function);
    m_rttiFunction = function;
}

void ArrayContainsHandler::HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                                            int64_t a4)
{
    (void)a4;

    if (!aInstance || !aFrame)
        return;

    void* item;
    Red::GetParameter(aFrame, &item);

    ++aFrame->code; // ParamEnd

    const Context* context = GetContext(aFrame);

    const auto flat = context->tweakManager->GetFlat(GetFlatID(aInstance, context));

    if (flat.type != context->typeSpec->propertyType || flat.type->GetType() != Red::rtti::ERTTIType::Array)
    {
        *static_cast<bool*>(aOut) = false;
        return;
    }

    const auto* arrayType = reinterpret_cast<const Red::CRTTIBaseArrayType*>(flat.type);
    auto* innerType = arrayType->GetInnerType();
    const auto length = arrayType->GetLength(flat.instance);

    for (uint32_t i = 0; i < length; ++i)
    {
        if (innerType->IsEqual(arrayType->GetElement(flat.instance, i), item))
        {
            *static_cast<bool*>(aOut) = true;
            return;
        }
    }

    *static_cast<bool*>(aOut) = false;
}

Red::CName GetValueHandler::GetFunctionHash(const ScriptableRecordSpecPtr& aRecordSpec,
                                            const ScriptablePropertySpecPtr& aPropSpec) const
{
    std::vector<std::string> segments;
    segments.emplace_back(aRecordSpec->name);
    segments.emplace_back(aPropSpec->typeSpec->propertyType->GetName().ToString());
    segments.emplace_back(aPropSpec->functionName);

    return fmt::format("{}", fmt::join(segments, ";")).c_str();
}

void GetValueHandler::Register()
{
    if (m_rttiFunction)
        return;

    const std::string name = std::string(ScriptablePropertyHandlerPrefix) + "_GetValueHandler";
    auto* function = Red::CGlobalFunction::Create(name.c_str(), name.c_str(), &HandleInvocation);
    Red::CRTTISystem::Get()->RegisterFunction(function);
    m_rttiFunction = function;
}

void GetValueHandler::HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut, int64_t a4)
{
    (void)a4;

    if (!aInstance || !aFrame)
        return;

    ++aFrame->code;

    const Context* context = GetContext(aFrame);

    const auto flat = context->tweakManager->GetFlat(GetFlatID(aInstance, context));

    if (flat.type != context->typeSpec->propertyType)
        return;

    flat.type->Assign(aOut, flat.instance);
}

ScriptablePropertyHandlerRegistry::ScriptablePropertyHandlerRegistry(
    const Core::DeferredPtr<Red::TweakDBManager>& aManager)
    : m_manager(aManager)
{
}

void ScriptablePropertyHandlerRegistry::RegisterRTTIFunctions()
{
    s_getRecordArrayHandler.Register();
    s_recordArrayContainsHandler.Register();
    s_getRecordItemHandler.Register();
    s_getRecordItemHandleHandler.Register();
    s_getRecordHandler.Register();
    s_getRecordHandleHandler.Register();
    s_getArrayCountHandler.Register();
    s_getArrayItemHandler.Register();
    s_getArrayContainsHandler.Register();
    s_getValueHandler.Register();
}

void ScriptablePropertyHandlerRegistry::RegisterScriptableProperty(const ScriptableRecordSpecPtr& aRecordSpec,
                                                                   const ScriptablePropertySpecPtr& aPropSpec)
{
    if (aPropSpec->typeSpec->isArray && aPropSpec->typeSpec->isForeignKey)
    {
        RegisterPropertyFunction<GetterType::GetRecordArray>(aRecordSpec, aPropSpec);
        RegisterPropertyFunction<GetterType::GetArrayCount>(aRecordSpec, aPropSpec);
        RegisterPropertyFunction<GetterType::GetRecordItem>(aRecordSpec, aPropSpec);
        RegisterPropertyFunction<GetterType::GetRecordItemHandle>(aRecordSpec, aPropSpec);
        RegisterPropertyFunction<GetterType::RecordArrayContains>(aRecordSpec, aPropSpec);
    }
    else if (!aPropSpec->typeSpec->isArray && aPropSpec->typeSpec->isForeignKey)
    {
        RegisterPropertyFunction<GetterType::GetRecord>(aRecordSpec, aPropSpec);
        RegisterPropertyFunction<GetterType::GetRecordHandle>(aRecordSpec, aPropSpec);
    }
    else if (aPropSpec->typeSpec->isArray && Red::TweakDBUtil::IsResRefTokenArray(aPropSpec->typeSpec->propertyType))
    {
        RegisterPropertyFunction<GetterType::Get>(aRecordSpec, aPropSpec);
        RegisterPropertyFunction<GetterType::GetArrayCount>(aRecordSpec, aPropSpec);
        RegisterPropertyFunction<GetterType::GetArrayItem>(aRecordSpec, aPropSpec);
    }
    else if (aPropSpec->typeSpec->isArray)
    {
        RegisterPropertyFunction<GetterType::Get>(aRecordSpec, aPropSpec);
        RegisterPropertyFunction<GetterType::GetArrayCount>(aRecordSpec, aPropSpec);
        RegisterPropertyFunction<GetterType::GetArrayItem>(aRecordSpec, aPropSpec);
        RegisterPropertyFunction<GetterType::ArrayContains>(aRecordSpec, aPropSpec);
    }
    else
    {
        RegisterPropertyFunction<GetterType::Get>(aRecordSpec, aPropSpec);
    }
}

bool ScriptablePropertyHandlerRegistry::AdaptScriptFunction(const ScriptableRecordSpecPtr& aRecordSpec,
                                                            Red::CClassFunction* aFunc)
{
    const auto functionHash = GetFunctionHash(aRecordSpec, aFunc);

    std::shared_lock lockR(m_functionTypesMutex);

    if (const auto it = m_functionTypes.find(aRecordSpec->cname); it != m_functionTypes.end())
    {
        if (const auto it2 = it->second.find(functionHash); it2 != it->second.end())
        {
            const auto handler = GetHandler(it2->second);
            const auto baseFunctionName = handler->GetFunctionBaseName(aFunc->shortName.ToString());

            if (const auto propSpec = aRecordSpec->FindPropertyByFunctionName(baseFunctionName))
            {
                // TODO: truncate it here?
                if (!propSpec->isDescribed)
                    return false;

                const auto context = GetContext(aRecordSpec, propSpec);
                ReplaceScriptFunction(aFunc, context, handler->GetRTTIFunction());
                return true;
            }
        }
    }

    return false;
}

ScriptablePropertyHandler* ScriptablePropertyHandlerRegistry::GetHandler(const GetterType aType)
{
    // clang-format off
    switch (aType)
    {
    case GetterType::GetRecordArray: return &s_getRecordArrayHandler;
    case GetterType::RecordArrayContains: return &s_recordArrayContainsHandler;
    case GetterType::GetRecordItem: return &s_getRecordItemHandler;
    case GetterType::GetRecordItemHandle: return &s_getRecordItemHandleHandler;
    case GetterType::GetRecord: return &s_getRecordHandler;
    case GetterType::GetRecordHandle: return &s_getRecordHandleHandler;
    case GetterType::GetArrayCount: return &s_getArrayCountHandler;
    case GetterType::GetArrayItem: return &s_getArrayItemHandler;
    case GetterType::ArrayContains: return &s_getArrayContainsHandler;
    case GetterType::Get: return &s_getValueHandler;
    default: return nullptr;
    }
    // clang-format on
}

ContextPtr ScriptablePropertyHandlerRegistry::GetContext(const ScriptableRecordSpecPtr& aRecordSpec,
                                                         const ScriptablePropertySpecPtr& aPropSpec)
{
    {
        std::shared_lock lockRW(m_contextsMutex);
        if (const auto it = m_contexts.find(aRecordSpec->cname); it != m_contexts.end())
        {
            if (const auto it2 = it->second.find(aPropSpec->cname); it2 != it->second.end())
            {
                return it2->second;
            }
        }
    }

    const auto context = Core::MakeShared<Context>();

    context->tweakManager = m_manager;
    context->typeSpec = aPropSpec->typeSpec;
    context->appendix = aPropSpec->appendix;

    {
        std::unique_lock lockRW(m_contextsMutex);
        m_contexts[aRecordSpec->cname][aPropSpec->cname] = context;
    }

    return context;
}

template<GetterType Type>
void ScriptablePropertyHandlerRegistry::RegisterPropertyFunction(const ScriptableRecordSpecPtr& aRecordSpec,
                                                                 const ScriptablePropertySpecPtr& aPropSpec)
{
    const auto* handler = GetHandler(Type);
    const auto hash = handler->GetFunctionHash(aRecordSpec, aPropSpec);
    std::unique_lock lockRW(m_functionTypesMutex);
    m_functionTypes[aRecordSpec->cname][hash] = Type;
}

void ScriptablePropertyHandlerRegistry::ReplaceScriptFunction(Red::CClassFunction* aFunction,
                                                              const ContextPtr& aContext,
                                                              Red::CGlobalFunction* aNativeFunc)
{
    const auto bytecode = CreateFunctionBytecode(aContext, aFunction, aNativeFunc);
    aFunction->bytecode.bytecode.buffer.data = bytecode.data;
    aFunction->bytecode.bytecode.buffer.size = bytecode.size;
}

void ScriptablePropertyHandlerRegistry::TruncateScriptFunction(Red::CClassFunction* aFunction)
{
    aFunction->bytecode.bytecode.buffer.data = nullptr;
    aFunction->bytecode.bytecode.buffer.size = 0;
}

Red::RawBuffer ScriptablePropertyHandlerRegistry::CreateFunctionBytecode(const ContextPtr& aContext,
                                                                         Red::CClassFunction* aFunction,
                                                                         Red::CGlobalFunction* aNativeFunction)
{
    constexpr uint8_t ParamOp = 25;
    constexpr uint8_t CallStaticOp = 36;
    constexpr uint8_t ParamEndOp = 38;
    constexpr uint8_t ReturnOp = 39;
    constexpr uint32_t OpSize = sizeof(char);
    constexpr uint32_t OffsetSize = sizeof(uint16_t);
    constexpr uint32_t FlagsSize = sizeof(uint16_t);
    constexpr uint32_t PointerSize = sizeof(void*);
    constexpr uint32_t BaseCodeSize = OpSize + OffsetSize * 2 + PointerSize + FlagsSize + OpSize;
    constexpr uint16_t BaseExitOffset = BaseCodeSize - OpSize - OffsetSize;

    const uint32_t extraCodeSize =
        aFunction->params.Size() * (OpSize + PointerSize) + PointerSize + (aFunction->returnType ? 1 : 0);
    const uint32_t finalCodeSize = BaseCodeSize + extraCodeSize;
    const uint16_t finalExitOffset = BaseExitOffset + extraCodeSize;

    constexpr Red::Memory::EngineAllocator allocator;
    auto* memory = allocator.Alloc(finalCodeSize).memory;
    auto* code = static_cast<uint8_t*>(memory);

    if (aFunction->returnType)
    {
        *code = ReturnOp;
        code += OpSize;
    }

    *code = CallStaticOp;
    code += OpSize;

    *reinterpret_cast<uint16_t*>(code) = finalExitOffset;
    code += OffsetSize;

    *reinterpret_cast<uint16_t*>(code) = 0;
    code += OffsetSize;

    *reinterpret_cast<void**>(code) = aNativeFunction;
    code += PointerSize;

    *reinterpret_cast<uint16_t*>(code) = 0;
    code += FlagsSize;

    for (const auto& param : aFunction->params)
    {
        *code = ParamOp;
        code += OpSize;

        *reinterpret_cast<void**>(code) = param;
        code += PointerSize;
    }

    *code = ParamEndOp;
    code += OpSize;

    *reinterpret_cast<const Context**>(code) = aContext.get();

    return {memory, finalCodeSize};
}

Red::CName ScriptablePropertyHandlerRegistry::GetFunctionHash(const ScriptableRecordSpecPtr& aRecordSpec,
                                                              Red::CClassFunction* aFunction)
{
    std::vector<std::string> segments;
    segments.emplace_back(aRecordSpec->name);
    segments.emplace_back(aFunction->returnType ? aFunction->returnType->type->GetName().ToString() : "void");
    segments.emplace_back(aFunction->shortName.ToString());

    for (const auto& param : aFunction->params)
    {
        segments.emplace_back(param->type->GetName().ToString());
    }

    return fmt::format("{}", fmt::join(segments, ";")).c_str();
}

} // namespace App
