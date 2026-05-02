#include "ScriptableRecordManager.hpp"

#include "App/Tweaks/TweakService.hpp"
#include "ScriptablePropertyHandler.hpp"
#include "ScriptableTweakDBRecord.hpp"

namespace App
{
ScriptableRecordManager::ScriptableRecordManager(const Core::DeferredPtr<Red::TweakDBManager>& aManager)
    : m_rtti(Red::CRTTISystem::Get())
    , m_tweakManager(aManager)
{
}

ScriptableRecordManager::~ScriptableRecordManager()
{
    for (auto& spec : m_specs | std::views::values)
    {
        UnregisterScriptableRecordSpec(spec);
    }

    // for (const auto& entry : m_functions)
    // {
    //     DestroyClosure(entry);
    // }
}

void ScriptableRecordManager::CreateFunctions(ScriptableRecordClass* aClass,
                                              const Core::SharedPtr<ScriptablePropertySpec>& aSpec)
{
    const auto typeSpec = aSpec->typeSpec;

    if (typeSpec->isArray && typeSpec->isForeignKey)
        CreateFKArrayFunctions(aClass, aSpec);
}

void ScriptableRecordManager::CreateFKArrayFunctions(ScriptableRecordClass* aClass,
                                                     const Core::SharedPtr<ScriptablePropertySpec>& aSpec)
{
    const auto baseName = Red::TweakDBUtil::Capitalize(aSpec->name);

    // void [Prop](DynArray<WeakHandle<TweakDBRecord>* out);
    ScriptablePropertyHandler::CreateGetFKArray(aClass, baseName, aSpec);

    // int Get[Prop]Count()
    ScriptablePropertyHandler::CreateGetArraySize(aClass, baseName, aSpec);

    // WeakHandle<TweakDBRecord> Get[Prop]Item(int index)
    ScriptablePropertyHandler::CreateGetRecordWHandleAt(aClass, baseName, aSpec);

    // Handle<TweakDBRecord> Get[Prop]ItemHandle(int index)
    ScriptablePropertyHandler::CreateGetRecordHandleAt(aClass, baseName, aSpec);

    // bool [Prop]Contains(WeakHandle<TweakDBRecord> item)
    ScriptablePropertyHandler::CreateRecordArrayContains(aClass, baseName, aSpec);
}

void ScriptableRecordManager::CreateFKFunctions(ScriptableRecordClass* aClass,
                                                const Core::SharedPtr<ScriptablePropertySpec>& aSpec)
{
    const auto baseName = Red::TweakDBUtil::Capitalize(aSpec->name);

    // WeakHandle<TweakDBRecord> [Prop]()
    ScriptablePropertyHandler::CreateGetRecordWHandle(aClass, baseName, aSpec);

    // Handle<TweakDBRecord> [Prop]Handle()
    ScriptablePropertyHandler::CreateGetRecordHandle(aClass, baseName, aSpec);
}

void ScriptableRecordManager::CreateResRefArrayFunctions(ScriptableRecordClass* aClass,
                                                         const Core::SharedPtr<ScriptablePropertySpec>& aSpec)
{
    const auto baseName = Red::TweakDBUtil::Capitalize(aSpec->name);

    // DynArray<ResRef> [Prop]()
    ScriptablePropertyHandler::CreateGetResRefArray(aClass, aSpec->name, aSpec);

    // int Get[Prop]Count()
    ScriptablePropertyHandler::CreateGetResRefArraySize(aClass, baseName, aSpec);

    // ResRef Get[Prop]Item(int index)
    ScriptablePropertyHandler::CreateGetResRefArrayItem(aClass, baseName, aSpec);
}

void ScriptableRecordManager::CreateArrayFunctions(ScriptableRecordClass* aClass,
                                                   const Core::SharedPtr<ScriptablePropertySpec>& aSpec)
{
    const auto baseName = Red::TweakDBUtil::Capitalize(aSpec->name);

    // DynArray<CName> [Prop]()
    ScriptablePropertyHandler::CreateGetArray(aClass, aSpec->name, aSpec);

    // int Get[Prop]Count()
    ScriptablePropertyHandler::CreateGetArraySize(aClass, baseName, aSpec);

    // CName Get[Prop]Item(int index)
    ScriptablePropertyHandler::CreateGetArrayItem(aClass, baseName, aSpec);

    // bool [Prop]Contains(CName item)
    ScriptablePropertyHandler::CreateArrayContains(aClass, baseName, aSpec);
}

void ScriptableRecordManager::CreateNormalFunctions(ScriptableRecordClass* aClass,
                                                    const Core::SharedPtr<ScriptablePropertySpec>& aSpec)
{
    const auto baseName = Red::TweakDBUtil::Capitalize(aSpec->name);

    // [type] [Prop]()
    ScriptablePropertyHandler::CreateGet(aClass, baseName, aSpec);
}

Red::CBaseFunction* ScriptableRecordManager::CreateFunction(ScriptableRecordClass* aClass, const std::string& aName,
                                                            const TweakPropertySpecPtr& aSpec,
                                                            const ContextPtr& aContext)
{
    const auto name = Red::TweakDBUtil::Capitalize(aName);
    const auto nativeName = std::string("__").append(aName);

    Red::CClassFunction* nativeFunc = Red::CClassFunction::Create<void*>(aClass, nativeName.c_str(), nativeName.c_str(),
                                                                         &ScriptableRecordManager::DispatchNoArgGetter);
    nativeFunc->SetReturnType(aSpec->propertyTypeName);

    aClass->RegisterFunction(nativeFunc);

    constexpr Red::Memory::RTTIFunctionAllocator allocator;
    const auto scriptFunc = allocator.Alloc<Red::CClassFunction>();
    std::memcpy(scriptFunc, nativeFunc, sizeof(Red::CClassFunction));

    const auto fullName = Red::Detail::MakeScriptFunctionName(scriptFunc, name.c_str());
    scriptFunc->shortName = Red::CNamePool::Add(name);
    scriptFunc->fullName = Red::CNamePool::Add(fullName.c_str());

    const auto bytecode = CreateFunctionBytecode(aContext.get(), nativeFunc);
    scriptFunc->bytecode.bytecode.buffer.data = bytecode.data;
    scriptFunc->bytecode.bytecode.buffer.size = bytecode.size;

    scriptFunc->flags.isNative = false;
    aClass->RegisterFunction(scriptFunc);

    {
        std::scoped_lock lockRW(m_functionsMutex);
        m_functions.emplace_back(scriptFunc);
        m_contexts.emplace_back(aContext);
    }

    return scriptFunc;
}

Red::RawBuffer ScriptableRecordManager::CreateFunctionBytecode(const Context* aContext, Red::CBaseFunction* aFunc)
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
        aFunc->params.Size() * (OpSize + PointerSize) + PointerSize + (aFunc->returnType ? 1 : 0);
    const uint32_t finalCodeSize = BaseCodeSize + extraCodeSize;
    const uint16_t finalExitOffset = BaseExitOffset + extraCodeSize;

