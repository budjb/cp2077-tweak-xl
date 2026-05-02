#include "ScriptablePropertyHandler.hpp"

#include "ScriptableRecordManager.hpp"
#include "ScriptableTweakDBRecord.hpp"

namespace
{
constexpr auto PtrSize = sizeof(void*);
const auto* tweakDBIDArrayType = Red::TypeLocator<Red::ERTDBFlatType::TweakDBID>::GetArray();

constexpr auto arrayPrefix = Red::GetTypePrefixStr<Red::DynArray>();
constexpr auto whandlePrefix = Red::GetTypePrefixStr<Red::WeakHandle>();
constexpr auto handlePrefix = Red::GetTypePrefixStr<Red::Handle>();
} // namespace

namespace App
{
void ScriptablePropertyHandler::CreateGetFKArray(
    ScriptableRecordClass* aClass, const std::string& aName,
    const Core::SharedPtr<ScriptableRecordManager::ScriptablePropertySpec>& aSpec)
{
    Red::CNamePool::Add(aName.c_str());

    auto* func = Red::CClassFunction::Create(aClass, aName.c_str(), aName.c_str(), &HandleGetRecordArray);
    func->AddParam(GetWHandleArrayType(aSpec->typeSpec->foreignType)->GetName(), "outList", true, false);
    aClass->RegisterFunction(func);
}

void ScriptablePropertyHandler::CreateGetArraySize(
    ScriptableRecordClass* aClass, const std::string& aName,
    const Core::SharedPtr<ScriptableRecordManager::ScriptablePropertySpec>& aSpec)
{
    const std::string name = "Get" + aName + "Count";
    Red::CNamePool::Add(name.c_str());

    auto* func = Red::CClassFunction::Create(aClass, name.c_str(), name.c_str(), &HandleGetArrayCount);
    func->SetReturnType(Red::GetTypeName<int>());

    aClass->RegisterFunction(func);
}

void ScriptablePropertyHandler::CreateGetRecordWHandleAt(
    ScriptableRecordClass* aClass, const std::string& aName,
    const Core::SharedPtr<ScriptableRecordManager::ScriptablePropertySpec>& aSpec)
{
    const std::string name = "Get" + aName + "Item";
    Red::CNamePool::Add(name.c_str());

    auto* func = Red::CClassFunction::Create(aClass, name.c_str(), name.c_str(), &HandleGetRecordItem);
    func->AddParam(Red::GetTypeName<int>(), "index", false, false);
    func->SetReturnType(GetWHandleType(aSpec->typeSpec->foreignType)->GetName());

    aClass->RegisterFunction(func);
}

void ScriptablePropertyHandler::CreateGetRecordHandleAt(
    ScriptableRecordClass* aClass, const std::string& aName,
    const Core::SharedPtr<ScriptableRecordManager::ScriptablePropertySpec>& aSpec)
{
    const std::string name = "Get" + aName + "ItemHandle";
    Red::CNamePool::Add(name.c_str());

    auto* func = Red::CClassFunction::Create(aClass, name.c_str(), name.c_str(), &HandleGetRecordItemHandle);
    func->AddParam(Red::GetTypeName<int>(), "index", false, false);
    func->SetReturnType(GetHandleType(aSpec->typeSpec->foreignType)->GetName());

    aClass->RegisterFunction(func);
}

void ScriptablePropertyHandler::CreateRecordArrayContains(
    ScriptableRecordClass* aClass, const std::string& aName,
    const Core::SharedPtr<ScriptableRecordManager::ScriptablePropertySpec>& aSpec)
{
    const std::string name = aName + "Contains";
    Red::CNamePool::Add(name.c_str());

    auto* func = Red::CClassFunction::Create(aClass, name.c_str(), name.c_str(), &HandleGetRecordItemHandle);
    func->AddParam(GetWHandleType(aSpec->typeSpec->foreignType)->GetName(), "item", false, false);
    func->SetReturnType(Red::GetTypeName<bool>());

    aClass->RegisterFunction(func);
}

void ScriptablePropertyHandler::CreateGetRecordWHandle(
    ScriptableRecordClass* aClass, const std::string& aName,
    const Core::SharedPtr<ScriptableRecordManager::ScriptablePropertySpec>& aSpec)
{
    const std::string name = aName;
    Red::CNamePool::Add(name.c_str());

    auto* func = Red::CClassFunction::Create(aClass, name.c_str(), name.c_str(), &HandleGetRecord);
    func->SetReturnType(GetWHandleType(aSpec->typeSpec->foreignType)->GetName());

    aClass->RegisterFunction(func);
}

void ScriptablePropertyHandler::CreateGetRecordHandle(
    ScriptableRecordClass* aClass, const std::string& aName,
    const Core::SharedPtr<ScriptableRecordManager::ScriptablePropertySpec>& aSpec)
{
    const std::string name = aName + "Handle";
    Red::CNamePool::Add(name.c_str());

    auto* func = Red::CClassFunction::Create(aClass, name.c_str(), name.c_str(), &HandleGetRecordItemHandle);
    func->SetReturnType(GetHandleType(aSpec->typeSpec->foreignType)->GetName());

    aClass->RegisterFunction(func);
}

void ScriptablePropertyHandler::CreateGetResRefArray(
    ScriptableRecordClass* aClass, const std::string& aName,
    const Core::SharedPtr<ScriptableRecordManager::ScriptablePropertySpec>& aSpec)
{
    const std::string name = aName;
    Red::CNamePool::Add(name.c_str());

    auto* func = Red::CClassFunction::Create(aClass, name.c_str(), name.c_str(), &HandleGet);
    func->SetReturnType(Red::GetTypeName<Red::DynArray<Red::ResRef>>());

    aClass->RegisterFunction(func);
}

void ScriptablePropertyHandler::CreateGetResRefArraySize(
    ScriptableRecordClass* aClass, const std::string& aName,
    const Core::SharedPtr<ScriptableRecordManager::ScriptablePropertySpec>& aSpec)
{
    const std::string name = "Get" + aName + "Count";
    Red::CNamePool::Add(name.c_str());

    auto* func = Red::CClassFunction::Create(aClass, name.c_str(), name.c_str(), &HandleGetArrayCount);
    func->SetReturnType(Red::GetTypeName<int>());

    aClass->RegisterFunction(func);
}

void ScriptablePropertyHandler::CreateGetResRefArrayItem(
    ScriptableRecordClass* aClass, const std::string& aName,
    const Core::SharedPtr<ScriptableRecordManager::ScriptablePropertySpec>& aSpec)
{
    const std::string name = "Get" + aName + "Item";
    Red::CNamePool::Add(name.c_str());

    auto* func = Red::CClassFunction::Create(aClass, name.c_str(), name.c_str(), &HandleGetArrayItem);
    func->AddParam(Red::GetTypeName<int>(), "index", false, false);
    func->SetReturnType(Red::GetTypeName<Red::ResRef>());

    aClass->RegisterFunction(func);
}

void ScriptablePropertyHandler::CreateGetArray(
    ScriptableRecordClass* aClass, const std::string& aName,
    const Core::SharedPtr<ScriptableRecordManager::ScriptablePropertySpec>& aSpec)
{
    const std::string name = aName;
    Red::CNamePool::Add(name.c_str());

    auto* func = Red::CClassFunction::Create(aClass, name.c_str(), name.c_str(), &HandleGet);
    func->SetReturnType(aSpec->typeSpec->propertyType->GetName());

    aClass->RegisterFunction(func);
}

void ScriptablePropertyHandler::CreateGetArrayItem(
    ScriptableRecordClass* aClass, const std::string& aName,
    const Core::SharedPtr<ScriptableRecordManager::ScriptablePropertySpec>& aSpec)
{
    const std::string name = "Get" + aName + "Item";
    Red::CNamePool::Add(name.c_str());

    auto* func = Red::CClassFunction::Create(aClass, name.c_str(), name.c_str(), &HandleGetArrayItem);
    func->AddParam(Red::GetTypeName<int>(), "index", false, false);
    func->SetReturnType(aSpec->typeSpec->propertyType->GetName());

    aClass->RegisterFunction(func);
}

void ScriptablePropertyHandler::CreateArrayContains(
    ScriptableRecordClass* aClass, const std::string& aName,
    const Core::SharedPtr<ScriptableRecordManager::ScriptablePropertySpec>& aSpec)
{
    const std::string name = aName + "Contains";
    Red::CNamePool::Add(name.c_str());

    auto* func = Red::CClassFunction::Create(aClass, name.c_str(), name.c_str(), &HandleArrayContains);
    func->AddParam(aSpec->typeSpec->propertyType->GetName(), "item", false, false);
    func->SetReturnType(Red::GetTypeName<bool>());

    aClass->RegisterFunction(func);
}

void ScriptablePropertyHandler::CreateGet(ScriptableRecordClass* aClass, const std::string& aName,
                                          const Core::SharedPtr<ScriptableRecordManager::ScriptablePropertySpec>& aSpec)
{
    const std::string name = aName;
    Red::CNamePool::Add(name.c_str());

    auto* func = Red::CClassFunction::Create(aClass, name.c_str(), name.c_str(), &HandleGet);
    func->SetReturnType(aSpec->typeSpec->propertyType->GetName());

    aClass->RegisterFunction(func);
}

void ScriptablePropertyHandler::HandleGetRecordArray(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                                                 int64_t a4)
{
    Red::DynArray<Red::WeakHandle<Red::TweakDBRecord>>* outArray;
    Red::GetParameter(aFrame, &outArray);

    aFrame->code++; // Skip ParamEnd operand

    const auto* context = *reinterpret_cast<const ScriptableRecordManager::Context**>(aFrame->code);
    aFrame->code += PtrSize; // Move past ctx pointer

    if (!aOut || !context)
        return;

    const auto* record = static_cast<ScriptableTweakDBRecord*>(aInstance);
    const auto flat = context->tweakManager->GetFlat(record->recordID);

    *static_cast<RecordWHandleArray*>(aOut) = *GetRecordArray(flat, context->propSpec);
}

void ScriptablePropertyHandler::HandleGetArrayCount(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                                                   int64_t a4)
{
    aFrame->code++; // Skip ParamEnd operand

    const auto* context = *reinterpret_cast<const ScriptableRecordManager::Context**>(aFrame->code);
    aFrame->code += PtrSize; // Move past ctx pointer

    if (!aOut || !context)
        return;

    const auto* record = static_cast<ScriptableTweakDBRecord*>(aInstance);
    const auto flat = context->tweakManager->GetFlat(record->recordID);

    *static_cast<int*>(aOut) = GetArrayCount(flat, context->propSpec);
}

void ScriptablePropertyHandler::HandleGetRecordItem(Red::IScriptable* aInstance, Red::CStackFrame* aFrame,
                                                         void* aOut, int64_t a4)
{
    int index;
    Red::GetParameter(aFrame, &index);

    aFrame->code++; // Skip ParamEnd operand

    const auto* context = *reinterpret_cast<const ScriptableRecordManager::Context**>(aFrame->code);
    aFrame->code += PtrSize; // Move past ctx pointer

    if (!aOut || !context)
        return;

    const auto* record = static_cast<ScriptableTweakDBRecord*>(aInstance);
    const auto flat = context->tweakManager->GetFlat(record->recordID);

    *static_cast<RecordWHandle*>(aOut) = GetRecordItem(flat, context->propSpec, index);
}

void ScriptablePropertyHandler::HandleGetRecordItemHandle(Red::IScriptable* aInstance, Red::CStackFrame* aFrame,
                                                        void* aOut, int64_t a4)
{
    int index;
    Red::GetParameter(aFrame, &index);

    aFrame->code++; // Skip ParamEnd operand

    const auto* context = *reinterpret_cast<const ScriptableRecordManager::Context**>(aFrame->code);
    aFrame->code += PtrSize; // Move past ctx pointer

    if (!aOut || !context)
        return;

    const auto* record = static_cast<ScriptableTweakDBRecord*>(aInstance);
    const auto flat = context->tweakManager->GetFlat(record->recordID);

    *static_cast<RecordHandle*>(aOut) = GetRecordItemHandle(flat, context->propSpec, index);
}

void ScriptablePropertyHandler::HandleRecordArrayContains(Red::IScriptable* aInstance, Red::CStackFrame* aFrame,
                                                          void* aOut, int64_t a4)
{
    RecordWHandle record;
    Red::GetParameter(aFrame, &record);

    aFrame->code++; // Skip ParamEnd operand

    const auto* context = *reinterpret_cast<const ScriptableRecordManager::Context**>(aFrame->code);
    aFrame->code += PtrSize; // Move past ctx pointer

    if (!aOut || !context)
        return;

    const auto* rec = static_cast<ScriptableTweakDBRecord*>(aInstance);
    const auto flat = context->tweakManager->GetFlat(rec->recordID);

    *static_cast<bool*>(aOut) = RecordArrayContains(flat, context->propSpec, record);
}

void ScriptablePropertyHandler::HandleGetRecord(Red::IScriptable* aInstance, Red::CStackFrame* aFrame,
                                                       void* aOut, int64_t a4)
{
    aFrame->code++; // Skip ParamEnd operand

    const auto* context = *reinterpret_cast<const ScriptableRecordManager::Context**>(aFrame->code);
    aFrame->code += PtrSize; // Move past ctx pointer

    if (!aOut || !context)
        return;

    const auto* record = static_cast<ScriptableTweakDBRecord*>(aInstance);
    const auto flat = context->tweakManager->GetFlat(record->recordID);

    *static_cast<RecordWHandle*>(aOut) = GetRecord(flat, context->propSpec);
}

void ScriptablePropertyHandler::HandleGetRecordHandle(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                                                      int64_t a4)
{
    aFrame->code++; // Skip ParamEnd operand

    const auto* context = *reinterpret_cast<const ScriptableRecordManager::Context**>(aFrame->code);
    aFrame->code += PtrSize; // Move past ctx pointer

    if (!aOut || !context)
        return;

    const auto* record = static_cast<ScriptableTweakDBRecord*>(aInstance);
    const auto flat = context->tweakManager->GetFlat(record->recordID);

    *static_cast<RecordHandle*>(aOut) = GetRecordHandle(flat, context->propSpec);
}

void ScriptablePropertyHandler::HandleGetArrayItem(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                                                   int64_t a4)
{
    int index;
    Red::GetParameter(aFrame, &index);

    aFrame->code++;

    const auto* context = *reinterpret_cast<const ScriptableRecordManager::Context**>(aFrame->code);
    aFrame->code += PtrSize;

    if (!aOut || !context)
        return;

    const auto* record = static_cast<ScriptableTweakDBRecord*>(aInstance);
    const auto flat = context->tweakManager->GetFlat(record->recordID);

    if (flat.type != context->propSpec->flatType || flat.type->GetType() != Red::ERTTIType::Array)
        return;

    const auto* arrayType = static_cast<const Red::CRTTIBaseArrayType*>(flat.type);
    const auto* innerType = arrayType->GetInnerType();
    const auto length = arrayType->GetLength(flat.instance);

    if (index < 0 || index >= length)
        return;

    innerType->Assign(aOut, arrayType->GetElement(flat.instance, index));
}

void ScriptablePropertyHandler::HandleArrayContains(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                                                    int64_t a4)
{
    const auto* context = *reinterpret_cast<const ScriptableRecordManager::Context**>(aFrame->code);
    aFrame->code += PtrSize;

    if (!aOut || !context)
        return;

    const auto* arrayType = static_cast<const Red::CRTTIBaseArrayType*>(context->propSpec->propertyType);
    const auto* innerType = arrayType->GetInnerType();

    const auto item = Red::MakeValue(innerType);
    Red::GetParameter(aFrame, item->instance);

    aFrame->code++;

    const auto* record = static_cast<ScriptableTweakDBRecord*>(aInstance);
    const auto flat = context->tweakManager->GetFlat(record->recordID);

    *static_cast<bool*>(aOut) = ArrayContains(flat, context->propSpec, item->instance);
}

void ScriptablePropertyHandler::HandleGet(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut, int64_t a4)
{
    aFrame->code++;

    const auto* context = *reinterpret_cast<const ScriptableRecordManager::Context**>(aFrame->code);
    aFrame->code += PtrSize;

    if (!aOut || !context)
        return;

    const auto* record = static_cast<ScriptableTweakDBRecord*>(aInstance);
    const auto flat = context->tweakManager->GetFlat(record->recordID);

    if (flat.type == context->propSpec->propertyType)
    {
        flat.type->Assign(aOut, flat.instance);
    }
}

ScriptablePropertyHandler::RecordWHandleArrayPtr ScriptablePropertyHandler::GetRecordArray(
    const Red::Value<>& aValue, const TweakPropertySpecPtr& aSpec)
{
    if (!aSpec->isForeignKey || !aSpec->isArray || aValue.type != aSpec->flatType)
        return nullptr;

    const auto* targetArrayType = reinterpret_cast<const Red::CRTTIBaseArrayType*>(aSpec->propertyType);
    const auto* innerType = targetArrayType->GetInnerType();

    if (!innerType || innerType->GetType() != Red::ERTTIType::WeakHandle)
        return nullptr;

    const auto length = tweakDBIDArrayType->GetLength(aValue.instance);
    const auto result = Red::MakeInstance<RecordWHandleArray>(length);

    for (uint32_t i = 0; i < length; ++i)
    {
        auto& element = result->At(i);
        const auto& id = *static_cast<Red::TweakDBID*>(tweakDBIDArrayType->GetElement(aValue.instance, i));
        AssignRecord(targetArrayType->GetElement(&element, i), id, innerType, aSpec->foreignType);
    }

    return result;
}

ScriptablePropertyHandler::RecordWHandle ScriptablePropertyHandler::GetRecordItem(
    const Red::Value<>& aValue, const TweakPropertySpecPtr& aSpec, const int aIndex)
{
    return GetRecordItemHandle(aValue, aSpec, aIndex);
}

ScriptablePropertyHandler::RecordHandle ScriptablePropertyHandler::GetRecordItemHandle(const Red::Value<>& aValue,
                                                                                     const TweakPropertySpecPtr& aSpec,
                                                                                     const int aIndex)
{
    if (!aSpec->isForeignKey || !aSpec->isArray || aSpec->propertyType->GetType() != Red::ERTTIType::Array)
        return nullptr;

    const auto* targetArrayType = reinterpret_cast<const Red::CRTTIBaseArrayType*>(aSpec->propertyType);
    const auto* innerType = targetArrayType->GetInnerType();

    if (!innerType || innerType->GetType() != Red::ERTTIType::Handle)
        return nullptr;

    const auto length = tweakDBIDArrayType->GetLength(aValue.instance);

    if (aIndex < 0 || aIndex >= length)
        return {};

    const auto& id = *static_cast<Red::TweakDBID*>(targetArrayType->GetElement(aValue.instance, aIndex));
    auto record = s_manager->GetRecord(id);

    if (record && record->GetType()->IsA(aSpec->foreignType))
        return record;

    return nullptr;
}

bool ScriptablePropertyHandler::RecordArrayContains(const Red::Value<>& aValue, const TweakPropertySpecPtr& aSpec,
                                                    const Red::WeakHandle<Red::TweakDBRecord>& aRecord)
{
    if (!aSpec->isForeignKey || !aSpec->isArray || aSpec->propertyType->GetType() != Red::ERTTIType::Array)
        return false;

    // TODO: dunno if this works
    const auto& array = static_cast<Red::DynArray<Red::TweakDBID>*>(aValue.instance);
    return array->Contains(aRecord.instance->recordID);
}

ScriptablePropertyHandler::RecordWHandle ScriptablePropertyHandler::GetRecord(const Red::Value<>& aValue,
                                                                                     const TweakPropertySpecPtr& aSpec)
{
    if (!aSpec->isForeignKey || aSpec->propertyType->GetType() != Red::ERTTIType::WeakHandle)
        return {};

    const auto& id = *static_cast<Red::TweakDBID*>(aValue.instance);
    auto record = s_manager->GetRecord(id);

    if (record && record->GetType()->IsA(aSpec->foreignType))
        return record;

    return {};
}

ScriptablePropertyHandler::RecordHandle ScriptablePropertyHandler::GetRecordHandle(const Red::Value<>& aValue,
                                                                                   const TweakPropertySpecPtr& aSpec)
{
    if (!aSpec->isForeignKey || aSpec->propertyType->GetType() != Red::ERTTIType::Handle)
        return {};

    const auto& id = *static_cast<Red::TweakDBID*>(aValue.instance);
    auto record = s_manager->GetRecord(id);

    if (record && record->GetType()->IsA(aSpec->foreignType))
        return record;

    return {};
}

uint32_t ScriptablePropertyHandler::GetArrayCount(const Red::Value<>& aValue, const TweakPropertySpecPtr& aSpec)
{
    if (aSpec->propertyType != aValue.type || aValue.type->GetType() != Red::ERTTIType::Array)
        return 0;

    const auto* targetArrayType = static_cast<const Red::CRTTIBaseArrayType*>(aValue.type);
    return targetArrayType->GetLength(aValue.instance);
}

bool ScriptablePropertyHandler::ArrayContains(const Red::Value<>& aValue, const TweakPropertySpecPtr& aSpec,
                                              Red::Instance item)
{
    if (aSpec->propertyType->GetType() != Red::ERTTIType::Array || aValue.type != aSpec->propertyType)
        return false;

    auto* arrayType =
        const_cast<Red::CRTTIBaseArrayType*>(static_cast<const Red::CRTTIBaseArrayType*>(aSpec->propertyType));

    const auto length = arrayType->GetLength(aValue.instance);

    for (uint32_t i = 0; i < length; ++i)
    {
        if (arrayType->IsEqual(arrayType->GetElement(aValue.instance, i), item))
            return true;
    }

    return false;
}

void ScriptablePropertyHandler::AssignRecord(const Red::Instance aInstance, const Red::TweakDBID aId,
                                             const Red::CBaseRTTIType* aType, const Red::CClass* aRecordType)
{
    const auto record = s_manager->GetRecord(aId);

    if (!record || !record->GetType()->IsA(aRecordType))
    {
        if (aType->GetType() == Red::ERTTIType::WeakHandle)
        {
            *static_cast<Red::WeakHandle<Red::TweakDBRecord>*>(aInstance) = Red::WeakHandle<Red::TweakDBRecord>{};
        }
        else
        {
            *static_cast<Red::Handle<Red::TweakDBRecord>*>(aInstance) = nullptr;
        }
    }
    else
    {
        if (aType->GetType() == Red::ERTTIType::WeakHandle)
        {
            *static_cast<Red::WeakHandle<Red::TweakDBRecord>*>(aInstance) = record;
        }
        else
        {
            *static_cast<Red::Handle<Red::TweakDBRecord>*>(aInstance) = record;
        }
    }
}

Red::CBaseRTTIType* ScriptablePropertyHandler::GetHandleType(const Red::CClass* aClass)
{
    std::string name = handlePrefix.data();
    name.append(aClass->GetName().ToString());
    return Red::CRTTISystem::Get()->GetType(Red::CNamePool::Add(name.c_str()));
}

Red::CBaseRTTIType* ScriptablePropertyHandler::GetWHandleType(const Red::CClass* aClass)
{
    std::string name = whandlePrefix.data();
    name.append(aClass->GetName().ToString());
    return Red::CRTTISystem::Get()->GetType(Red::CNamePool::Add(name.c_str()));
}

Red::CBaseRTTIType* ScriptablePropertyHandler::GetWHandleArrayType(const Red::CClass* aClass)
{
    std::string name = arrayPrefix.data();
    name.append(whandlePrefix.data());
    name.append(aClass->GetName().ToString());
    return Red::CRTTISystem::Get()->GetType(Red::CNamePool::Add(name.c_str()));
}

void ScriptablePropertyHandler::SetTweakDBManager(const Core::DeferredPtr<Red::TweakDBManager>& aManager)
{
    s_manager = aManager;
}

} // namespace App
