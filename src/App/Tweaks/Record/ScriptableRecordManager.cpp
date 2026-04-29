#include "ScriptableRecordManager.hpp"

#include "App/Tweaks/TweakService.hpp"
#include "Core/Facades/Container.hpp"
#include "ScriptableTweakDBRecord.hpp"

namespace App
{
ScriptableRecordManager::ScriptableRecordManager()
    : m_rtti(Red::CRTTISystem::Get())
{
}

ScriptableRecordManager* ScriptableRecordManager::Get()
{
    static auto instance = ScriptableRecordManager();
    return &instance;
}

ScriptableRecordManager::~ScriptableRecordManager()
{
    for (auto& spec : m_specs | std::views::values)
    {
        UnregisterScriptableRecordSpec(spec);
    }

    for (const auto& entry : m_closures)
    {
        DestroyClosure(entry);
    }
}

// TODO: Handle additional functions depending on the type. See Reflection.cpp during introspection for skipped
// functions.
Red::ScriptingFunction_t<void*> ScriptableRecordManager::CreateClosure(const Context& aContext)
{
    std::scoped_lock lock(m_closuresMutex);

    if (!m_cifReady)
    {
        if (ffi_prep_cif(&m_cif, FFI_DEFAULT_ABI, static_cast<unsigned>(m_argTypes.size()), &ffi_type_void,
                         m_argTypes.data()) != FFI_OK)
        {
            return nullptr;
        }

        m_cifReady = true;
    }

    auto entry = Core::MakeUnique<Closure>();
    entry->context = aContext;
    entry->closure = static_cast<ffi_closure*>(ffi_closure_alloc(sizeof(ffi_closure), &entry->executable));

    if (!entry->closure)
    {
        return nullptr;
    }

    if (ffi_prep_closure_loc(entry->closure, &m_cif, &FfiDispatch, &entry->context, entry->executable) != FFI_OK)
    {
        ffi_closure_free(entry->closure);
        return nullptr;
    }

    const auto function = reinterpret_cast<Red::ScriptingFunction_t<void*>>(entry->executable);
    m_closures.emplace_back(std::move(entry));
    return function;
}

Red::ScriptingFunction_t<void*> ScriptableRecordManager::CreateClosure(const std::string& aAppendix,
                                                                       const TweakPropertySpecPtr& aTypeInfo)
{
    return CreateClosure({aAppendix, aTypeInfo});
}

bool ScriptableRecordManager::DestroyClosure(const Core::SharedPtr<Closure>& aClosure)
{
    if (aClosure)
    {
        return DestroyClosure(aClosure->closure);
    }
    return false;
}

bool ScriptableRecordManager::DestroyClosure(const ffi_closure* aClosure)
{
    if (!aClosure)
    {
        return false;
    }

    std::scoped_lock lock(m_closuresMutex);

    for (auto it = m_closures.begin(); it != m_closures.end(); ++it)
    {
        const auto& entry = *it;

        if (!entry || !entry->closure)
        {
            continue;
        }

        if (entry->closure != aClosure)
        {
            continue;
        }

        ffi_closure_free(entry->closure);
        entry->closure = nullptr;
        entry->executable = nullptr;
        m_closures.erase(it);

        return true;
    }

    return false;
}

void ScriptableRecordManager::FfiDispatch(ffi_cif* aCif, void* aRet, void** aArgs, void* aUserData)
{
    (void)aCif;
    (void)aRet;

    const auto* context = static_cast<Context*>(aUserData);

    if (!context)
    {
        return;
    }

    auto* instance = *static_cast<Red::IScriptable**>(aArgs[0]);
    auto* stackFrame = *static_cast<Red::CStackFrame**>(aArgs[1]);
    auto* out = *static_cast<void**>(aArgs[2]);

    if (!instance || !stackFrame || !out)
    {
        return;
    }

    stackFrame->code++;

    const auto* record = reinterpret_cast<ScriptableTweakDBRecord*>(instance);
    const auto& [appendix, typeInfo] = *context;

    if (!typeInfo || !typeInfo->propertyType || !typeInfo->flatType)
    {
        return;
    }

    const auto svc = Core::Resolve<TweakService>();

    if (!svc)
    {
        return;
    }

    auto& tweakDbManager = svc->GetManager();
    const auto value = tweakDbManager.GetFlat(record->recordID + appendix);

    if (!value)
    {
        return;
    }

    if (value.type == typeInfo->propertyType)
    {
        value.type->Assign(out, value.instance);
        return;
    }

    if (value.type != typeInfo->flatType || !typeInfo->isForeignKey)
    {
        LogError(
            "Type mismatch when retrieving property value for {}. Expected property type {} or flat type {}, got {}.",
            appendix, typeInfo->propertyType->GetName().ToString(), typeInfo->flatType->GetName().ToString(),
            value.type->GetName().ToString());
        return;
    }

    auto* scriptableRecordManager = Get();
    Red::ValuePtr<> converted;

    switch (typeInfo->propertyType->GetType())
    {
    case Red::ERTTIType::Array:
        converted = scriptableRecordManager->ConvertValue<Red::ERTTIType::Array>(value, typeInfo, svc);
        break;
    case Red::ERTTIType::Handle:
        converted = scriptableRecordManager->ConvertValue<Red::ERTTIType::Handle>(value, typeInfo, svc);
        break;
    case Red::ERTTIType::WeakHandle:
        converted = scriptableRecordManager->ConvertValue<Red::ERTTIType::WeakHandle>(value, typeInfo, svc);
        break;
    default:
        LogError("Unsupported foreign-key return type {} for {}.", typeInfo->propertyType->GetName().ToString(),
                 appendix);
        return;
    }

    if (!converted)
    {
        LogError("Failed to convert foreign-key value for {} from {} to {}.", appendix,
                 value.type->GetName().ToString(), typeInfo->propertyType->GetName().ToString());
        return;
    }

    typeInfo->propertyType->Assign(out, converted->instance);
}

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
                                                               const TweakPropertySpecPtr& aTypeInfo,
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
    propertyInfo->typeInfo = aTypeInfo;
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

void ScriptableRecordManager::InsertScriptableRecordDefaults(const Core::SharedPtr<Red::TweakDBManager>& aManager)
{
    for (const auto& spec : m_specs | std::views::values)
    {
        InsertScriptableRecordDefaults(spec, aManager);
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

    const auto& typeInfo = aSpec->typeInfo;

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

    const auto closure = CreateClosure(aSpec->appendix, typeInfo);

    if (!closure)
    {
        return false;
    }

    const auto name = RegisterPropertyFunctionName(aSpec->name);
    auto* function = Red::CClassFunction::Create(aClass, name.ToString(), name.ToString(), closure);
    function->SetReturnType(typeInfo->propertyTypeName);
    aClass->RegisterFunction(function);

    aSpec->isDescribed = true;
    return true;
}

void ScriptableRecordManager::InsertScriptableRecordDefaults(const Core::SharedPtr<ScriptableRecordSpec>& aSpec,
                                                             const Core::SharedPtr<Red::TweakDBManager>& aManager)
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
            instance = Red::TweakDBUtil::Construct(prop->typeInfo->flatType);
        }