    constexpr Red::Memory::EngineAllocator allocator;
    auto* memory = allocator.Alloc(finalCodeSize).memory;
    auto* code = static_cast<uint8_t*>(memory);

    if (aFunc->returnType)
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

    *reinterpret_cast<void**>(code) = aFunc;
    code += PointerSize;

    *reinterpret_cast<uint16_t*>(code) = 0;
    code += FlagsSize;

    for (const auto& param : aFunc->params)
    {
        *code = ParamOp;
        code += OpSize;

        *reinterpret_cast<void**>(code) = param;
        code += PointerSize;
    }

    *code = ParamEndOp;
    code += OpSize;

    *reinterpret_cast<const Context**>(code) = aContext;

    return {memory, finalCodeSize};
}

// bool ScriptableRecordManager::DestroyClosure(const Core::SharedPtr<Closure>& aClosure)
// {
//     if (aClosure)
//     {
//         return DestroyClosure(aClosure->closure);
//     }
//     return false;
// }
//
// bool ScriptableRecordManager::DestroyClosure(const ffi_closure* aClosure)
// {
//     if (!aClosure)
//     {
//         return false;
//     }
//
//     std::scoped_lock lock(m_closuresMutex);
//
//     for (auto it = m_functions.begin(); it != m_functions.end(); ++it)
//     {
//         const auto& entry = *it;
//
//         if (!entry || !entry->closure)
//         {
//             continue;
//         }
//
//         if (entry->closure != aClosure)
//         {
//             continue;
//         }
//
//         ffi_closure_free(entry->closure);
//         entry->closure = nullptr;
//         entry->executable = nullptr;
//         m_functions.erase(it);
//
//         return true;
//     }
//
//     return false;
// }

