#include "ScriptableRecordManager.hpp"
#include "ScriptableRecordClass.hpp"
#include "ScriptableTweakDBRecord.hpp"

namespace
{
constexpr auto arrayPrefix = Red::GetTypePrefixStr<Red::DynArray>();
constexpr auto whandlePrefix = Red::GetTypePrefixStr<Red::WeakHandle>();
constexpr auto handlePrefix = Red::GetTypePrefixStr<Red::Handle>();

Red::TweakDBID GetFlatID(Red::Instance aInstance, const App::ScriptableRecordManager::Context* aContext)
{
    if (!aInstance || !aContext)
        return {};

    const auto* record = static_cast<App::ScriptableTweakDBRecord*>(aInstance);
    return record->recordID + aContext->appendix;
}

Red::rtti::IType* GetHandleType(const Red::CClass* aClass)
{
    std::string name = handlePrefix.data();
    name.append(aClass->GetName().ToString());
    return Red::CRTTISystem::Get()->GetType(Red::CNamePool::Add(name.c_str()));
}

Red::rtti::IType* GetWHandleType(const Red::CClass* aClass)
{
    std::string name = whandlePrefix.data();
    name.append(aClass->GetName().ToString());
    return Red::CRTTISystem::Get()->GetType(Red::CNamePool::Add(name.c_str()));
}

Red::rtti::IType* GetWHandleArrayType(const Red::CClass* aClass)
{
    std::string name = arrayPrefix.data();
    name.append(whandlePrefix.data());
    name.append(aClass->GetName().ToString());
    return Red::CRTTISystem::Get()->GetType(Red::CNamePool::Add(name.c_str()));
}

} // namespace

