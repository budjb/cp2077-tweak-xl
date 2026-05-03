#include "ScriptableRecordManager.hpp"

#include "App/Tweaks/TweakPropertySpec.hpp"
#include "App/Tweaks/TweakService.hpp"
#include "ScriptableRecordClass.hpp"
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
        DeregisterScriptableRecord(spec);
    }
}

Red::CBaseFunction* ScriptableRecordManager::CreateScriptFunction(ScriptableRecordClass* aClass,
                                                                  const std::string& aName, const ContextPtr& aContext,
                                                                  const Red::ScriptingFunction_t<void*>& aFunc,
                                                                  const FunctionCustomizer& aCustomizer)
{
    const std::string nativeName = Red::TweakDBUtil::CreateIgnoredPropertyName(aName);
    auto* nativeFunc = Red::CClassFunction::Create(aClass, nativeName.c_str(), nativeName.c_str(), aFunc);
    aCustomizer(nativeFunc);
    aClass->RegisterFunction(nativeFunc);

    const auto fullName = Red::Detail::MakeScriptFunctionName(nativeFunc, aName.c_str());
    const auto scriptFunc = Red::CClassFunction::Create(aClass, fullName.c_str(), aName.c_str(), aFunc);
    scriptFunc->params.Insert(scriptFunc->params.End(), nativeFunc->params.Begin(), nativeFunc->params.End());
    scriptFunc->returnType = nativeFunc->returnType;
    scriptFunc->flags = nativeFunc->flags;

    const auto bytecode = CreateFunctionBytecode(aContext, nativeFunc);
    scriptFunc->bytecode.bytecode.buffer.data = bytecode.data;
    scriptFunc->bytecode.bytecode.buffer.size = bytecode.size;

    scriptFunc->flags.isNative = false;
    aClass->RegisterFunction(scriptFunc);

    // TODO: keep this?
    LogPropertyFunction(scriptFunc);

    return scriptFunc;
}

// TODO: keep this?
void ScriptableRecordManager::LogPropertyFunction(const Red::CClassFunction* aFunc)
{
    std::string args;

    for (const auto arg : aFunc->params)
    {
        if (!args.empty())
            args.append(", ");
        args.append(GetFriendlyTypeName(arg->type));
        args.append(" ");
        args.append(arg->name.ToString());
    }

    std::string ret = aFunc->returnType ? GetFriendlyTypeName(aFunc->returnType->type) : "void";

    LogDebug("Registered function '{} {}::{}({})", ret, GetFriendlyClassName(aFunc->parent),
             aFunc->shortName.ToString(), args);
}

// TODO: keep this?
std::string ScriptableRecordManager::GetFriendlyTypeName(const Red::CBaseRTTIType* aType)
{
    std::string name = aType->GetName().ToString();
    bool isArray = false;
    bool isFK = false;
    bool isWeak = false;

    if (name.starts_with("array:"))
    {
        isArray = true;
        name.erase(0, std::strlen("array:"));
    }

    if (name.starts_with("handle:"))
    {
        isFK = true;
        name.erase(0, std::strlen("handle:"));
        name = Red::TweakDBUtil::GetRecordAliasName<std::string>(name);
    }
    else if (name.starts_with("whandle:"))
    {
        isFK = true;
        isWeak = true;
        name.erase(0, std::strlen("whandle:"));
        name = Red::TweakDBUtil::GetRecordAliasName<std::string>(name);
    }

    if (isFK)
    {
        if (isWeak)
            name = "wref<" + name + ">";
        else
            name = "ref<" + name + ">";
    }

    if (isArray)
        name = "array<" + name + ">";

    return name;
}

std::string ScriptableRecordManager::GetFriendlyClassName(const Red::CClass* aClass)
{
    return m_rtti->ConvertNativeToScriptName(aClass->GetName()).ToString();
}

Red::RawBuffer ScriptableRecordManager::CreateFunctionBytecode(const ContextPtr& aContext, Red::CBaseFunction* aFunc)
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

    *reinterpret_cast<const Context**>(code) = aContext.get();

    return {memory, finalCodeSize};
}

bool ScriptableRecordManager::CreateScriptableRecord(Red::TweakDB* aTweakDB, const uint32_t aHash,
                                                     const Red::TweakDBID aRecordId)
{
    if (const auto cls = GetRecordClass(aHash))
    {
        return CreateScriptableRecord(aTweakDB, cls, aRecordId);
    }
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

ScriptableRecordManager::ScriptableRecordSpecPtr ScriptableRecordManager::GetRecordSpec(Red::CName aName) const
{
    std::shared_lock lockR(m_specsMutex);
    if (const auto it = m_specs.find(aName); it != m_specs.end())
    {
        return it->second;
    }
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

bool ScriptableRecordManager::DescribeScriptableRecordSpec(const ScriptableRecordSpecPtr& aSpec)
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
                                                             const ScriptablePropertySpecPtr& aSpec)
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

    CreateGetterFunctions(aClass, aSpec);

    aSpec->isDescribed = true;
    return true;
}

void ScriptableRecordManager::InsertScriptableRecordDefaults(const ScriptableRecordSpecPtr& aSpec)
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

bool ScriptableRecordManager::DeregisterScriptableRecord(const ScriptableRecordSpecPtr& aSpec)
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

    {
        std::unique_lock lockRW(m_classesMutex);
        m_rtti->UnregisterType(aSpec->type);
        m_classes.erase(aSpec->hash);
    }

    {
        std::unique_lock lockRW(m_contextsMutex);
        m_contexts.erase(aSpec->cname);
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
    {
        return it->second.get();
    }
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

} // namespace App