Red::CName ScriptableRecordManager::RegisterScriptableRecordType(const std::string& aName,
                                                                 const std::optional<std::string>& aParentName)
{
    const auto cname = aName.c_str();

    if (GetRecordSpec(cname))
    {
        LogError("Registration of record type {} failed because another with the same name is already registered.",
                 aName);
        return {};
    }

    if (m_rtti->GetClass(cname))
    {
        LogError("Registration of record type {} failed because a class with the same name already exists.", aName);
        return {};
    }

    const auto spec = Core::MakeShared<ScriptableRecordSpec>();

    spec->name = aName;
    spec->aliasName = Red::TweakDBUtil::GetRecordAliasName<std::string>(aName);
    spec->shortName = Red::TweakDBUtil::GetRecordShortName<std::string>(aName);

    spec->cname = Red::CNamePool::Add(spec->name.c_str());
    spec->aliasCName = Red::CNamePool::Add(spec->aliasName.c_str());
    spec->shortCName = Red::CName(spec->shortName.c_str());

    spec->hash = Red::TweakDBUtil::GetRecordTypeHash(spec->shortName);
    spec->parent = aParentName;

    {
        std::unique_lock lockRW(m_specsMutex);
        m_specs[spec->cname] = spec;
        m_specsByHash[spec->hash] = spec;
        return spec->cname;
    }
}

Red::CName ScriptableRecordManager::RegisterScriptableProperty(Red::CName aRecordName, const std::string& aPropertyName,
                                                               const TweakPropertySpecPtr& aTypeSpec,
                                                               const Red::InstancePtr<>& aDefaultValue)
{
    const auto cname = Red::CName{aPropertyName.c_str()};

    const auto recordInfo = GetRecordSpec(aRecordName);

    if (!recordInfo)
    {
        LogError("Failed to register property {} for record type {} because the registration for record type does not "
                 "exist.",
                 aPropertyName, aRecordName.ToString());
        return {};
    }

    if (recordInfo->props.contains(cname))
    {
        return {};
    }

    const auto propertyInfo = Core::MakeShared<ScriptablePropertySpec>();
    propertyInfo->name = aPropertyName;
    propertyInfo->appendix = "." + aPropertyName;
    propertyInfo->typeSpec = aTypeSpec;
    propertyInfo->cname = Red::CName{aPropertyName.c_str()};
    propertyInfo->defaultValue = aDefaultValue;

    recordInfo->props[propertyInfo->cname] = propertyInfo;

    return propertyInfo->cname;
}

void ScriptableRecordManager::RegisterScriptableRecordSpecs()
{
#ifndef NDEBUG
    RegisterTestScriptableRecord();
#endif

    for (auto& spec : m_specs | std::views::values)
    {
        RegisterScriptableRecordSpec(spec);
    }
}

void ScriptableRecordManager::DescribeScriptableRecordSpecs()
{
    for (auto& spec : m_specs | std::views::values)
    {
        DescribeScriptableRecordSpec(spec);
    }
}

void ScriptableRecordManager::InsertScriptableRecordDefaults()
{
    for (const auto& spec : m_specs | std::views::values)
    {
        InsertScriptableRecordDefaults(spec);
    }
}

Core::SharedPtr<ScriptableRecordManager::ScriptableRecordSpec> ScriptableRecordManager::GetRecordSpec(
    Red::CName aName) const
{
    std::shared_lock lockR(m_specsMutex);
    if (const auto it = m_specs.find(aName); it != m_specs.end())
    {
        return it->second;
    }
    return nullptr;
}

bool ScriptableRecordManager::RegisterScriptableRecordSpec(const Core::SharedPtr<ScriptableRecordSpec>& aSpec)
{
    if (aSpec->isRegistered)
    {
        LogDebug("Record type {} is already registered, skipping registration.", aSpec->shortName);
        return false;
    }

    if (auto* cls = CreateRecordClass(aSpec))
    {
        aSpec->type = cls;
        aSpec->isRegistered = true;
        return true;
    }

    return false;
}

