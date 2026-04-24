#include "ScriptableRecordManager.hpp"

#include "App/Tweaks/TweakService.hpp"
#include "Core/Facades/Container.hpp"
#include "ScriptableTweakDBRecord.hpp"

namespace
{
constexpr auto ScriptableRecordSize = sizeof(Red::ScriptableTweakDBRecord);
constexpr auto ScriptableRecordAlignment = alignof(Red::ScriptableTweakDBRecord);
} // namespace

namespace App
{
#pragma region RecordClass

RecordClass::RecordClass(Red::CName aName, const uint32_t aHash)
    : CClass(aName, ScriptableRecordSize, {.isNative = true})
    , tweakBaseHash(aHash)
{
    alignment = ScriptableRecordAlignment;
}

void RecordClass::ConstructCls(void* aMemory) const
{
    new (aMemory) Red::ScriptableTweakDBRecord(this);
}

void RecordClass::DestructCls(void* aMemory) const
{
    static_cast<Red::ScriptableTweakDBRecord*>(aMemory)->~ScriptableTweakDBRecord();
}

void* RecordClass::AllocMemory() const
{
    const auto alignedSize = Red::AlignUp(size, alignment);

    const auto allocator = GetAllocator();
    auto [memory, size] = allocator->AllocAligned(alignedSize, alignment);

    std::memset(memory, 0, size);
    return memory;
}

const bool RecordClass::IsEqual(const void* aLhs, const void* aRhs, const uint32_t a3)
{
    using func_t = bool (*)(CClass*, const void*, const void*, uint32_t);
#if defined(RED4EXT_V1_SDK_VERSION_CURRENT) || defined(RED4EXT_SDK_0_5_0)
    static RED4ext::UniversalRelocFunc<func_t> func(RED4ext::Detail::AddressHashes::TTypedClass_IsEqual);
#else
    static RelocFunc<func_t> func(RED4ext::Addresses::TTypedClass_IsEqual);
#endif
    return func(this, aLhs, aRhs, a3);
}

void RecordClass::Assign(void* aLhs, const void* aRhs) const
{
    new (aLhs) Red::ScriptableTweakDBRecord(*static_cast<const Red::ScriptableTweakDBRecord*>(aRhs));
}

#pragma endregion

#pragma region ScriptableRecordManager

ScriptableRecordManager::ScriptableRecordManager()
    : m_rtti(Red::CRTTISystem::Get())
{
}

ScriptableRecordManager::~ScriptableRecordManager()
{
    for (const auto& entry : m_closures)
    {
        if (entry->closure)
        {
            ffi_closure_free(entry->closure);
        }
    }
}

#pragma region Closures

ScriptableRecordManager::GetterFn ScriptableRecordManager::CreateClosure(const std::string& aName,
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

    auto entry = Core::MakeUnique<Entry>();
    entry->context.appendix = "." + aName;
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

    const auto function = reinterpret_cast<GetterFn>(entry->executable);
    m_closures.emplace_back(std::move(entry));
    return function;
}

bool ScriptableRecordManager::DestroyClosure(const GetterFn& aClosure)
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

        if (reinterpret_cast<GetterFn>(entry->executable) != aClosure)
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

