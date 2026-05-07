#include "ScriptableRecordManager.hpp"

#include "App/Tweaks/TweakService.hpp"
#include "App/Tweaks/TweakTypeSpec.hpp"
#include "ScriptablePropertyHandler.hpp"
#include "ScriptableRecordClass.hpp"
#include "ScriptableTweakDBRecord.hpp"

namespace App
{
ScriptablePropertySpecPtr ScriptableRecordSpec::FindPropertyByFunctionName(const std::string& aFunctionName) const
{
    for (const auto& prop : props | std::views::values)
    {
        if (prop->functionName == aFunctionName)
            return prop;
    }

    return nullptr;
}

ScriptableRecordManager::ScriptableRecordManager(const Core::DeferredPtr<Red::TweakDBManager>& aManager)
    : m_rtti(Red::CRTTISystem::Get())
    , m_tweakManager(aManager)
    , m_handlers(Core::MakeShared<ScriptablePropertyHandlers>(aManager))
{
}

ScriptableRecordManager::~ScriptableRecordManager()
{
    for (auto& spec : m_specs | std::views::values)
        DeregisterScriptableRecord(spec);
}

Core::Vector<ScriptableRecordSpecPtr> ScriptableRecordManager::GetRecordSpecs() const
{
    Core::Vector<ScriptableRecordSpecPtr> vector;

    std::shared_lock lock(m_specsMutex);
    vector.reserve(m_specs.size());
    for (const auto& spec : m_specs | std::views::values)
        vector.emplace_back(spec);

    return vector;
}

bool ScriptableRecordManager::CreateScriptableRecord(Red::TweakDB* aTweakDB, const uint32_t aHash,
                                                     const Red::TweakDBID aRecordId)
{
    if (const auto cls = GetRecordClass(aHash))
        return CreateScriptableRecord(aTweakDB, cls, aRecordId);

    return false;
}

bool ScriptableRecordManager::CreateScriptableRecord(Red::TweakDB* aTweakDB, ScriptableRecordClass* aClass,
                                                     const Red::TweakDBID aRecordId)
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
        return spec->cname;
    }
}

Red::CName ScriptableRecordManager::RegisterScriptableProperty(Red::CName aRecordName, const std::string& aPropertyName,
                                                               const TweakTypeSpecPtr& aTypeSpec,
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
        return {};

    const auto propertyInfo = Core::MakeShared<ScriptablePropertySpec>();
    propertyInfo->name = aPropertyName;
    propertyInfo->functionName = Red::TweakDBUtil::Capitalize(aPropertyName);
    propertyInfo->appendix = "." + aPropertyName;
    propertyInfo->typeSpec = aTypeSpec;
    propertyInfo->cname = Red::CName{aPropertyName.c_str()};
    propertyInfo->defaultValue = aDefaultValue;

    recordInfo->props[propertyInfo->cname] = propertyInfo;

    return propertyInfo->cname;
}

void ScriptableRecordManager::RegisterRTTITypes()
{
    // TODO: move this elsewhere, really.
    m_handlers->RegisterInvocationHandler();

#ifndef NDEBUG
    RegisterTestScriptableRecord();
#endif

    for (auto& spec : m_specs | std::views::values)
    {
        RegisterScriptableRecordSpec(spec);
    }
}

void ScriptableRecordManager::DescribeRTTITypes()
{
    for (auto& spec : m_specs | std::views::values)
        DescribeRTTIType(spec);
}

void ScriptableRecordManager::InsertDefaults()
{
    for (const auto& spec : m_specs | std::views::values)
        InsertDefaults(spec);
}

void ScriptableRecordManager::AdaptScriptClasses(const Red::DynArray<Red::ScriptClass*>& aClasses)
{
    for (const auto& classDef : aClasses)
        AdaptScriptClass(classDef);
}

ScriptableRecordSpecPtr ScriptableRecordManager::GetRecordSpec(Red::CName aName) const
{
    std::shared_lock lockR(m_specsMutex);
    if (const auto it = m_specs.find(aName); it != m_specs.end())
        return it->second;

    return nullptr;
}

bool ScriptableRecordManager::RegisterScriptableRecordSpec(const ScriptableRecordSpecPtr& aSpec)
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

bool ScriptableRecordManager::DescribeRTTIType(const ScriptableRecordSpecPtr& aSpec)
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

    Red::CNamePool::Add(Red::TweakDBUtil::GetHandleTypeName<std::string>(aSpec->type).c_str());
    Red::CNamePool::Add(Red::TweakDBUtil::GetWHandleTypeName<std::string>(aSpec->type).c_str());
    Red::CNamePool::Add(Red::TweakDBUtil::GetWHandleArrayTypeName<std::string>(aSpec->type).c_str());

    for (const auto& prop : aSpec->props | std::views::values)
        RegisterRTTIProperty(aSpec, prop);

    aSpec->isDescribed = true;
    return true;
}