bool ScriptableRecordManager::DescribeScriptableRecordSpec(const Core::SharedPtr<ScriptableRecordSpec>& aSpec)
{
    if (!aSpec->isRegistered || aSpec->isDescribed || !aSpec->type)
    {
        LogDebug("Record type {} is not registered or already described, skipping description.", aSpec->shortName);
        return false;
    }

    if (aSpec->parent.has_value())
    {
        auto* parentCls = m_rtti->GetClass(Red::TweakDBUtil::GetRecordFullName<Red::CName>(aSpec->parent.value()));

        if (!parentCls || !Red::TweakDBUtil::IsRecordType(parentCls))
        {
            LogError("Failed to describe record type {} because the specified parent type {} does not exist or is not "
                     "a valid record type.",
                     aSpec->shortName, aSpec->parent.value());
            return false;
        }

        aSpec->type->parent = parentCls;
    }
    else
    {
        aSpec->type->parent = ScriptableTweakDBRecord::TYPE::GetClass();
    }

    for (const auto& prop : aSpec->props | std::views::values)
    {
        DescribeScriptablePropertySpec(aSpec->type, prop);
    }

    aSpec->isDescribed = true;
    return true;
}

bool ScriptableRecordManager::DescribeScriptablePropertySpec(ScriptableRecordClass* aClass,
                                                             const Core::SharedPtr<ScriptablePropertySpec>& aSpec)
{
    if (aSpec->isDescribed)
    {
        LogDebug("Property {} of record type {} is already described, skipping description.", aSpec->name,
                 aClass->GetName().ToString());
        return false;
    }

    const auto& typeInfo = aSpec->typeSpec;

    if (typeInfo->isForeignKey)
    {
        if (!typeInfo->foreignType)
        {
            if (const auto* type = m_rtti->GetClass(typeInfo->foreignTypeName))
            {
                typeInfo->foreignType = type;
            }
            else
            {
                LogError("Failed to describe property {} of record type {}, the foreign type {} does not exist.",
                         aSpec->name, aClass->GetName().ToString(), typeInfo->foreignName);
                return false;
            }
        }
    }

    if (!typeInfo->propertyType)
    {
        if (const auto* type = m_rtti->GetType(typeInfo->propertyTypeName))
        {
            typeInfo->propertyType = type;
        }
        else
        {
            LogError("Failed to describe property {} of record type {}, the property type {} does not exist.",
                     aSpec->name, aClass->GetName().ToString(), typeInfo->foreignName);
            return false;
        }
    }

    CreateFunctions(aClass, aSpec);

    aSpec->isDescribed = true;
    return true;
}

void ScriptableRecordManager::InsertScriptableRecordDefaults(const Core::SharedPtr<ScriptableRecordSpec>& aSpec)
{
    if (!aSpec->isDescribed || aSpec->isInserted)
    {
        return;
    }

    const auto recordID = Red::TweakDBUtil::GetRTDBRecordID(aSpec->shortName);

    for (const auto& prop : aSpec->props | std::views::values)
    {
        if (!prop->isDescribed)
            continue;

        const auto flatID = recordID + std::string_view(".") + prop->name;
        auto instance = prop->defaultValue;

        if (!instance)
        {
            instance = Red::TweakDBUtil::Construct(prop->typeSpec->flatType);
        }

        if (!m_tweakManager->SetFlat(flatID, prop->typeSpec->flatType, instance.get()))
        {
            LogError("Failed to insert default value for property {} of record type {} into TweakDB.", prop->name,
                     aSpec->name);
        }
    }

    if (aSpec->type->parent)
    {
        InsertScriptableRecordDefaults(aSpec->type->parent);
    }

    aSpec->isInserted = true;
}

void ScriptableRecordManager::InsertScriptableRecordDefaults(const Red::CClass* aClass)
{
    if (!aClass)
    {
        return;
    }

    if (const auto spec = GetRecordSpec(aClass->GetName()))
    {
        InsertScriptableRecordDefaults(spec);
    }
}

bool ScriptableRecordManager::UnregisterScriptableRecordSpec(const Core::SharedPtr<ScriptableRecordSpec>& aSpec)
{
    if (!aSpec->type)
    {
        LogDebug("Record type {} is not registered, skipping unregistration.", aSpec->shortName);
        return false;
    }

    auto* type = aSpec->type;

    for (const auto& prop : aSpec->props | std::views::values)
    {
        prop->isDescribed = false;
    }

    aSpec->type = nullptr;
    aSpec->isRegistered = false;
    aSpec->isDescribed = false;

    return DestroyRecordClass(type);
}