        if (!aManager->SetFlat(flatID, prop->typeInfo->flatType, instance.get()))
        {
            LogError("Failed to insert default value for property {} of record type {} into TweakDB.", prop->name,
                     aSpec->name);
        }
    }

    if (aSpec->type->parent)
    {
        InsertScriptableRecordDefaults(aSpec->type->parent, aManager);
    }

    aSpec->isInserted = true;
}

void ScriptableRecordManager::InsertScriptableRecordDefaults(const Red::CClass* aClass,
                                                             const Core::SharedPtr<Red::TweakDBManager>& aManager)
{
    if (!aClass)
    {
        return;
    }

    if (const auto spec = GetRecordSpec(aClass->GetName()))
    {
        InsertScriptableRecordDefaults(spec, aManager);
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
    LogWarning("Failed to create scriptable record because the corresponding record class with hash {} was not found.",
               aHash);
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

#ifndef NDEBUG

void ScriptableRecordManager::RegisterTestScriptableRecord()
{
    const auto name = RegisterScriptableRecordType(Red::TweakDBUtil::NormalizeRecordName("TweakXLTest"));
    const auto info = GetTweakPropertySpec("CName");

    RegisterScriptableProperty(name, "foo", info);
    RegisterScriptableProperty(name, "bar", info);
}

void ScriptableRecordManager::TestScriptableRecord(const Core::SharedPtr<Red::TweakDBManager>& aManager)
{
    using recordType = Red::TypeLocator<"gamedataTweakXLTest_Record">;
    using cnameType = Red::TypeLocator<"CName">;

    static auto recordID = Red::TweakDBID{"test.tweakxl.scriptable"};
    static auto fooValue = Red::CNamePool::Add("test foo value");
    static auto barValue = Red::CNamePool::Add("test bar value");
    static auto fooAppendix = std::string_view(".foo");
    static auto barAppendix = std::string_view(".bar");

    assert(aManager->CreateRecord(recordID, recordType::GetClass()));
    assert(aManager->SetFlat(recordID + fooAppendix, cnameType::GetClass(), &fooValue));
    assert(aManager->SetFlat(recordID + barAppendix, cnameType::GetClass(), &barValue));

    const auto record =
        reinterpret_cast<ScriptableTweakDBRecord*>(aManager->GetTweakDB()->GetRecord(recordID).instance);

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

template<>
Red::ValuePtr<> ScriptableRecordManager::ConvertValue<Red::ERTTIType::Array>(
    const Red::Value<>& aValue, const TweakPropertySpecPtr& aTypeInfo, const Core::SharedPtr<TweakService>& aService)
{
    if (!aTypeInfo || !aService || !aTypeInfo->propertyType || !aTypeInfo->foreignType || !aTypeInfo->isForeignKey ||
        !aTypeInfo->isArray || !aValue || aValue.type->GetName() != Red::ERTDBFlatType::TweakDBIDArray)
    {
        return {};
    }

    if (aTypeInfo->propertyType->GetType() != Red::ERTTIType::Array)
        return {};

    auto* arrayType = reinterpret_cast<const Red::CRTTIBaseArrayType*>(aTypeInfo->propertyType);
    const auto* innerType = arrayType->GetInnerType();

    if (!innerType ||
        (innerType->GetType() != Red::ERTTIType::Handle && innerType->GetType() != Red::ERTTIType::WeakHandle))
    {
        return {};
    }

    const auto* ids = static_cast<const Red::DynArray<Red::TweakDBID>*>(aValue.instance);
    auto converted = Red::MakeValue(aTypeInfo->propertyType);

    for (uint32_t i = 0; i < ids->Size(); ++i)
    {
        arrayType->InsertAt(converted->instance, static_cast<int32_t>(i));
        auto* dst = arrayType->GetElement(converted->instance, i);
        const auto record = aService->GetManager().GetRecord(ids->At(i));

        if (innerType->GetType() == Red::ERTTIType::Handle)
        {
            Red::Handle<Red::TweakDBRecord> handle{};
            if (record && record->GetType()->IsA(aTypeInfo->foreignType))
                handle = record;

            innerType->Assign(dst, &handle);
        }
        else
        {
            Red::WeakHandle<Red::TweakDBRecord> weakHandle{};
            if (record && record->GetType()->IsA(aTypeInfo->foreignType))
                weakHandle = record;

            innerType->Assign(dst, &weakHandle);
        }
    }

    return converted;
}

template<>
Red::ValuePtr<> ScriptableRecordManager::ConvertValue<Red::ERTTIType::Handle>(
    const Red::Value<>& aValue, const TweakPropertySpecPtr& aTypeInfo, const Core::SharedPtr<TweakService>& aService)
{
    if (!aTypeInfo || !aService || !aTypeInfo->propertyType || !aTypeInfo->foreignType || !aTypeInfo->isForeignKey ||
        aTypeInfo->isArray || !aValue || aValue.type->GetName() != Red::ERTDBFlatType::TweakDBID ||
        aTypeInfo->propertyType->GetType() != Red::ERTTIType::Handle)
    {
        return {};
    }

    const auto& id = *static_cast<const Red::TweakDBID*>(aValue.instance);
    const auto record = aService->GetManager().GetRecord(id);

    if (!record || !record->GetType()->IsA(aTypeInfo->foreignType))
        return {};

    auto converted = Red::MakeValue(aTypeInfo->propertyType);
    const Red::Handle<Red::TweakDBRecord> handle = record;
    aTypeInfo->propertyType->Assign(converted->instance, &handle);
    return converted;
}

template<>
Red::ValuePtr<> ScriptableRecordManager::ConvertValue<Red::ERTTIType::WeakHandle>(
    const Red::Value<>& aValue, const TweakPropertySpecPtr& aTypeInfo, const Core::SharedPtr<TweakService>& aService)
{
    if (!aTypeInfo || !aService || !aTypeInfo->propertyType || !aTypeInfo->foreignType || !aTypeInfo->isForeignKey ||
        aTypeInfo->isArray || !aValue || aValue.type->GetName() != Red::ERTDBFlatType::TweakDBID ||
        aTypeInfo->propertyType->GetType() != Red::ERTTIType::WeakHandle)
    {
        return {};
    }

    const auto& id = *static_cast<const Red::TweakDBID*>(aValue.instance);
    const auto record = aService->GetManager().GetRecord(id);

    if (!record || !record->GetType()->IsA(aTypeInfo->foreignType))
        return {};

    auto converted = Red::MakeValue(aTypeInfo->propertyType);
    const Red::WeakHandle weakHandle = record;
    aTypeInfo->propertyType->Assign(converted->instance, &weakHandle);
    return converted;
}

} // namespace App