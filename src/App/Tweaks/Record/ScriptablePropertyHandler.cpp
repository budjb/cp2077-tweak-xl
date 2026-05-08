#include "ScriptablePropertyHandler.hpp"
#include "ScriptableTweakDBRecord.hpp"

#include <spdlog/fmt/bundled/ranges.h>

namespace
{
const Red::CRTTIBaseArrayType* ToArrayType(const Red::rtti::IType* aType)
{
    if (aType->GetType() == Red::rtti::ERTTIType::Array)
    {
        return reinterpret_cast<const Red::CRTTIBaseArrayType*>(aType);
    }

    return nullptr;
}

const Red::rtti::IType* GetInnerType(const Red::rtti::IType* aType)
{
    if (const auto* arrayType = ToArrayType(aType))
        return arrayType->GetInnerType();
    return nullptr;
}

Red::CName GetInnerTypeName(const Red::rtti::IType* aType)
{
    if (const auto* type = GetInnerType(aType))
        return type->GetName();
    return {};
}
} // namespace

namespace App
{
std::string ScriptablePropertyGetter::GetFunctionBaseName(const std::string& aName) const
{
    return aName.substr(m_prefixLength, aName.length() - m_prefixLength - m_suffixLength);
}

#ifndef NDEBUG

std::string ScriptablePropertyGetter::GetFunctionName(const std::string& aName) const
{
    return std::string(m_prefix).append(aName).append(m_suffix);
}

void ScriptablePropertyGetter::NoOpFunction(Red::IScriptable*, Red::CStackFrame*, void*, int64_t)
{
}

#endif

Red::TweakDBID ScriptablePropertyGetter::GetFlatID(const Red::Instance aInstance, const Context* aContext)
{
    return static_cast<ScriptableTweakDBRecord*>(aInstance)->recordID + aContext->appendix;
}

template<template<typename> typename THandle>
    requires(std::is_same_v<THandle<Red::TweakDBRecord>, Red::Handle<Red::TweakDBRecord>> ||
             std::is_same_v<THandle<Red::TweakDBRecord>, Red::WeakHandle<Red::TweakDBRecord>>)
std::optional<Red::DynArray<THandle<Red::TweakDBRecord>>> ScriptablePropertyGetter::GetRecordArray(
    const Red::Value<>& aValue, const Context* aContext)
{
    if (!aContext->typeSpec->foreignType || !Red::TweakDBUtil::IsArrayType(aValue.type))
        return nullptr;

    const auto* flatArrayType = ToArrayType(aValue.type);

    if (!flatArrayType)
        return nullptr;

    const auto count = flatArrayType->GetLength(aValue.instance);
    auto outArray = Red::DynArray<THandle<Red::TweakDBRecord>>(count);

    for (uint32_t i = 0; i < count; ++i)
    {
        const auto recordID = *static_cast<Red::TweakDBID*>(flatArrayType->GetElement(aValue.instance, i));
        const auto record = aContext->tweakManager->GetRecord(recordID);

        if (record && record->GetType()->IsA(aContext->typeSpec->foreignType))
            outArray.At(i) = record;
        else
            outArray.At(i) = nullptr;
    }

    return std::move(outArray);
}

Red::CName RecordArrayGetter::GetFunctionHash(const ScriptableRecordSpecPtr& aRecordSpec,
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

void RecordArrayGetter::HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
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

#ifndef NDEBUG
void RecordArrayGetter::ConfigureScriptFunction(Red::CClassFunction* aFunction,
                                                const ScriptablePropertySpecPtr& aPropSpec) const
{
    aFunction->AddParam(aPropSpec->typeSpec->propertyTypeName, "outList", true);
}
#endif

Red::CName RecordArrayContainsGetter::GetFunctionHash(const ScriptableRecordSpecPtr& aRecordSpec,
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

void RecordArrayContainsGetter::HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
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

#ifndef NDEBUG
void RecordArrayContainsGetter::ConfigureScriptFunction(Red::CClassFunction* aFunction,
                                                        const ScriptablePropertySpecPtr& aPropSpec) const
{
    aFunction->AddParam(Red::TweakDBUtil::GetWHandleTypeName<Red::CName>(aPropSpec->typeSpec->foreignType), "item");
    aFunction->SetReturnType(Red::GetTypeName<bool>());
}
#endif

Red::CName RecordItemGetter::GetFunctionHash(const ScriptableRecordSpecPtr& aRecordSpec,
                                             const ScriptablePropertySpecPtr& aPropSpec) const
{
    static constexpr auto IntName = Red::GetTypeNameStr<int>();

    std::string funcName = "Get" + aPropSpec->functionName + "Item";

    std::vector<std::string> segments;
    segments.emplace_back(aRecordSpec->name);
    segments.emplace_back(
        Red::TweakDBUtil::GetWHandleTypeName<Red::CName>(aPropSpec->typeSpec->foreignType).ToString());
    segments.emplace_back(funcName);
    segments.emplace_back(IntName.data());

    return fmt::format("{}", fmt::join(segments, ";")).c_str();
}

void RecordItemGetter::HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
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

#ifndef NDEBUG
void RecordItemGetter::ConfigureScriptFunction(Red::CClassFunction* aFunction,
                                               const ScriptablePropertySpecPtr& aPropSpec) const
{
    aFunction->AddParam(Red::GetTypeName<int>(), "index");
    aFunction->SetReturnType(Red::TweakDBUtil::GetWHandleTypeName<Red::CName>(aPropSpec->typeSpec->foreignType));
}
#endif

Red::CName RecordItemHandleGetter::GetFunctionHash(const ScriptableRecordSpecPtr& aRecordSpec,
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

void RecordItemHandleGetter::HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
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

#ifndef NDEBUG
void RecordItemHandleGetter::ConfigureScriptFunction(Red::CClassFunction* aFunction,
                                                     const ScriptablePropertySpecPtr& aPropSpec) const
{
    aFunction->AddParam(Red::GetTypeName<int>(), "index");
    aFunction->SetReturnType(Red::TweakDBUtil::GetHandleTypeName<Red::CName>(aPropSpec->typeSpec->foreignType));
}
#endif

Red::CName RecordGetter::GetFunctionHash(const ScriptableRecordSpecPtr& aRecordSpec,
                                         const ScriptablePropertySpecPtr& aPropSpec) const
{
    std::vector<std::string> segments;
    segments.emplace_back(aRecordSpec->name);
    segments.emplace_back(Red::TweakDBUtil::GetWHandleType(aPropSpec->typeSpec->foreignType)->GetName().ToString());
    segments.emplace_back(aPropSpec->functionName);

    return fmt::format("{}", fmt::join(segments, ";")).c_str();
}

void RecordGetter::HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
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

#ifndef NDEBUG
void RecordGetter::ConfigureScriptFunction(Red::CClassFunction* aFunction,
                                           const ScriptablePropertySpecPtr& aPropSpec) const
{
    aFunction->SetReturnType(Red::TweakDBUtil::GetWHandleTypeName<Red::CName>(aPropSpec->typeSpec->foreignType));
}
#endif

Red::CName RecordHandleGetter::GetFunctionHash(const ScriptableRecordSpecPtr& aRecordSpec,
                                               const ScriptablePropertySpecPtr& aPropSpec) const
{
    std::string funcName = aPropSpec->functionName + "Handle";

    std::vector<std::string> segments;
    segments.emplace_back(aRecordSpec->name);
    segments.emplace_back(Red::TweakDBUtil::GetHandleType(aPropSpec->typeSpec->foreignType)->GetName().ToString());
    segments.emplace_back(funcName);

    return fmt::format("{}", fmt::join(segments, ";")).c_str();
}

void RecordHandleGetter::HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
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

#ifndef NDEBUG
void RecordHandleGetter::ConfigureScriptFunction(Red::CClassFunction* aFunction,
                                                 const ScriptablePropertySpecPtr& aPropSpec) const
{
    aFunction->SetReturnType(Red::TweakDBUtil::GetHandleTypeName<Red::CName>(aPropSpec->typeSpec->foreignType));
}
#endif

Red::CName ArrayCountGetter::GetFunctionHash(const ScriptableRecordSpecPtr& aRecordSpec,
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

void ArrayCountGetter::HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                                        const Context* aContext) const
{
    ++aFrame->code;

    if (!aOut)
        return;

    if (const auto flat = aContext->tweakManager->GetFlat(GetFlatID(aInstance, aContext));
        flat.type->GetType() == Red::rtti::ERTTIType::Array)
    {
        const auto* arrayType = ToArrayType(flat.type);
        *static_cast<uint32_t*>(aOut) = arrayType->GetLength(flat.instance);
    }
    else
    {
        *static_cast<int*>(aOut) = 0;
    }
}

#ifndef NDEBUG
void ArrayCountGetter::ConfigureScriptFunction(Red::CClassFunction* aFunction,
                                               const ScriptablePropertySpecPtr& aPropSpec) const
{
    aFunction->SetReturnType(Red::GetTypeName<int>());
}
#endif

Red::CName ArrayItemGetter::GetFunctionHash(const ScriptableRecordSpecPtr& aRecordSpec,
                                            const ScriptablePropertySpecPtr& aPropSpec) const
{
    static constexpr auto IntName = Red::GetTypeNameStr<int>();

    std::string funcName = "Get" + aPropSpec->functionName + "Item";

    std::vector<std::string> segments;
    segments.emplace_back(aRecordSpec->name);
    segments.emplace_back(GetInnerTypeName(aPropSpec->typeSpec->propertyType).ToString());
    segments.emplace_back(funcName);
    segments.emplace_back(IntName.data());

    return fmt::format("{}", fmt::join(segments, ";")).c_str();
}

void ArrayItemGetter::HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
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

    const auto* arrayType = ToArrayType(flat.type);
    const auto* innerType = arrayType->GetInnerType();
    const auto length = arrayType->GetLength(flat.instance);

    if (index >= 0 && static_cast<uint32_t>(index) < length)
    {
        innerType->Assign(aOut, arrayType->GetElement(flat.instance, index));
    }
}

#ifndef NDEBUG
void ArrayItemGetter::ConfigureScriptFunction(Red::CClassFunction* aFunction,
                                              const ScriptablePropertySpecPtr& aPropSpec) const
{
    aFunction->AddParam(Red::GetTypeName<int>(), "index");
    aFunction->SetReturnType(GetInnerTypeName(aPropSpec->typeSpec->propertyType));
}
#endif

Red::CName ArrayContainsGetter::GetFunctionHash(const ScriptableRecordSpecPtr& aRecordSpec,
                                                const ScriptablePropertySpecPtr& aPropSpec) const
{
    static constexpr auto BoolName = Red::GetTypeNameStr<bool>();

    std::string funcName = aPropSpec->functionName + "Contains";

    std::vector<std::string> segments;
    segments.emplace_back(aRecordSpec->name);
    segments.emplace_back(BoolName.data());
    segments.emplace_back(funcName);
    segments.emplace_back(GetInnerTypeName(aPropSpec->typeSpec->propertyType).ToString());

    return fmt::format("{}", fmt::join(segments, ";")).c_str();
}

void ArrayContainsGetter::HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
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

    const auto* arrayType = ToArrayType(flat.type);
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

#ifndef NDEBUG
void ArrayContainsGetter::ConfigureScriptFunction(Red::CClassFunction* aFunction,
                                                  const ScriptablePropertySpecPtr& aPropSpec) const
{
    aFunction->AddParam(GetInnerTypeName(aPropSpec->typeSpec->propertyType), "item");
    aFunction->SetReturnType(Red::GetTypeName<bool>());
}
#endif

Red::CName ResRefArrayGetter::GetFunctionHash(const ScriptableRecordSpecPtr& aRecordSpec,
                                              const ScriptablePropertySpecPtr& aPropSpec) const
{
    static constexpr auto ResRefArrayName = Red::GetTypeNameStr<Red::DynArray<Red::ResRef>>();

    std::vector<std::string> segments;
    segments.emplace_back(aRecordSpec->name);
    segments.emplace_back(ResRefArrayName.data());
    segments.emplace_back(aPropSpec->functionName);

    return fmt::format("{}", fmt::join(segments, ";")).c_str();
}

void ResRefArrayGetter::HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                                         const Context* aContext) const
{
    static const auto ResRefType = Red::TypeLocator<Red::GetTypeName<Red::ResRef>()>::Get();
    static const auto RaRefArrayType =
        ToArrayType(Red::TypeLocator<Red::GetTypeName<Red::DynArray<Red::RaRef<Red::CResource>>>()>::Get());

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
        ResRefType->Assign(&result.At(i), RaRefArrayType->GetElement(flat.instance, i));
    }

    *static_cast<Red::DynArray<Red::ResRef>*>(aOut) = result;
}

#ifndef NDEBUG
void ResRefArrayGetter::ConfigureScriptFunction(Red::CClassFunction* aFunction,
                                                const ScriptablePropertySpecPtr& aPropSpec) const
{
    aFunction->SetReturnType(Red::GetTypeName<Red::DynArray<Red::ResRef>>());
}
#endif

Red::CName ResRefItemGetter::GetFunctionHash(const ScriptableRecordSpecPtr& aRecordSpec,
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

void ResRefItemGetter::HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                                        const Context* aContext) const
{
    static const auto ResRefType = Red::TypeLocator<Red::GetTypeName<Red::ResRef>()>::Get();
    static const auto RaRefArrayType =
        ToArrayType(Red::TypeLocator<Red::GetTypeName<Red::DynArray<Red::RaRef<Red::CResource>>>()>::Get());

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

#ifndef NDEBUG
void ResRefItemGetter::ConfigureScriptFunction(Red::CClassFunction* aFunction,
                                               const ScriptablePropertySpecPtr& aPropSpec) const
{
    aFunction->AddParam(Red::GetTypeName<int>(), "index");
    aFunction->SetReturnType(Red::GetTypeName<Red::ResRef>());
}
#endif

Red::CName ResRefGetter::GetFunctionHash(const ScriptableRecordSpecPtr& aRecordSpec,
                                         const ScriptablePropertySpecPtr& aPropSpec) const
{
    static constexpr auto ResRefName = Red::GetTypeNameStr<Red::ResRef>();

    std::vector<std::string> segments;
    segments.emplace_back(aRecordSpec->name);
    segments.emplace_back(ResRefName.data());
    segments.emplace_back(aPropSpec->functionName);

    return fmt::format("{}", fmt::join(segments, ";")).c_str();
}

void ResRefGetter::HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
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

#ifndef NDEBUG
void ResRefGetter::ConfigureScriptFunction(Red::CClassFunction* aFunction,
                                           const ScriptablePropertySpecPtr& aPropSpec) const
{
    aFunction->SetReturnType(Red::GetTypeName<Red::ResRef>());
}
#endif

Red::CName ValueGetter::GetFunctionHash(const ScriptableRecordSpecPtr& aRecordSpec,
                                        const ScriptablePropertySpecPtr& aPropSpec) const
{
    std::vector<std::string> segments;
    segments.emplace_back(aRecordSpec->name);
    segments.emplace_back(aPropSpec->typeSpec->propertyType->GetName().ToString());
    segments.emplace_back(aPropSpec->functionName);

    return fmt::format("{}", fmt::join(segments, ";")).c_str();
}

void ValueGetter::HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
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

#ifndef NDEBUG
void ValueGetter::ConfigureScriptFunction(Red::CClassFunction* aFunction,
                                          const ScriptablePropertySpecPtr& aPropSpec) const
{
    aFunction->SetReturnType(aPropSpec->typeSpec->propertyTypeName);
}
#endif

ScriptablePropertyHandler::ScriptablePropertyHandler(const Core::DeferredPtr<Red::TweakDBManager>& aManager)
    : m_manager(aManager)
    , m_rtti(Red::CRTTISystem::Get())
{
}

void ScriptablePropertyHandler::RegisterInvocationHandler()
{
    auto* function = Red::CGlobalFunction::Create(InvocationHandlerName, InvocationHandlerName, &HandleInvocation);
    m_rtti->RegisterFunction(function);
    m_invocationHandler = function;
}

void ScriptablePropertyHandler::RegisterScriptableProperty(const ScriptableRecordSpecPtr& aRecordSpec,
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

bool ScriptablePropertyHandler::AdaptScriptFunction(const ScriptableRecordSpecPtr& aRecordSpec,
                                                    Red::CClassFunction* aFunc)
{
    const auto functionHash = GetFunctionHash(aRecordSpec, aFunc);

    std::shared_lock lockR(m_functionTypesMutex);

    if (const auto it = m_functionTypes.find(aRecordSpec->cname); it != m_functionTypes.end())
    {
        if (const auto it2 = it->second.find(functionHash); it2 != it->second.end())
        {
            const auto getterType = it2->second;
            const auto handler = GetPropertyGetter(getterType);
            const auto baseFunctionName = handler->GetFunctionBaseName(aFunc->shortName.ToString());

            if (const auto propSpec = aRecordSpec->FindPropertyByFunctionName(baseFunctionName))
            {
                // TODO: truncate it here?
                if (!propSpec->isDescribed)
                    return false;

                const auto context = CreateContext(aRecordSpec, propSpec);
                ReplaceScriptFunction(aFunc, getterType, context);
                return true;
            }
        }
    }

    return false;
}

#ifndef NDEBUG

void ScriptablePropertyHandler::CreateScriptFunctions(const ScriptableRecordSpecPtr& aRecordSpec)
{
    if (!aRecordSpec->type)
        return;

    for (const auto propSpec : aRecordSpec->props | std::views::values)
    {
        if (propSpec->typeSpec->isArray && propSpec->typeSpec->isForeignKey)
        {
            CreatePropertyFunction<GetterType::GetRecordArray>(aRecordSpec, propSpec);
            CreatePropertyFunction<GetterType::GetArrayCount>(aRecordSpec, propSpec);
            CreatePropertyFunction<GetterType::GetRecordItem>(aRecordSpec, propSpec);
            CreatePropertyFunction<GetterType::GetRecordItemHandle>(aRecordSpec, propSpec);
            CreatePropertyFunction<GetterType::RecordArrayContains>(aRecordSpec, propSpec);
        }
        else if (!propSpec->typeSpec->isArray && propSpec->typeSpec->isForeignKey)
        {
            CreatePropertyFunction<GetterType::GetRecord>(aRecordSpec, propSpec);
            CreatePropertyFunction<GetterType::GetRecordHandle>(aRecordSpec, propSpec);
        }
        else if (propSpec->typeSpec->isArray && propSpec->typeSpec->isResRef)
        {
            CreatePropertyFunction<GetterType::GetResRefArray>(aRecordSpec, propSpec);
            CreatePropertyFunction<GetterType::GetArrayCount>(aRecordSpec, propSpec);
            CreatePropertyFunction<GetterType::GetResRefItem>(aRecordSpec, propSpec);
        }
        else if (propSpec->typeSpec->isArray)
        {
            CreatePropertyFunction<GetterType::Get>(aRecordSpec, propSpec);
            CreatePropertyFunction<GetterType::GetArrayCount>(aRecordSpec, propSpec);
            CreatePropertyFunction<GetterType::GetArrayItem>(aRecordSpec, propSpec);
            CreatePropertyFunction<GetterType::ArrayContains>(aRecordSpec, propSpec);
        }
        else if (propSpec->typeSpec->isResRef)
        {
            CreatePropertyFunction<GetterType::GetResRef>(aRecordSpec, propSpec);
        }
        else
        {
            CreatePropertyFunction<GetterType::Get>(aRecordSpec, propSpec);
        }
    }
}

template<GetterType Type>
void ScriptablePropertyHandler::CreatePropertyFunction(const ScriptableRecordSpecPtr& aRecordSpec,
                                                       const ScriptablePropertySpecPtr& aPropSpec)
{
    RegisterPropertyFunction<Type>(aRecordSpec, aPropSpec);
    const auto* handler = GetPropertyGetter(Type);

    const auto name = handler->GetFunctionName(aPropSpec->functionName);
    auto* function = Red::CClassFunction::Create(aRecordSpec->type, name.c_str(), name.c_str(), &HandleInvocation);
    handler->ConfigureScriptFunction(function, aPropSpec);
    aRecordSpec->type->RegisterFunction(function);

    function->flags.isNative = false;
    Red::MarkSpecial(function);

    const auto context = CreateContext(aRecordSpec, aPropSpec);
    const auto bytecode = CreateFunctionBytecode(Type, context, function);

    function->bytecode.bytecode.buffer.data = bytecode.data;
    function->bytecode.bytecode.buffer.size = bytecode.size;
}

#endif

void ScriptablePropertyHandler::HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                                                 int64_t)
{
    if (!aInstance || !aFrame)
        return;

    const auto context = GetContext(aFrame);
    const auto getterType = GetGetterType(aFrame);

    GetPropertyGetter(getterType)->HandleInvocation(aInstance, aFrame, aOut, context);
}

Context* ScriptablePropertyHandler::GetContext(Red::CStackFrame* aFrame)
{
    static constexpr auto PtrSize = sizeof(void*);

    auto* context = *reinterpret_cast<Context**>(aFrame->code);
    aFrame->code += PtrSize;
    return context;
}

GetterType ScriptablePropertyHandler::GetGetterType(Red::CStackFrame* aFrame)
{
    static constexpr auto TypeSize = sizeof(GetterType);
    const auto type = *reinterpret_cast<GetterType*>(aFrame->code);
    aFrame->code += TypeSize;
    return type;
}

ScriptablePropertyGetter* ScriptablePropertyHandler::GetPropertyGetter(const GetterType aType)
{
    // clang-format off
    switch (aType)
    {
    case GetterType::GetRecordArray: return &s_recordArrayGetter;
    case GetterType::RecordArrayContains: return &s_recordArrayContainsGetter;
    case GetterType::GetRecordItem: return &s_recordItemGetter;
    case GetterType::GetRecordItemHandle: return &s_recordItemHandleGetter;
    case GetterType::GetRecord: return &s_recordGetter;
    case GetterType::GetRecordHandle: return &s_recordHandleGetter;
    case GetterType::GetArrayCount: return &s_arrayCountGetter;
    case GetterType::GetArrayItem: return &s_arrayItemGetter;
    case GetterType::ArrayContains: return &s_arrayContainsGetter;
    case GetterType::GetResRefArray: return &s_resRefArrayGetter;
    case GetterType::GetResRefItem: return &s_resRefItemGetter;
    case GetterType::GetResRef: return &s_resRefGetter;
    case GetterType::Get: return &s_valueGetter;
    default: return nullptr;
    }
    // clang-format on
}

ContextPtr ScriptablePropertyHandler::CreateContext(const ScriptableRecordSpecPtr& aRecordSpec,
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
void ScriptablePropertyHandler::RegisterPropertyFunction(const ScriptableRecordSpecPtr& aRecordSpec,
                                                         const ScriptablePropertySpecPtr& aPropSpec)
{
    const auto* handler = GetPropertyGetter(Type);
    const auto hash = handler->GetFunctionHash(aRecordSpec, aPropSpec);
    std::unique_lock lockRW(m_functionTypesMutex);
    m_functionTypes[aRecordSpec->cname][hash] = Type;
}

void ScriptablePropertyHandler::ReplaceScriptFunction(Red::CClassFunction* aFunction, const GetterType aGetterType,
                                                      const ContextPtr& aContext) const
{
    const auto bytecode = CreateFunctionBytecode(aGetterType, aContext, aFunction);
    aFunction->bytecode.bytecode.buffer.data = bytecode.data;
    aFunction->bytecode.bytecode.buffer.size = bytecode.size;
}

void ScriptablePropertyHandler::TruncateScriptFunction(Red::CClassFunction* aFunction)
{
    aFunction->bytecode.bytecode.buffer.data = nullptr;
    aFunction->bytecode.bytecode.buffer.size = 0;
}

Red::RawBuffer ScriptablePropertyHandler::CreateFunctionBytecode(const GetterType aGetterType,
                                                                 const ContextPtr& aContext,
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
    constexpr uint32_t GetterTypeSize = sizeof(GetterType);
    constexpr uint32_t BaseCodeSize = OpSize + OffsetSize * 2 + PointerSize + FlagsSize + OpSize;
    constexpr uint16_t BaseExitOffset = BaseCodeSize - OpSize - OffsetSize;

    const uint32_t extraCodeSize =
        aFunction->params.Size() * (OpSize + PointerSize) + PointerSize + GetterTypeSize + (aFunction->returnType ? 1 : 0);
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

    *reinterpret_cast<GetterType*>(code) = aGetterType;
    code += GetterTypeSize;

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

Red::CName ScriptablePropertyHandler::GetFunctionHash(const ScriptableRecordSpecPtr& aRecordSpec,
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