Red::CName ScriptableRecordManager::RegisterPropertyFunctionName(const std::string& aName)
{
    return Red::CNamePool::Add(Red::TweakDBUtil::Capitalize(aName).c_str());
}

ScriptableRecordClass* ScriptableRecordManager::GetRecordClass(const uint32_t aHash) const
{
    std::shared_lock lockR(m_classesMutex);
    if (const auto it = m_classes.find(aHash); it != m_classes.end())
    {
        return it->second.get();
    }
    return nullptr;
}

ScriptableRecordClass* ScriptableRecordManager::CreateRecordClass(const Core::SharedPtr<ScriptableRecordSpec>& aSpec)
{
    if (GetRecordClass(aSpec->hash))
    {
        LogError("Failed to create RTTI class for record type {} because its specification was not found.",
                 aSpec->shortName);
        return nullptr;
    }

    if (m_rtti->GetClass(aSpec->cname))
    {
        LogError("Failed to create RTTI class for record type {} because a class with the same name already exists.",
                 aSpec->shortName);
        return nullptr;
    }

    const auto cls = Core::MakeShared<ScriptableRecordClass>(aSpec->cname, aSpec->hash);

    m_rtti->RegisterType(cls.get());
    m_rtti->RegisterScriptName(aSpec->cname, aSpec->aliasCName);

    {
        std::unique_lock lockRW(m_classesMutex);
        m_classes[cls->tweakBaseHash] = cls;
    }

    return cls.get();
}

bool ScriptableRecordManager::DestroyRecordClass(ScriptableRecordClass* aClass)
{
    std::unique_lock lockRW(m_classesMutex);

    if (const auto it = m_classes.find(aClass->tweakBaseHash); it != m_classes.end())
    {
        m_rtti->UnregisterType(aClass);
        m_classes.erase(it);
        return true;
    }

    LogWarning("Failed to destroy RTTI class for record type {} because it was not found in the manager's registry.",
               aClass->GetName().ToString());
    return false;
}

bool ScriptableRecordManager::CreateScriptableRecord(Red::TweakDB* aTweakDB, const uint32_t aHash,
                                                     Red::TweakDBID aRecordId)
{
    if (const auto cls = GetRecordClass(aHash))
    {
        return CreateScriptableRecord(aTweakDB, cls, aRecordId);
    }
    return false;
}

bool ScriptableRecordManager::CreateScriptableRecord(Red::TweakDB* aTweakDB, ScriptableRecordClass* aClass,
                                                     Red::TweakDBID aRecordId)
{
    if (!aClass || !aTweakDB)
    {
        LogError("Failed to create scriptable record because the provided class or TweakDB instance is null.");
        return false;
    }

    if (const auto instance = Red::MakeScriptedHandle<ScriptableTweakDBRecord>(aClass))
    {
        instance->recordID = aRecordId;
        instance->nativeType = aClass;

        Raw::InsertRecord(aTweakDB, aRecordId, aClass, instance);

        return true;
    }

    LogError("Failed to create an instance of scriptable record type {}.", aClass->GetName().ToString());
    return false;
}

void ScriptableRecordManager::DispatchNoArgGetter(Red::IScriptable* aContext, Red::CStackFrame* aFrame, void* aOut,
                                                  const int64_t a4)
{
    (void)a4;

    if (!aContext || !aFrame || !aFrame->code || !aOut)
        return;

    // Bytecode layout appends ParamEnd followed by baked Context*.
    if (static_cast<uint8_t>(*aFrame->code) != 38)
        return;

    aFrame->code++; // Skip ParamEnd

    auto* context = *reinterpret_cast<Context**>(aFrame->code);
    aFrame->code += sizeof(Context*);

    const auto* record = static_cast<ScriptableTweakDBRecord*>(aContext);
    const auto value = context->tweakManager->GetFlat(record->recordID + context->appendix);

    if (!value)
        return;

    Red::ValuePtr<> result;

    if (!result)
        return;

    result->type->Assign(aOut, result->instance);
}

#ifndef NDEBUG

void ScriptableRecordManager::RegisterTestScriptableRecord()
{
    const auto name = RegisterScriptableRecordType(Red::TweakDBUtil::NormalizeRecordName("TweakXLTest"));
    const auto info = GetTweakPropertySpec("CName");

    RegisterScriptableProperty(name, "foo", info);
    RegisterScriptableProperty(name, "bar", info);
}