namespace App::ScriptableRecordGetters
{
constexpr uint32_t OpSize = sizeof(char);
constexpr uint32_t PtrSize = sizeof(void*);

using RecordHandle = ScriptableRecordManager::RecordHandle;
using RecordWHandle = ScriptableRecordManager::RecordWHandle;
using RecordArray = ScriptableRecordManager::RecordArray;
using RecordArrayPtr = ScriptableRecordManager::RecordArrayPtr;

using Context = ScriptableRecordManager::Context;

RecordArrayPtr GetRecordArray(const Red::Value<>& aValue, const Context* aContext)
{
    static const auto* tweakDBIDArrayType = Red::TypeLocator<Red::ERTDBFlatType::TweakDBID>::GetArray();

    const auto propSpec = aContext->propSpec;

    if (!propSpec->isForeignKey || !propSpec->isArray || aValue.type != propSpec->flatType)
        return nullptr;

    const auto* targetArrayType = reinterpret_cast<const Red::CRTTIBaseArrayType*>(propSpec->propertyType);
    const auto* innerType = targetArrayType->GetInnerType();

    if (!innerType || innerType->GetType() != Red::rtti::ERTTIType::WeakHandle)
        return nullptr;

    const auto length = tweakDBIDArrayType->GetLength(aValue.instance);
    const auto result = Red::MakeInstance<ScriptableRecordManager::RecordArray>(length);

    for (uint32_t i = 0; i < length; ++i)
    {
        auto& element = result->At(i);
        const auto& id = *static_cast<Red::TweakDBID*>(tweakDBIDArrayType->GetElement(aValue.instance, i));
        const auto record = aContext->tweakManager->GetRecord(id);
        const auto instance = targetArrayType->GetElement(&element, i);

        if (!record || !record->GetType()->IsA(propSpec->foreignType))
        {
            if (innerType->GetType() == Red::rtti::ERTTIType::WeakHandle)
            {
                *static_cast<Red::WeakHandle<Red::TweakDBRecord>*>(instance) = Red::WeakHandle<Red::TweakDBRecord>{};
            }
            else
            {
                *static_cast<Red::Handle<Red::TweakDBRecord>*>(instance) = nullptr;
            }
        }
        else
        {
            if (innerType->GetType() == Red::rtti::ERTTIType::WeakHandle)
            {
                *static_cast<Red::WeakHandle<Red::TweakDBRecord>*>(instance) = record;
            }
            else
            {
                *static_cast<Red::Handle<Red::TweakDBRecord>*>(instance) = record;
            }
        }
    }

    return result;
}

RecordHandle GetRecordItemHandle(const Red::Value<>& aValue, const Context* aContext, const int aIndex)
{
    static const auto* tweakDBIDArrayType = Red::TypeLocator<Red::ERTDBFlatType::TweakDBID>::GetArray();

    const auto propSpec = aContext->propSpec;

    if (!propSpec->isForeignKey || !propSpec->isArray ||
        propSpec->propertyType->GetType() != Red::rtti::ERTTIType::Array)
        return nullptr;

    const auto* targetArrayType = reinterpret_cast<const Red::CRTTIBaseArrayType*>(propSpec->propertyType);
    const auto* innerType = targetArrayType->GetInnerType();

    if (!innerType || innerType->GetType() != Red::rtti::ERTTIType::Handle)
        return nullptr;

    const auto length = tweakDBIDArrayType->GetLength(aValue.instance);

    if (aIndex < 0 || aIndex >= length)
        return {};

    const auto& id = *static_cast<Red::TweakDBID*>(targetArrayType->GetElement(aValue.instance, aIndex));
    auto record = aContext->tweakManager->GetRecord(id);

    if (record && record->GetType()->IsA(propSpec->foreignType))
        return record;

    return nullptr;
}

RecordWHandle GetRecordItem(const Red::Value<>& aValue, const Context* aContext, const int aIndex)
{
    return GetRecordItemHandle(aValue, aContext, aIndex);
}

bool RecordArrayContains(const Red::Value<>& aValue, const Context* aContext, const RecordWHandle& aRecord)
{
    static auto* arrayType =
        reinterpret_cast<Red::CRTTIBaseArrayType*>(Red::TypeLocator<Red::ERTDBFlatType::TweakDBIDArray>::Get());
    static auto* innerType = Red::TypeLocator<Red::ERTDBFlatType::TweakDBID>::Get();

    const auto propSpec = aContext->propSpec;

    if (propSpec->flatType != arrayType)
        return false;

    const auto length = arrayType->GetLength(aValue.instance);

    for (uint32_t i = 0; i < length; ++i)
    {
        if (innerType->IsEqual(arrayType->GetElement(aValue.instance, i), &aRecord.instance->recordID))
            return true;
    }

    return false;
}

RecordWHandle GetRecord(const Red::Value<>& aValue, const Context* aContext)
{
    const auto propSpec = aContext->propSpec;

    if (!propSpec->isForeignKey || propSpec->propertyType->GetType() != Red::rtti::ERTTIType::WeakHandle)
        return {};

    const auto& id = *static_cast<Red::TweakDBID*>(aValue.instance);
    auto record = aContext->tweakManager->GetRecord(id);

    if (record && record->GetType()->IsA(propSpec->foreignType))
        return record;

    return {};
}

RecordHandle GetRecordHandle(const Red::Value<>& aValue, const Context* aContext)
{
    const auto propSpec = aContext->propSpec;

    if (!propSpec->isForeignKey || propSpec->propertyType->GetType() != Red::rtti::ERTTIType::Handle)
        return {};

    const auto& id = *static_cast<Red::TweakDBID*>(aValue.instance);
    auto record = aContext->tweakManager->GetRecord(id);

    if (record && record->GetType()->IsA(propSpec->foreignType))
        return record;

    return {};
}

uint32_t GetArrayCount(const Red::Value<>& aValue, const Context* aContext)
{
    const auto propSpec = aContext->propSpec;

    if (propSpec->propertyType != aValue.type || aValue.type->GetType() != Red::rtti::ERTTIType::Array)
        return 0;

    const auto* targetArrayType = reinterpret_cast<const Red::CRTTIBaseArrayType*>(aValue.type);
    return targetArrayType->GetLength(aValue.instance);
}

bool ArrayContains(const Red::Value<>& aValue, const Context* aContext, Red::Instance item)
{
    const auto propSpec = aContext->propSpec;

    if (propSpec->propertyType->GetType() != Red::rtti::ERTTIType::Array || aValue.type != propSpec->propertyType)
        return false;

    auto* arrayType =
        const_cast<Red::CRTTIBaseArrayType*>(reinterpret_cast<const Red::CRTTIBaseArrayType*>(propSpec->propertyType));
    auto* innerType = arrayType->GetInnerType();

    const auto length = arrayType->GetLength(aValue.instance);

    for (uint32_t i = 0; i < length; ++i)
    {
        if (innerType->IsEqual(arrayType->GetElement(aValue.instance, i), item))
            return true;
    }

    return false;
}

const Context* GetContext(Red::CStackFrame* aFrame)
{
    const auto* context = *reinterpret_cast<Context**>(aFrame->code);
    aFrame->code += sizeof(Context*); // Move past ctx pointer
    return context;
}

void HandleGetRecordArray(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut, int64_t a4)
{
    Red::DynArray<Red::WeakHandle<Red::TweakDBRecord>>* outArray;
    Red::GetParameter(aFrame, &outArray);

    aFrame->code++; // Skip ParamEnd operand

    const auto* context = GetContext(aFrame);

    if (!aOut || !context)
        return;

    const auto flat = context->tweakManager->GetFlat(GetFlatID(aInstance, context));

    *static_cast<RecordArray*>(aOut) = *GetRecordArray(flat, context);
}

void HandleGetArrayCount(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut, int64_t a4)
{
    aFrame->code++; // Skip ParamEnd operand

    const auto* context = GetContext(aFrame);

    if (!aOut || !context)
        return;

    const auto flat = context->tweakManager->GetFlat(GetFlatID(aInstance, context));

    *static_cast<int*>(aOut) = static_cast<int>(GetArrayCount(flat, context));
}

void HandleGetRecordItem(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut, int64_t a4)
{
    int index;
    Red::GetParameter(aFrame, &index);

    aFrame->code++; // Skip ParamEnd operand

    const auto* context = GetContext(aFrame);

    if (!aOut || !context)
        return;

    const auto flat = context->tweakManager->GetFlat(GetFlatID(aInstance, context));

    *static_cast<ScriptableRecordManager::RecordWHandle*>(aOut) = GetRecordItem(flat, context, index);
}

void HandleGetRecordItemHandle(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut, int64_t a4)
{
    int index;
    Red::GetParameter(aFrame, &index);

    aFrame->code++; // Skip ParamEnd operand

    const auto* context = GetContext(aFrame);

    if (!aOut || !context)
        return;

    const auto flat = context->tweakManager->GetFlat(GetFlatID(aInstance, context));

    *static_cast<ScriptableRecordManager::RecordHandle*>(aOut) = GetRecordItemHandle(flat, context, index);
}

void HandleRecordArrayContains(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut, int64_t a4)
{
    ScriptableRecordManager::RecordWHandle record;
    Red::GetParameter(aFrame, &record);

    aFrame->code++; // Skip ParamEnd operand

    const auto* context = GetContext(aFrame);

    if (!aOut || !context)
        return;

    const auto flat = context->tweakManager->GetFlat(GetFlatID(aInstance, context));

    *static_cast<bool*>(aOut) = RecordArrayContains(flat, context, record);
}

void HandleGetRecord(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut, int64_t a4)
{
    aFrame->code++; // Skip ParamEnd operand

    const auto* context = GetContext(aFrame);

    if (!aOut || !context)
        return;

    const auto flat = context->tweakManager->GetFlat(GetFlatID(aInstance, context));

    *static_cast<ScriptableRecordManager::RecordWHandle*>(aOut) = GetRecord(flat, context);
}

void HandleGetRecordHandle(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut, int64_t a4)
{
    aFrame->code++; // Skip ParamEnd operand

    const auto* context = GetContext(aFrame);

    if (!aOut || !context)
        return;

    const auto flat = context->tweakManager->GetFlat(GetFlatID(aInstance, context));

    *static_cast<ScriptableRecordManager::RecordHandle*>(aOut) = GetRecordHandle(flat, context);
}

void HandleGetArrayItem(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut, int64_t a4)
{
    int index;
    Red::GetParameter(aFrame, &index);

    aFrame->code++; // Skip ParamEnd operand

    const auto* context = GetContext(aFrame);

    if (!aOut || !context)
        return;

    const auto flat = context->tweakManager->GetFlat(GetFlatID(aInstance, context));

    if (flat.type != context->propSpec->flatType || flat.type->GetType() != Red::rtti::ERTTIType::Array)
        return;

    const auto* arrayType = reinterpret_cast<const Red::CRTTIBaseArrayType*>(flat.type);
    const auto* innerType = arrayType->GetInnerType();
    const auto length = arrayType->GetLength(flat.instance);

    if (index < 0 || index >= length)
        return;

    innerType->Assign(aOut, arrayType->GetElement(flat.instance, index));
}

void HandleArrayContains(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut, int64_t a4)
{
    static constexpr uint32_t ContextOffset = OpSize + PtrSize + 1;
    static constexpr uint32_t EndOffset = ContextOffset + PtrSize;

    aFrame->code += ContextOffset;

    const auto* context = GetContext(aFrame);

    if (!aOut || !context)
        return;

    aFrame->code -= EndOffset;

    const auto* arrayType = reinterpret_cast<const Red::CRTTIBaseArrayType*>(context->propSpec->propertyType);
    const auto* innerType = arrayType->GetInnerType();

    const auto item = Red::MakeValue(innerType);
    Red::GetParameter(aFrame, item->instance);

    aFrame->code += EndOffset;

    const auto flat = context->tweakManager->GetFlat(GetFlatID(aInstance, context));

    *static_cast<bool*>(aOut) = ArrayContains(flat, context, item->instance);
}

void HandleGet(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut, int64_t a4)
{
    aFrame->code++;

    const auto* context = GetContext(aFrame);

    if (!aOut || !context)
        return;

    const auto flat = context->tweakManager->GetFlat(GetFlatID(aInstance, context));

    if (flat.type == context->propSpec->propertyType)
    {
        flat.type->Assign(aOut, flat.instance);
    }
}

} // namespace App::ScriptableRecordGetters