    const auto* record = reinterpret_cast<Red::ScriptableTweakDBRecord*>(instance);

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

#pragma endregion

#pragma region Specs

Red::CName ScriptableRecordManager::RegisterScriptableRecordType(const std::string& aName,
                                                                 const std::optional<std::string>& aParentName)
{
    const auto cname = Red::CName{aName.c_str()};

    if (GetRecordSpec(cname))
    {
        std::shared_lock lockR(m_specsMutex);
        if (const auto it = m_specs.find(Red::CName(aName.c_str())); it != m_specs.end())
        {
            return {};
        }
    }

    if (m_rtti->GetClass(aName.c_str()))
    {
        return {};
    }

    const auto spec = Core::MakeShared<ScriptableRecordSpec>();
    spec->name = Red::TweakDBUtil::GetRecordFullName<std::string>(aName);
    spec->cname = cname;
    spec->parent = aParentName;

    {
        std::unique_lock lockRW(m_specsMutex);
        if (const auto [_, success] = m_specs.insert({cname, spec}); success)
        {
            return spec->cname;
        }
    }

    return {};
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
    propertyInfo->type = aFlatType;
    propertyInfo->foreignType = aForeignType;
    propertyInfo->cname = Red::CName{aPropertyName.c_str()};

    recordInfo->props[propertyInfo->cname] = propertyInfo;

    return propertyInfo->cname;
}

void ScriptableRecordManager::RegisterScriptableRecordSpecs()
{
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

    const auto fullName = RegisterRecordFullName(Red::TweakDBUtil::GetRecordFullName<std::string>(aSpec->name));

    if (GetRecordClass(fullName))
    {
        // TODO: log warning
        return false;
    }

    if (m_rtti->GetClass(fullName))
    {
        // TODO: log warning
        return false;
    }

    const auto cls = Core::MakeShared<RecordClass>(fullName, Red::TweakDBUtil::GetRecordTypeHash(fullName));
    const auto aliasName = RegisterRecordAliasName(Red::TweakDBUtil::GetRecordAliasName<std::string>(cls->name));

    m_rtti->RegisterType(cls.get());
    m_rtti->RegisterScriptName(cls->name, aliasName);

    {
        std::unique_lock lockRW(m_classesMutex);
        m_classes[cls->tweakBaseHash] = cls;
    }

    aSpec->isRegistered = true;
    return true;
}

bool ScriptableRecordManager::DescribeScriptableRecordSpec(const Core::SharedPtr<ScriptableRecordSpec>& aSpec)
{
    if (!aSpec->isRegistered || aSpec->isDescribed)
    {
        // TODO: log warning
        return false;
    }

    auto* cls = GetRecordClass(Red::TweakDBUtil::GetRecordFullName<Red::CName>(aSpec->name));

    if (!cls)
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

        cls->parent = parentCls;
    }
    else
    {
        cls->parent = Red::ScriptableTweakDBRecord::TYPE::GetClass();
    }

    for (const auto& prop : aSpec->props | std::views::values)
    {
        DescribeScriptablePropertySpec(cls, prop);
    }

    aSpec->isDescribed = true;
    return true;
}

bool ScriptableRecordManager::DescribeScriptablePropertySpec(RecordClass* aClass,
                                                             const Core::SharedPtr<ScriptablePropertySpec>& aSpec)
{
    if (aSpec->isDescribed)
    {
        return false;
    }

    const auto* type = Red::TweakDBUtil::GetFlatType(aSpec->type);

    if (!type)
    {
        return false;
    }

    const auto closure = CreateClosure(aSpec->name, type);

    if (!closure)
    {
        return false;
    }

    const auto name = RegisterPropertyFunctionName(aSpec->name);
    auto* function = Red::CClassFunction::Create(aClass, name.ToString(), name.ToString(), closure);
    function->SetReturnType(type->GetName());
    aClass->RegisterFunction(function);

    aSpec->isDescribed = true;
    return true;
}

Red::CName ScriptableRecordManager::RegisterRecordFullName(const std::string& aName)
{
    return Red::CNamePool::Add(Red::TweakDBUtil::GetRecordFullName<std::string>(aName).c_str());
}

Red::CName ScriptableRecordManager::RegisterRecordAliasName(const std::string& aName)
{
    return Red::CNamePool::Add(Red::TweakDBUtil::GetRecordFullName<std::string>(aName).c_str());
}

Red::CName ScriptableRecordManager::RegisterPropertyFunctionName(const std::string& aName)
{
    std::string name = aName;
    name[0] = static_cast<char>(std::toupper(name[0]));
    return Red::CNamePool::Add(name.c_str());
}

RecordClass* ScriptableRecordManager::GetRecordClass(const uint32_t aHash) const
{
    std::shared_lock lockR(m_classesMutex);
    if (const auto it = m_classes.find(aHash); it != m_classes.end())
    {
        return it->second.get();
    }
    return nullptr;
}

#pragma endregion

// TODO: move this out
bool ScriptableRecordManager::CreateScriptableRecord(Red::TweakDB* aTweakDB, Red::TweakDBID aRecordId, uint32_t aHash)
{
    // if (const auto instance = m_reflection->ConstructScriptableRecord(aRecordId, aHash))
    // {
    //     Raw::InsertRecord(aTweakDB, aRecordId, instance->GetType(), instance);
    //     return true;
    // }
    return false;
}

#pragma endregion

} // namespace App