void ScriptableRecordManager::TestScriptableRecord()
{
    using recordType = Red::TypeLocator<"gamedataTweakXLTest_Record">;
    using cnameType = Red::TypeLocator<"CName">;

    static auto recordID = Red::TweakDBID{"test.tweakxl.scriptable"};
    static auto fooValue = Red::CNamePool::Add("test foo value");
    static auto barValue = Red::CNamePool::Add("test bar value");
    static auto fooAppendix = std::string_view(".foo");
    static auto barAppendix = std::string_view(".bar");

    assert(m_tweakManager->CreateRecord(recordID, recordType::GetClass()));
    assert(m_tweakManager->SetFlat(recordID + fooAppendix, cnameType::GetClass(), &fooValue));
    assert(m_tweakManager->SetFlat(recordID + barAppendix, cnameType::GetClass(), &barValue));

    const auto record =
        reinterpret_cast<ScriptableTweakDBRecord*>(m_tweakManager->GetTweakDB()->GetRecord(recordID).instance);

    assert(record);

    {
        auto* func = recordType::GetClass()->GetFunction("Foo");
        Red::CName result;
        assert(Red::ExecuteFunction(record, func, &result));
        assert(result && strcmp(result.ToString(), "test foo value") == 0);
    }

    {
        auto* func = recordType::GetClass()->GetFunction("Bar");
        Red::CName result;
        assert(Red::ExecuteFunction(record, func, &result));
        assert(result && strcmp(result.ToString(), "test bar value") == 0);
    }
}

