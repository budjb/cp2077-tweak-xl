#include "ScriptablePropertyHandler.hpp"
#include "ScriptableTweakDBRecord.hpp"
#include <spdlog/fmt/bundled/ranges.h>

namespace App
{
std::string ScriptablePropertyHandler::GetFunctionBaseName(const std::string& aName) const
{
    return aName.substr(m_prefixLength, aName.length() - m_prefixLength - m_suffixLength);
}

Red::TweakDBID ScriptablePropertyHandler::GetFlatID(const Red::Instance aInstance, const Context* aContext)
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

void GetRecordArrayHandler::HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                                             const Context* aContext) const
{
    (void)aOut;

    void* out;
    Red::GetParameter(aFrame, &out);

    ++aFrame->code; // ParamEnd

    const auto flat = aContext->tweakManager->GetFlat(GetFlatID(aInstance, aContext));

    if (const auto result = GetRecordArray<Red::WeakHandle>(flat, aContext))
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

void RecordArrayContainsHandler::HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                                                  const Context* aContext) const
{
    Red::WeakHandle<Red::TweakDBRecord>* item;
    Red::GetParameter(aFrame, &item);

    ++aFrame->code; // ParamEnd

    if (!aOut)
        return;

    const auto flat = aContext->tweakManager->GetFlat(GetFlatID(aInstance, aContext));

    if (const auto result = GetRecordArray<Red::WeakHandle>(flat, aContext))
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

void GetRecordItemHandler::HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                                            const Context* aContext) const
{
    int index;
    Red::GetParameter(aFrame, &index);

    ++aFrame->code; // ParamEnd

    if (!aOut)
        return;

    if (index < 0)
        return;

    if (const auto flat = aContext->tweakManager->GetFlat(GetFlatID(aInstance, aContext)))
    {
        if (const auto result = GetRecordArray<Red::WeakHandle>(flat, aContext))
        {
            if (index < result->Size())
            {
                if (const auto record = result->At(index);
                    record.instance->GetType()->IsA(aContext->typeSpec->foreignType))
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

void GetRecordItemHandleHandler::HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                                                  const Context* aContext) const
{
    int index;
    Red::GetParameter(aFrame, &index);

    ++aFrame->code; // ParamEnd

    if (!aOut)
        return;

    if (index < 0)
        return;

    if (const auto flat = aContext->tweakManager->GetFlat(GetFlatID(aInstance, aContext)))
    {
        if (const auto result = GetRecordArray<Red::Handle>(flat, aContext))
        {
            if (index < result->Size())
            {
                if (const auto record = result->At(index);
                    record.instance->GetType()->IsA(aContext->typeSpec->foreignType))
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

void GetRecordHandler::HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                                        const Context* aContext) const
{
    ++aFrame->code;

    if (!aOut)
        return;

    if (const auto flat = aContext->tweakManager->GetFlat(GetFlatID(aInstance, aContext)))
    {
        const auto* id = static_cast<Red::TweakDBID*>(flat.instance);

        if (const auto record = aContext->tweakManager->GetRecord(*id);
            record && record->GetType()->IsA(aContext->typeSpec->foreignType))
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

void GetRecordHandleHandler::HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                                              const Context* aContext) const
{
    ++aFrame->code;

    if (!aOut)
        return;

    if (const auto flat = aContext->tweakManager->GetFlat(GetFlatID(aInstance, aContext)))
    {
        const auto* id = static_cast<Red::TweakDBID*>(flat.instance);

        if (const auto record = aContext->tweakManager->GetRecord(*id);
            record && record->GetType()->IsA(aContext->typeSpec->foreignType))
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

void GetArrayCountHandler::HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                                            const Context* aContext) const
{
    ++aFrame->code;

    if (!aOut)
        return;

    if (const auto flat = aContext->tweakManager->GetFlat(GetFlatID(aInstance, aContext));
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

void GetArrayItemHandler::HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                                           const Context* aContext) const
{
    int index;
    Red::GetParameter(aFrame, &index);

    ++aFrame->code; // ParamEnd

    if (!aOut)
        return;

    const auto flat = aContext->tweakManager->GetFlat(GetFlatID(aInstance, aContext));

    if (flat.type != aContext->typeSpec->propertyType || flat.type->GetType() != Red::rtti::ERTTIType::Array)
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

void ArrayContainsHandler::HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                                            const Context* aContext) const
{
    void* item;
    Red::GetParameter(aFrame, &item);

    ++aFrame->code; // ParamEnd

    if (!aOut)
        return;

    const auto flat = aContext->tweakManager->GetFlat(GetFlatID(aInstance, aContext));

    if (flat.type != aContext->typeSpec->propertyType || flat.type->GetType() != Red::rtti::ERTTIType::Array)
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

Red::CName GetResRefArrayHandler::GetFunctionHash(const ScriptableRecordSpecPtr& aRecordSpec,
                                                  const ScriptablePropertySpecPtr& aPropSpec) const
{
    static constexpr auto ResRefArrayName = Red::GetTypeNameStr<Red::DynArray<Red::ResRef>>();

    std::vector<std::string> segments;
    segments.emplace_back(aRecordSpec->name);
    segments.emplace_back(ResRefArrayName.data());
    segments.emplace_back(aPropSpec->functionName);

    return fmt::format("{}", fmt::join(segments, ";")).c_str();
}

void GetResRefArrayHandler::HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                                             const Context* aContext) const
{
    static const auto ResRefArrayType = reinterpret_cast<Red::CRTTIBaseArrayType*>(
        Red::TypeLocator<Red::GetTypeName<Red::DynArray<Red::ResRef>>()>::Get());
    static const auto RaRefArrayType = reinterpret_cast<Red::CRTTIBaseArrayType*>(
        Red::TypeLocator<Red::GetTypeName<Red::DynArray<Red::RaRef<Red::CResource>>>()>::Get());

    ++aFrame->code;

    if (!aOut)
        return;

    const auto flat = aContext->tweakManager->GetFlat(GetFlatID(aInstance, aContext));

    if (flat.type != RaRefArrayType)
        return;

    const auto length = RaRefArrayType->GetLength(flat.instance);

    auto result = Red::DynArray<Red::ResRef>();
    result.Resize(length);

    for (uint32_t i = 0; i < length; ++i)
    {
        ResRefArrayType->GetInnerType()->Assign(&result.At(i), RaRefArrayType->GetElement(flat.instance, i));
    }

    *static_cast<Red::DynArray<Red::ResRef>*>(aOut) = result;
}

Red::CName GetResRefItemHandler::GetFunctionHash(const ScriptableRecordSpecPtr& aRecordSpec,
                                                 const ScriptablePropertySpecPtr& aPropSpec) const
{
    static constexpr auto IntName = Red::GetTypeNameStr<int>();
    static constexpr auto ResRefName = Red::GetTypeNameStr<Red::ResRef>();

    std::vector<std::string> segments;
    segments.emplace_back(aRecordSpec->name);
    segments.emplace_back(ResRefName.data());
    segments.emplace_back(aPropSpec->functionName);
    segments.emplace_back(IntName.data());

    return fmt::format("{}", fmt::join(segments, ";")).c_str();
}

void GetResRefItemHandler::HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                                            const Context* aContext) const
{
    static const auto ResRefType = Red::TypeLocator<Red::GetTypeName<Red::ResRef>()>::Get();
    static const auto RaRefArrayType = reinterpret_cast<Red::CRTTIBaseArrayType*>(
        Red::TypeLocator<Red::GetTypeName<Red::DynArray<Red::RaRef<Red::CResource>>>()>::Get());

    int index;
    Red::GetParameter(aFrame, &index);

    ++aFrame->code;

    if (index < 0)
        return;

    const auto flat = aContext->tweakManager->GetFlat(GetFlatID(aInstance, aContext));

    if (flat.type != RaRefArrayType)
        return;

    if (const auto length = RaRefArrayType->GetLength(flat.instance); index >= static_cast<int>(length))
        return;

    ResRefType->Assign(aOut, RaRefArrayType->GetElement(flat.instance, index));
}

Red::CName GetResRefHandler::GetFunctionHash(const ScriptableRecordSpecPtr& aRecordSpec,
                                             const ScriptablePropertySpecPtr& aPropSpec) const
{
    static constexpr auto ResRefName = Red::GetTypeNameStr<Red::ResRef>();

    [[maybe_unused]] const auto meh = aPropSpec->typeSpec->propertyType->GetName().ToString();

    std::vector<std::string> segments;
    segments.emplace_back(aRecordSpec->name);
    segments.emplace_back(ResRefName.data());
    segments.emplace_back(aPropSpec->functionName);

    return fmt::format("{}", fmt::join(segments, ";")).c_str();
}

void GetResRefHandler::HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                                        const Context* aContext) const
{
    static const auto ResRefType = Red::TypeLocator<Red::GetTypeName<Red::ResRef>()>::Get();
    static const auto RaRefType = Red::TypeLocator<Red::GetTypeName<Red::RaRef<Red::CResource>>()>::Get();

    ++aFrame->code;

    if (!aOut)
        return;

    const auto flat = aContext->tweakManager->GetFlat(GetFlatID(aInstance, aContext));

    if (flat.type != RaRefType)
        return;

    ResRefType->Assign(aOut, flat.instance);
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

void GetValueHandler::HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                                       const Context* aContext) const
{
    ++aFrame->code;

    if (!aOut)
        return;

    const auto flat = aContext->tweakManager->GetFlat(GetFlatID(aInstance, aContext));

    if (flat.type != aContext->typeSpec->propertyType)
        return;

    flat.type->Assign(aOut, flat.instance);
}

ScriptablePropertyHandlers::ScriptablePropertyHandlers(const Core::DeferredPtr<Red::TweakDBManager>& aManager)
    : m_manager(aManager)
    , m_rtti(Red::CRTTISystem::Get())
{
}

void ScriptablePropertyHandlers::RegisterInvocationHandler()
{
    auto* function = Red::CGlobalFunction::Create(InvocationHandlerName, InvocationHandlerName, &HandleInvocation);
    m_rtti->RegisterFunction(function);
    m_invocationHandler = function;
}

void ScriptablePropertyHandlers::RegisterScriptableProperty(const ScriptableRecordSpecPtr& aRecordSpec,
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
    else if (aPropSpec->typeSpec->isArray && aPropSpec->typeSpec->isResRef)
    {
        RegisterPropertyFunction<GetterType::GetResRefArray>(aRecordSpec, aPropSpec);
        RegisterPropertyFunction<GetterType::GetArrayCount>(aRecordSpec, aPropSpec);
        RegisterPropertyFunction<GetterType::GetResRefItem>(aRecordSpec, aPropSpec);
    }
    else if (aPropSpec->typeSpec->isArray)
    {
        RegisterPropertyFunction<GetterType::Get>(aRecordSpec, aPropSpec);
        RegisterPropertyFunction<GetterType::GetArrayCount>(aRecordSpec, aPropSpec);
        RegisterPropertyFunction<GetterType::GetArrayItem>(aRecordSpec, aPropSpec);
        RegisterPropertyFunction<GetterType::ArrayContains>(aRecordSpec, aPropSpec);
    }
    else if (aPropSpec->typeSpec->isResRef)
    {
        RegisterPropertyFunction<GetterType::GetResRef>(aRecordSpec, aPropSpec);
    }
    else
    {
        RegisterPropertyFunction<GetterType::Get>(aRecordSpec, aPropSpec);
    }
}

bool ScriptablePropertyHandlers::AdaptScriptFunction(const ScriptableRecordSpecPtr& aRecordSpec,
                                                     Red::CClassFunction* aFunc)
{
    const auto functionHash = GetFunctionHash(aRecordSpec, aFunc);

    std::shared_lock lockR(m_functionTypesMutex);

    if (const auto it = m_functionTypes.find(aRecordSpec->cname); it != m_functionTypes.end())
    {
        if (const auto it2 = it->second.find(functionHash); it2 != it->second.end())
        {
            const auto getterType = it2->second;
            const auto handler = GetHandler(getterType);
            const auto baseFunctionName = handler->GetFunctionBaseName(aFunc->shortName.ToString());

            if (const auto propSpec = aRecordSpec->FindPropertyByFunctionName(baseFunctionName))
            {
                // TODO: truncate it here?
                if (!propSpec->isDescribed)
                    return false;

                const auto context = CreateContext(getterType, aRecordSpec, propSpec);
                ReplaceScriptFunction(aFunc, context);
                return true;
            }
        }
    }

    return false;
}

void ScriptablePropertyHandlers::HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                                                  int64_t a4)
{
    (void)a4;

    if (!aInstance || !aFrame)
        return;

    const auto context = GetContext(aFrame);

    GetHandler(context->getterType)->HandleInvocation(aInstance, aFrame, aOut, context);
}

Context* ScriptablePropertyHandlers::GetContext(Red::CStackFrame* aFrame)
{
    static constexpr auto PtrSize = sizeof(void*);

    auto* context = *reinterpret_cast<Context**>(aFrame->code);
    aFrame->code += PtrSize;
    return context;
}

ScriptablePropertyHandler* ScriptablePropertyHandlers::GetHandler(const GetterType aType)
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
    case GetterType::GetResRefArray: return &s_getResRefArrayHandler;
    case GetterType::GetResRefItem: return &s_getResRefItemHandler;
    case GetterType::GetResRef: return &s_getResRefHandler;
    case GetterType::Get: return &s_getValueHandler;
    default: return nullptr;
    }
    // clang-format on
}

ContextPtr ScriptablePropertyHandlers::CreateContext(const GetterType aGetterType,
                                                     const ScriptableRecordSpecPtr& aRecordSpec,
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
    context->getterType = aGetterType;
    context->typeSpec = aPropSpec->typeSpec;
    context->appendix = aPropSpec->appendix;

    {
        std::unique_lock lockRW(m_contextsMutex);
        m_contexts[aRecordSpec->cname][aPropSpec->cname] = context;
    }

    return context;
}

template<GetterType Type>
void ScriptablePropertyHandlers::RegisterPropertyFunction(const ScriptableRecordSpecPtr& aRecordSpec,
                                                          const ScriptablePropertySpecPtr& aPropSpec)
{
    const auto* handler = GetHandler(Type);
    const auto hash = handler->GetFunctionHash(aRecordSpec, aPropSpec);
    std::unique_lock lockRW(m_functionTypesMutex);
    m_functionTypes[aRecordSpec->cname][hash] = Type;
}

void ScriptablePropertyHandlers::ReplaceScriptFunction(Red::CClassFunction* aFunction, const ContextPtr& aContext) const
{
    const auto bytecode = CreateFunctionBytecode(aContext, aFunction);
    aFunction->bytecode.bytecode.buffer.data = bytecode.data;
    aFunction->bytecode.bytecode.buffer.size = bytecode.size;
}

void ScriptablePropertyHandlers::TruncateScriptFunction(Red::CClassFunction* aFunction)
{
    aFunction->bytecode.bytecode.buffer.data = nullptr;
    aFunction->bytecode.bytecode.buffer.size = 0;
}

Red::RawBuffer ScriptablePropertyHandlers::CreateFunctionBytecode(const ContextPtr& aContext,
                                                                  Red::CClassFunction* aFunction) const
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

    *reinterpret_cast<void**>(code) = m_invocationHandler;
    code += PointerSize;

    *reinterpret_cast<uint16_t*>(code) = 0;
    code += FlagsSize;

    *reinterpret_cast<const Context**>(code) = aContext.get();
    code += PointerSize;

    for (const auto& param : aFunction->params)
    {
        *code = ParamOp;
        code += OpSize;

        *reinterpret_cast<void**>(code) = param;
        code += PointerSize;
    }

    *code = ParamEndOp;

    return {memory, finalCodeSize};
}

Red::CName ScriptablePropertyHandlers::GetFunctionHash(const ScriptableRecordSpecPtr& aRecordSpec,
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