bool ScriptableRecordManager::RegisterRTTIProperty(const ScriptableRecordSpecPtr& aRecordSpec,
                                                   const ScriptablePropertySpecPtr& aPropSpec)
{
    if (aPropSpec->isDescribed)
    {
        LogDebug("Property {} of record type {} is already described, skipping description.", aPropSpec->name,
                 aRecordSpec->type->GetName().ToString());
        return false;
    }

    const auto& typeSpec = aPropSpec->typeSpec;

    if (typeSpec->isForeignKey)
    {
        if (!typeSpec->foreignType)
        {
            if (const auto* type = m_rtti->GetClass(typeSpec->foreignTypeName))
            {
                typeSpec->foreignType = type;
            }
            else
            {
                LogError("Failed to describe property {} of record type {}, the foreign type {} does not exist.",
                         aPropSpec->name, aRecordSpec->type->GetName().ToString(), typeSpec->foreignName);
                return false;
            }
        }

        Red::CNamePool::Add(Red::TweakDBUtil::GetHandleTypeName<std::string>(typeSpec->foreignType).c_str());
        Red::CNamePool::Add(Red::TweakDBUtil::GetWHandleTypeName<std::string>(typeSpec->foreignType).c_str());
        Red::CNamePool::Add(Red::TweakDBUtil::GetWHandleArrayTypeName<std::string>(typeSpec->foreignType).c_str());
    }

    if (!typeSpec->propertyType)
    {
        if (const auto* type = m_rtti->GetType(typeSpec->propertyTypeName))
        {
            typeSpec->propertyType = type;
        }
        else
        {
            LogError("Failed to describe property {} of record type {}, the property type {} does not exist.",
                     aPropSpec->name, aRecordSpec->type->GetName().ToString(), typeSpec->foreignName);
            return false;
        }
    }

    m_handlers->RegisterScriptableProperty(aRecordSpec, aPropSpec);

    aPropSpec->isDescribed = true;

    return true;
}

void ScriptableRecordManager::InsertDefaults(const ScriptableRecordSpecPtr& aSpec)
{
    if (!aSpec->isDescribed || aSpec->isInserted)
        return;

    const auto recordID = Red::TweakDBUtil::GetRTDBRecordID(aSpec->shortName);

    for (const auto& prop : aSpec->props | std::views::values)
    {
        if (!prop->isDescribed)
            continue;

        const auto flatID = recordID + std::string_view(".") + prop->name;
        auto instance = prop->defaultValue;

        if (!instance)
            instance = Red::TweakDBUtil::Construct(prop->typeSpec->flatType);

        if (!m_tweakManager->SetFlat(flatID, prop->typeSpec->flatType, instance.get()))
            LogError("Failed to insert default value for property {} of record type {} into TweakDB.", prop->name,
                     aSpec->name);
    }

    if (aSpec->type->parent)
        InsertDefaults(aSpec->type->parent);

    aSpec->isInserted = true;
}

void ScriptableRecordManager::InsertDefaults(const Red::CClass* aClass)
{
    if (!aClass)
        return;

    if (const auto spec = GetRecordSpec(aClass->GetName()))
        InsertDefaults(spec);
}

bool ScriptableRecordManager::DeregisterScriptableRecord(const ScriptableRecordSpecPtr& aSpec)
{
    if (!aSpec->type)
        return false;

    for (const auto& prop : aSpec->props | std::views::values)
        prop->isDescribed = false;

    aSpec->isRegistered = false;
    aSpec->isDescribed = false;

    {
        std::unique_lock lockRW(m_classesMutex);
        m_rtti->UnregisterType(aSpec->type);
        m_classes.erase(aSpec->hash);
        aSpec->type = nullptr;
    }

    {
        std::unique_lock lockRW(m_specsMutex);
        m_specs.erase(aSpec->cname);
    }

    return true;
}

ScriptableRecordClass* ScriptableRecordManager::GetRecordClass(const uint32_t aHash) const
{
    std::shared_lock lockR(m_classesMutex);

    if (const auto it = m_classes.find(aHash); it != m_classes.end())
        return it->second.get();

    return nullptr;
}

ScriptableRecordClass* ScriptableRecordManager::CreateRecordClass(const ScriptableRecordSpecPtr& aSpec)
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

void ScriptableRecordManager::AdaptScriptClass(const Red::ScriptClass* aClassDef)
{
    static const auto* ScriptableRecordType = ScriptableTweakDBRecord::TYPE::GetClass();

    if (!aClassDef->rttiClass)
        return;

    auto* cls = aClassDef->rttiClass;

    if (!cls->flags.isNative || !cls->parent || !cls->parent->IsA(ScriptableRecordType))
        return;

    const auto recordSpec = GetRecordSpec(cls->GetName());

    if (!recordSpec)
        return;

    for (const auto& func : cls->funcs)
        m_handlers->AdaptScriptFunction(recordSpec, func);
}

#ifndef NDEBUG

void ScriptableRecordManager::RegisterTestScriptableRecord()
{
    const auto name = RegisterScriptableRecordType(Red::TweakDBUtil::NormalizeRecordName("TweakXLTest"));
    const auto info = GetTweakTypeSpec("CName");

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
} // namespace App