#endif
//
// void ScriptableRecordManager::DispatchNoArgGetter(Red::IScriptable* aContext, Red::CStackFrame* aFrame, void* aOut,
//                                                   int64_t a4)
// {
//     aFrame->code++; // Skip ParamEnd operand
//
//     const Context* ctx = *reinterpret_cast<const Context**>(aFrame->code);
//     aFrame->code += sizeof(const Context*); // Move past ctx pointer
//
//     if (!aContext || !aOut)
//         return;
//
//     const auto* record = static_cast<ScriptableTweakDBRecord*>(aContext);
//     const auto& [appendix, typeInfo, recordManager, tweakManager] = *ctx;
//
//     if (!typeInfo || !typeInfo->propertyType || !typeInfo->flatType)
//     {
//         return;
//     }
//
//     const auto value = tweakManager->GetFlat(record->recordID + appendix);
//
//     if (!value)
//     {
//         return;
//     }
//
//     if (value.type == typeInfo->propertyType)
//     {
//         value.type->Assign(aOut, value.instance);
//         return;
//     }
//
//     if (value.type != typeInfo->flatType || !typeInfo->isForeignKey)
//     {
//         LogError(
//             "Type mismatch when retrieving property value for {}. Expected property type {} or flat type {}, got
//             {}.", appendix, typeInfo->propertyType->GetName().ToString(), typeInfo->flatType->GetName().ToString(),
//             value.type->GetName().ToString());
//         return;
//     }
//
//     Red::ValuePtr<> converted;
//
//     switch (typeInfo->propertyType->GetType())
//     {
//     case Red::ERTTIType::Array:
//         converted = recordManager->ConvertValue<Red::ERTTIType::Array>(value, typeInfo);
//         break;
//     case Red::ERTTIType::Handle:
//         converted = recordManager->ConvertValue<Red::ERTTIType::Handle>(value, typeInfo);
//         break;
//     case Red::ERTTIType::WeakHandle:
//         converted = recordManager->ConvertValue<Red::ERTTIType::WeakHandle>(value, typeInfo);
//         break;
//     default:
//         LogError("Unsupported foreign-key return type {} for {}.", typeInfo->propertyType->GetName().ToString(),
//                  appendix);
//         return;
//     }
//
//     if (!converted)
//     {
//         LogError("Failed to convert foreign-key value for {} from {} to {}.", appendix,
//                  value.type->GetName().ToString(), typeInfo->propertyType->GetName().ToString());
//         return;
//     }
//
//     typeInfo->propertyType->Assign(aOut, converted->instance);
// }
//
// template<>
// Red::ValuePtr<> ScriptableRecordManager::ConvertValue<Red::ERTTIType::Array>(const Red::Value<>& aValue,
//                                                                              const TweakPropertySpecPtr& aTypeSpec)
// {
//     if (!aTypeSpec || !aTypeSpec->propertyType || !aTypeSpec->foreignType || !aTypeSpec->isForeignKey ||
//         !aTypeSpec->isArray || !aValue || aValue.type->GetName() != Red::ERTDBFlatType::TweakDBIDArray)
//     {
//         return {};
//     }
//
//     if (aTypeSpec->propertyType->GetType() != Red::ERTTIType::Array)
//         return {};
//
//     auto* arrayType = reinterpret_cast<const Red::CRTTIBaseArrayType*>(aTypeSpec->propertyType);
//     const auto* innerType = arrayType->GetInnerType();
//
//     if (!innerType ||
//         (innerType->GetType() != Red::ERTTIType::Handle && innerType->GetType() != Red::ERTTIType::WeakHandle))
//     {
//         return {};
//     }
//
//     const auto* ids = static_cast<const Red::DynArray<Red::TweakDBID>*>(aValue.instance);
//     auto converted = Red::MakeValue(aTypeSpec->propertyType);
//
//     for (uint32_t i = 0; i < ids->Size(); ++i)
//     {
//         arrayType->InsertAt(converted->instance, static_cast<int32_t>(i));
//         auto* dst = arrayType->GetElement(converted->instance, i);
//         const auto record = m_tweakManager->GetRecord(ids->At(i));
//
//         if (innerType->GetType() == Red::ERTTIType::Handle)
//         {
//             Red::Handle<Red::TweakDBRecord> handle{};
//             if (record && record->GetType()->IsA(aTypeSpec->foreignType))
//                 handle = record;
//
//             innerType->Assign(dst, &handle);
//         }
//         else
//         {
//             Red::WeakHandle<Red::TweakDBRecord> weakHandle{};
//             if (record && record->GetType()->IsA(aTypeSpec->foreignType))
//                 weakHandle = record;
//
//             innerType->Assign(dst, &weakHandle);
//         }
//     }
//
//     return converted;
// }
//
// template<>
// Red::ValuePtr<> ScriptableRecordManager::ConvertValue<Red::ERTTIType::Handle>(const Red::Value<>& aValue,
//                                                                               const TweakPropertySpecPtr& aTypeSpec)
// {
//     if (!aTypeSpec || !aTypeSpec->propertyType || !aTypeSpec->foreignType || !aTypeSpec->isForeignKey ||
//         aTypeSpec->isArray || !aValue || aValue.type->GetName() != Red::ERTDBFlatType::TweakDBID ||
//         aTypeSpec->propertyType->GetType() != Red::ERTTIType::Handle)
//     {
//         return {};
//     }
//
//     const auto& id = *static_cast<const Red::TweakDBID*>(aValue.instance);
//     const auto record = m_tweakManager->GetRecord(id);
//
//     if (!record || !record->GetType()->IsA(aTypeSpec->foreignType))
//         return {};
//
//     auto converted = Red::MakeValue(aTypeSpec->propertyType);
//     aTypeSpec->propertyType->Assign(converted->instance, &record);
//     return converted;
// }
//
// template<>
// Red::ValuePtr<> ScriptableRecordManager::ConvertValue<Red::ERTTIType::WeakHandle>(const Red::Value<>& aValue,
//                                                                                   const TweakPropertySpecPtr&
//                                                                                   aTypeSpec)
// {
//     if (!aTypeSpec || !aTypeSpec->propertyType || !aTypeSpec->foreignType || !aTypeSpec->isForeignKey ||
//         aTypeSpec->isArray || !aValue || aValue.type->GetName() != Red::ERTDBFlatType::TweakDBID ||
//         aTypeSpec->propertyType->GetType() != Red::ERTTIType::WeakHandle)
//     {
//         return {};
//     }
//
//     const auto& id = *static_cast<const Red::TweakDBID*>(aValue.instance);
//     const auto record = m_tweakManager->GetRecord(id);
//
//     if (!record || !record->GetType()->IsA(aTypeSpec->foreignType))
//         return {};
//
//     auto converted = Red::MakeValue(aTypeSpec->propertyType);
//     const Red::WeakHandle weakHandle = record;
//     aTypeSpec->propertyType->Assign(converted->instance, &weakHandle);
//     return converted;
// }

} // namespace App
