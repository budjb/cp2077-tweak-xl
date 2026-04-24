#pragma once

#include <ffi.h>

#include "Core/Logging/LoggingAgent.hpp"
#include "Red/TweakDB/Manager.hpp"

namespace App
{
#pragma region RecordClass

class RecordClass : public Red::CClass
{
public:
    RecordClass(RED4ext::CName aName, uint32_t aHash);
    void ConstructCls(void* aMemory) const override;
    void DestructCls(void* aMemory) const override;
    [[nodiscard]] void* AllocMemory() const override;
    const bool IsEqual(const void* aLhs, const void* aRhs, uint32_t a3) override;
    void Assign(void* aLhs, const void* aRhs) const override;
    const uint32_t tweakBaseHash;
};

#pragma endregion

#pragma region ScriptableRecordManager

class ScriptableRecordManager : public Core::LoggingAgent
{
public:
    ScriptableRecordManager();
    ~ScriptableRecordManager();

    bool CreateScriptableRecord(Red::TweakDB* aTweakDB, Red::TweakDBID aRecordId, uint32_t aHash);

private:
    Red::CRTTISystem* m_rtti;

#pragma region Closures

private:
    using GetterFn = Red::ScriptingFunction_t<void*>;

    struct Context
    {
        std::string appendix;
        const Red::CBaseRTTIType* type;
    };

    struct Entry
    {
        ffi_closure* closure{};
        void* executable{};
        Context context;
    };

    GetterFn CreateClosure(const std::string& aName, const Red::CBaseRTTIType* aType);
    bool DestroyClosure(const GetterFn& aClosure);
    static void FfiDispatch(ffi_cif* aCif, void* aRet, void** aArgs, void* aUserData);

    mutable std::mutex m_closuresMutex;
    Core::Vector<Core::UniquePtr<Entry>> m_closures;
    ffi_cif m_cif{};
    std::array<ffi_type*, 4> m_argTypes{{&ffi_type_pointer, &ffi_type_pointer, &ffi_type_pointer, &ffi_type_sint64}};
    bool m_cifReady = false;

#pragma endregion

#pragma region Specs

public:
    Red::CName RegisterScriptableRecordType(const std::string& aName,
                                            const std::optional<std::string>& aParentName = std::nullopt);
    Red::CName RegisterScriptableProperty(Red::CName aRecordName, const std::string& aPropertyName, uint64_t aFlatType,
                                          const std::optional<std::string>& aForeignType);

    void RegisterScriptableRecordSpecs();
    void DescribeScriptableRecordSpecs();

private:
    struct ScriptablePropertySpec
    {
        std::string name;
        uint64_t type;
        std::optional<std::string> foreignType;
        Red::CName cname;
        // TODO: default value
        bool isDescribed = false;
    };

    struct ScriptableRecordSpec
    {
        std::string name;
        std::string aliasName;
        std::string shortName;
        Red::CName cname;
        Red::CClass* type;
        std::optional<std::string> parent;
        Core::Map<Red::CName, Core::SharedPtr<ScriptablePropertySpec>> props;
        bool isRegistered = false;
        bool isDescribed = false;
    };

    Core::SharedPtr<ScriptableRecordSpec> GetRecordSpec(Red::CName aName) const;
    bool RegisterScriptableRecordSpec(const Core::SharedPtr<ScriptableRecordSpec>& aSpec);
    bool DescribeScriptableRecordSpec(const Core::SharedPtr<ScriptableRecordSpec>& aSpec);
    bool DescribeScriptablePropertySpec(RecordClass* aClass, const Core::SharedPtr<ScriptablePropertySpec>& aSpec);

    static Red::CName RegisterRecordFullName(const std::string& aName);
    static Red::CName RegisterRecordAliasName(const std::string& aName);
    static Red::CName RegisterPropertyFunctionName(const std::string& aName);

    RecordClass* GetRecordClass(uint32_t aHash) const;

    mutable std::shared_mutex m_specsMutex;
    Core::Map<Red::CName, Core::SharedPtr<ScriptableRecordSpec>> m_specs;

    mutable std::shared_mutex m_classesMutex;
    Core::Map<uint32_t, Core::SharedPtr<RecordClass>> m_classes;

#pragma endregion

#pragma endregion
};
} // namespace App