#include "ScriptableRecordManager.hpp"

#include "App/Tweaks/TweakService.hpp"
#include "Core/Facades/Container.hpp"
#include "Red/TweakDB/Source/Source.hpp"
#include "ScriptableTweakDBRecord.hpp"

namespace
{
Red::TweakDBID BuildRTDBID(const std::string& aRecordName)
{
    std::string id = Red::TweakSource::SchemaPackage;
    id.append(".");
    id.append(aRecordName);
    return Red::TweakDBID{id};
}
} // namespace

namespace App
{
ScriptableRecordManager::ScriptableRecordManager()
    : m_rtti(Red::CRTTISystem::Get())
{
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

Red::ScriptingFunction_t<void*> ScriptableRecordManager::CreateClosure(const std::string& aAppendix,
                                                                       const Red::CBaseRTTIType* aType)
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
    entry->context.appendix = aAppendix;
    entry->context.type = aType;

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

    auto* instance = *static_cast<Red::IScriptable**>(aArgs[0]);
    auto* stackFrame = *static_cast<Red::CStackFrame**>(aArgs[1]);
    auto* out = *static_cast<void**>(aArgs[2]);

    if (!instance || !stackFrame || !out)
    {
        return;
    }

    stackFrame->code++;

    const auto* record = reinterpret_cast<ScriptableTweakDBRecord*>(instance);

    if (const auto svc = Core::Resolve<App::TweakService>())
    {
        if (const auto value = svc->GetManager().GetFlat(record->recordID + context->appendix))
        {
            if (value.type == context->type)
            {
                value.type->Assign(out, value);
            }
            else
            {
                // TODO: log error
            }
        }
    }
}

Red::CName ScriptableRecordManager::RegisterScriptableRecordType(const std::string& aName,
                                                                 const std::optional<std::string>& aParentName)
{
    if (const auto cname = Red::TweakDBUtil::GetRecordShortName<Red::CName>(aName); GetRecordSpec(cname))
    {
        std::shared_lock lockR(m_specsMutex);
        if (const auto it = m_specs.find(Red::CName(aName.c_str())); it != m_specs.end())
        {
            return {};
        }
    }

    if (m_rtti->GetClass(Red::CName(aName.c_str())))
    {
        return {};
    }

    const auto spec = Core::MakeShared<ScriptableRecordSpec>();

    spec->name = Red::TweakDBUtil::GetRecordFullName<std::string>(aName);
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
                                                               const uint64_t aFlatType,
                                                               const std::optional<std::string>& aForeignType)
{
    const auto cname = Red::CName{aPropertyName.c_str()};

    const auto recordInfo = GetRecordSpec(aRecordName);

    if (!recordInfo)
        return {};

    if (recordInfo->props.contains(cname))
    {
        return {};
    }

    if (!Red::TweakDBUtil::IsFlatType(aFlatType))
    {
        return {};
    }

    const auto propertyInfo = Core::MakeShared<ScriptablePropertySpec>();
    propertyInfo->name = aPropertyName;
    propertyInfo->appendix = "." + aPropertyName;
    propertyInfo->type = aFlatType;
    propertyInfo->foreignName = aForeignType;
    propertyInfo->cname = Red::CName{aPropertyName.c_str()};

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
        // TODO: log warning
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
        // TODO: log warning
        return false;
    }

    if (aSpec->parent.has_value())
    {
        auto* parentCls = m_rtti->GetClass(Red::TweakDBUtil::GetRecordFullName<Red::CName>(aSpec->parent.value()));

        if (!parentCls || !Red::TweakDBUtil::IsRecordType(parentCls))
        {
            // TODO: log error
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
        return false;
    }

    const auto* type = Red::TweakDBUtil::GetType(aSpec->type);

    if (!type)
    {
        return false;
    }

    const auto closure = CreateClosure(aSpec->name, type);

    if (!closure)
    {
        return false;
    }

    if (aSpec->foreignName.has_value())
    {
        aSpec->foreignType = m_rtti->GetClass(Red::TweakDBUtil::GetRecordFullName<Red::CName>(*aSpec->foreignName));
    }

    // TODO: FKs are wrong and I need to fix
    const auto name = RegisterPropertyFunctionName(aSpec->name);
    auto* function = Red::CClassFunction::Create(aClass, name.ToString(), name.ToString(), closure);
    function->SetReturnType(type->GetName());
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

    const auto recordID = BuildRTDBID(aSpec->shortName);

    CreateScriptableRecord(aManager->GetTweakDB(), aSpec->type, recordID);

    for (const auto& prop : aSpec->props | std::views::values)
    {
        // TODO: default values other than empty
        const auto flatID = recordID + std::string_view(".") + prop->name;
        auto instance = Red::MakeValue(prop->type);
        [[maybe_unused]] bool success = aManager->SetFlat(flatID, *instance);
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
    std::string name = aName;
    name[0] = static_cast<char>(std::toupper(name[0]));
    return Red::CNamePool::Add(name.c_str());
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
        // TODO: log warning
        return nullptr;
    }

    if (m_rtti->GetClass(aSpec->cname))
    {
        // TODO: log warning
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
        return false;
    }

    if (const auto instance = Red::MakeScriptedHandle<ScriptableTweakDBRecord>(aClass))
    {
        instance->recordID = aRecordId;
        instance->nativeType = aClass;

        Raw::InsertRecord(aTweakDB, aRecordId, aClass, instance);

        return true;
    }

    return false;
}

#ifndef NDEBUG

void ScriptableRecordManager::RegisterTestScriptableRecord()
{
    const auto name = RegisterScriptableRecordType("TweakXLTest");
    RegisterScriptableProperty(name, "foo", Red::ERTDBFlatType::CName);
    RegisterScriptableProperty(name, "bar", Red::ERTDBFlatType::CName);
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

} // namespace App