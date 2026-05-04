#include "ScriptablePropertyHandler.hpp"

#include "ScriptableTweakDBRecord.hpp"

namespace App
{
void ScriptablePropertyHandler::RegisterFunctions()
{
    using namespace Red::TweakDBUtil;

    for (const auto type : GetFlatTypes())
    {
        if (IsForeignKeyArray(type))
        {
        }
        else if (IsForeignKey(type))
        {
        }
        else if (IsResRefTokenArray(type))
        {
        }
        else if (IsArrayType(type))
        {
        }
        else
        {
            RegisterFunction<&ScriptablePropertyHandler::GetHandler>(
                [type](Red::CGlobalFunction* func) { func->SetReturnType(type); });
        }
    }
}

void ScriptablePropertyHandler::GetRecordArrayHandler(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                                                      int64_t a4)
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

void ScriptablePropertyHandler::GetArrayCountHandler(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                                                     int64_t a4)
{
    aFrame->code++; // Skip ParamEnd operand

    const auto* context = GetContext(aFrame);

    if (!aOut || !context)
        return;

    const auto flat = context->tweakManager->GetFlat(GetFlatID(aInstance, context));

    *static_cast<int*>(aOut) = static_cast<int>(GetArrayCount(flat, context));
}

void ScriptablePropertyHandler::GetRecordItemHandler(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                                                     int64_t a4)
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

void ScriptablePropertyHandler::GetRecordItemHandleHandler(Red::IScriptable* aInstance, Red::CStackFrame* aFrame,
                                                           void* aOut, int64_t a4)
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

void ScriptablePropertyHandler::RecordArrayContainsHandler(Red::IScriptable* aInstance, Red::CStackFrame* aFrame,
                                                           void* aOut, int64_t a4)
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

void ScriptablePropertyHandler::GetRecordHandler(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                                                 int64_t a4)
{
    aFrame->code++; // Skip ParamEnd operand

    const auto* context = GetContext(aFrame);

    if (!aOut || !context)
        return;

    const auto flat = context->tweakManager->GetFlat(GetFlatID(aInstance, context));

    *static_cast<ScriptableRecordManager::RecordWHandle*>(aOut) = GetRecord(flat, context);
}

void ScriptablePropertyHandler::GetRecordHandleHandler(Red::IScriptable* aInstance, Red::CStackFrame* aFrame,
                                                       void* aOut, int64_t a4)
{
    aFrame->code++; // Skip ParamEnd operand

    const auto* context = GetContext(aFrame);

    if (!aOut || !context)
        return;

    const auto flat = context->tweakManager->GetFlat(GetFlatID(aInstance, context));

    *static_cast<ScriptableRecordManager::RecordHandle*>(aOut) = GetRecordHandle(flat, context);
}

void ScriptablePropertyHandler::GetArrayItemHandler(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                                                    int64_t a4)
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

void ScriptablePropertyHandler::ArrayContainsHandler(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                                                     int64_t a4)
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

void ScriptablePropertyHandler::GetHandler(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                                           int64_t a4)
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

Red::TweakDBID ScriptablePropertyHandler::GetFlatID(Red::Instance aInstance,
                                                    const ScriptableRecordManager::Context* aContext)
{
    if (!aInstance || !aContext)
        return {};

    const auto* record = static_cast<ScriptableTweakDBRecord*>(aInstance);
    return record->recordID + aContext->appendix;
}

ScriptablePropertyHandler::RecordArrayPtr ScriptablePropertyHandler::GetRecordArray(const Red::Value<>& aValue,
                                                                                    const Context* aContext)
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

ScriptablePropertyHandler::RecordHandle ScriptablePropertyHandler::GetRecordItemHandle(const Red::Value<>& aValue,
                                                                                       const Context* aContext,
                                                                                       const int aIndex)
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

ScriptablePropertyHandler::RecordWHandle ScriptablePropertyHandler::GetRecordItem(const Red::Value<>& aValue,
                                                                                  const Context* aContext,
                                                                                  const int aIndex)
{
    return GetRecordItemHandle(aValue, aContext, aIndex);
}

bool ScriptablePropertyHandler::RecordArrayContains(const Red::Value<>& aValue, const Context* aContext,
                                                    const RecordWHandle& aRecord)
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

ScriptablePropertyHandler::RecordWHandle ScriptablePropertyHandler::GetRecord(const Red::Value<>& aValue,
                                                                              const Context* aContext)
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

ScriptablePropertyHandler::RecordHandle ScriptablePropertyHandler::GetRecordHandle(const Red::Value<>& aValue,
                                                                                   const Context* aContext)
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

uint32_t ScriptablePropertyHandler::GetArrayCount(const Red::Value<>& aValue, const Context* aContext)
{
    const auto propSpec = aContext->propSpec;

    if (propSpec->propertyType != aValue.type || aValue.type->GetType() != Red::rtti::ERTTIType::Array)
        return 0;

    const auto* targetArrayType = reinterpret_cast<const Red::CRTTIBaseArrayType*>(aValue.type);
    return targetArrayType->GetLength(aValue.instance);
}

bool ScriptablePropertyHandler::ArrayContains(const Red::Value<>& aValue, const Context* aContext, Red::Instance item)
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

const ScriptablePropertyHandler::Context* ScriptablePropertyHandler::GetContext(Red::CStackFrame* aFrame)
{
    const auto* context = *reinterpret_cast<Context**>(aFrame->code);
    aFrame->code += sizeof(Context*); // Move past ctx pointer
    return context;
}

} // namespace App