namespace App
{
void ScriptableRecordManager::CreateGetterFunctions(ScriptableRecordClass* aClass,
                                                    const ScriptablePropertySpecPtr& aSpec)
{
    const auto context = Core::MakeShared<Context>();

    context->appendix = aSpec->appendix;
    context->propSpec = aSpec->typeSpec;
    context->recordManager = ToShared();
    context->tweakManager = m_tweakManager;

    {
        std::unique_lock lockRW(m_contextsMutex);
        m_contexts[aClass->name][aSpec->cname] = context;
    }

    const auto typeSpec = aSpec->typeSpec;
    const auto name = Red::TweakDBUtil::Capitalize(aSpec->name);

    if (typeSpec->isArray && typeSpec->isForeignKey)
    {
        CreateGetRecords(aClass, name, context);
        CreateGetArrayCount(aClass, name, context);
        CreateGetRecordItem(aClass, name, context);
        CreateGetRecordItemHandle(aClass, name, context);
        CreateRecordArrayContains(aClass, name, context);
    }
    else if (typeSpec->isForeignKey)
    {
        CreateGetRecord(aClass, name, context);
        CreateGetRecordHandle(aClass, name, context);
    }
    else if (typeSpec->isArray && Red::TweakDBUtil::IsResRefTokenArray(typeSpec->propertyTypeName))
    {
        CreateGet(aClass, name, context);
        CreateGetArrayCount(aClass, name, context);
        CreateGetArrayItem(aClass, name, context);
    }
    else if (typeSpec->isArray)
    {
        CreateGet(aClass, name, context);
        CreateGetArrayCount(aClass, name, context);
        CreateGetArrayItem(aClass, name, context);
        CreateArrayContains(aClass, name, context);
    }
    else
    {
        CreateGet(aClass, name, context);
    }
}

void ScriptableRecordManager::CreateGetRecords(ScriptableRecordClass* aClass, const std::string& aName,
                                               const ContextPtr& aContext)
{
    const FunctionCustomizer customizer = [aContext](Red::CClassFunction* func) {
        func->AddParam(GetWHandleArrayType(aContext->propSpec->foreignType)->GetName(), "outList", true, false);
    };

    CreateScriptFunction(aClass, aName, aContext, &ScriptableRecordGetters::HandleGetRecordArray, customizer);
}

void ScriptableRecordManager::CreateGetRecordItem(ScriptableRecordClass* aClass, const std::string& aName,
                                                  const ContextPtr& aContext)
{
    const std::string name = "Get" + aName + "Item";

    const FunctionCustomizer customizer = [aContext](Red::CClassFunction* func) {
        func->AddParam(Red::GetTypeName<int>(), "index", false, false);
        func->SetReturnType(GetWHandleType(aContext->propSpec->foreignType)->GetName());
    };

    CreateScriptFunction(aClass, name, aContext, &ScriptableRecordGetters::HandleGetRecordItem, customizer);
}

void ScriptableRecordManager::CreateGetRecordItemHandle(ScriptableRecordClass* aClass, const std::string& aName,
                                                        const ContextPtr& aContext)
{
    const std::string name = "Get" + aName + "ItemHandle";

    const FunctionCustomizer customizer = [aContext](Red::CClassFunction* func) {
        func->AddParam(Red::GetTypeName<int>(), "index", false, false);
        func->SetReturnType(GetHandleType(aContext->propSpec->foreignType)->GetName());
    };

    CreateScriptFunction(aClass, name, aContext, &ScriptableRecordGetters::HandleGetRecordItemHandle, customizer);
}

void ScriptableRecordManager::CreateRecordArrayContains(ScriptableRecordClass* aClass, const std::string& aName,
                                                        const ContextPtr& aContext)
{
    const std::string name = aName + "Contains";

    const FunctionCustomizer customizer = [aContext](Red::CClassFunction* func) {
        func->AddParam(GetWHandleType(aContext->propSpec->foreignType)->GetName(), "item", false, false);
        func->SetReturnType(Red::GetTypeName<bool>());
    };

    CreateScriptFunction(aClass, name, aContext, &ScriptableRecordGetters::HandleRecordArrayContains, customizer);
}

void ScriptableRecordManager::CreateGetRecord(ScriptableRecordClass* aClass, const std::string& aName,
                                              const ContextPtr& aContext)
{
    const std::string& name = aName;

    const FunctionCustomizer customizer = [aContext](Red::CClassFunction* func) {
        func->SetReturnType(GetWHandleType(aContext->propSpec->foreignType)->GetName());
    };

    CreateScriptFunction(aClass, name, aContext, &ScriptableRecordGetters::HandleGetRecord, customizer);
}

void ScriptableRecordManager::CreateGetRecordHandle(ScriptableRecordClass* aClass, const std::string& aName,
                                                    const ContextPtr& aContext)
{
    const std::string name = aName + "Handle";

    const FunctionCustomizer customizer = [aContext](Red::CClassFunction* func) {
        func->SetReturnType(GetHandleType(aContext->propSpec->foreignType)->GetName());
    };

    CreateScriptFunction(aClass, name, aContext, &ScriptableRecordGetters::HandleGetRecordHandle, customizer);
}

void ScriptableRecordManager::CreateGetArrayCount(ScriptableRecordClass* aClass, const std::string& aName,
                                                  const ContextPtr& aContext)
{
    const std::string name = "Get" + aName + "Count";

    const FunctionCustomizer customizer = [](Red::CClassFunction* func) {
        func->SetReturnType(Red::GetTypeName<int>());
    };

    CreateScriptFunction(aClass, name, aContext, &ScriptableRecordGetters::HandleGetArrayCount, customizer);
}

void ScriptableRecordManager::CreateGetArrayItem(ScriptableRecordClass* aClass, const std::string& aName,
                                                 const ContextPtr& aContext)
{
    const std::string name = "Get" + aName + "Item";

    const FunctionCustomizer customizer = [aContext](Red::CClassFunction* func) {
        func->AddParam(Red::GetTypeName<int>(), "index", false, false);
        func->SetReturnType(aContext->propSpec->propertyType->GetName());
    };

    CreateScriptFunction(aClass, name, aContext, &ScriptableRecordGetters::HandleGetArrayItem, customizer);
}

void ScriptableRecordManager::CreateArrayContains(ScriptableRecordClass* aClass, const std::string& aName,
                                                  const ContextPtr& aContext)
{
    const std::string name = aName + "Contains";

    const FunctionCustomizer customizer = [aContext](Red::CClassFunction* func) {
        const auto* arrayType = reinterpret_cast<const Red::CRTTIBaseArrayType*>(aContext->propSpec->propertyType);
        func->AddParam(arrayType->GetInnerType()->GetName(), "item", false, false);
        func->SetReturnType(Red::GetTypeName<bool>());
    };

    CreateScriptFunction(aClass, name, aContext, &ScriptableRecordGetters::HandleArrayContains, customizer);
}

void ScriptableRecordManager::CreateGet(ScriptableRecordClass* aClass, const std::string& aName,
                                        const ContextPtr& aContext)
{
    const std::string& name = aName;

    const FunctionCustomizer customizer = [aContext](Red::CClassFunction* func) {
        func->SetReturnType(aContext->propSpec->propertyType->GetName());
    };

    CreateScriptFunction(aClass, name, aContext, &ScriptableRecordGetters::HandleGet, customizer);
}
} // namespace App
