#pragma once

#include <ffi.h>

#include "App/Tweaks/Record/ScriptableRecordClass.hpp"
#include "Core/Logging/LoggingAgent.hpp"
#include "Red/TweakDB/Manager.hpp"

namespace App
{

class ScriptableRecordManager : public Core::LoggingAgent
{
public:
    ScriptableRecordManager();
    ~ScriptableRecordManager();

    bool CreateScriptableRecord(Red::TweakDB* aTweakDB, uint32_t aHash, Red::TweakDBID aRecordId);
    bool CreateScriptableRecord(Red::TweakDB* aTweakDB, ScriptableRecordClass* aClass, Red::TweakDBID aRecordId);

#ifndef NDEBUG
    void TestScriptableRecord(const Core::SharedPtr<Red::TweakDBManager>& aManager);
#endif

private:
    Red::CRTTISystem* m_rtti;

#ifndef NDEBUG
    void RegisterTestScriptableRecord();
#endif

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
    bool DestroyClosure(const Core::SharedPtr<Entry>& aClosure);
    bool DestroyClosure(const ffi_closure* aClosure);
    static void FfiDispatch(ffi_cif* aCif, void* aRet, void** aArgs, void* aUserData);

    mutable std::mutex m_closuresMutex;
    Core::Vector<Core::SharedPtr<Entry>> m_closures;
    ffi_cif m_cif{};
    std::array<ffi_type*, 4> m_argTypes{{&ffi_type_pointer, &ffi_type_pointer, &ffi_type_pointer, &ffi_type_sint64}};
    bool m_cifReady = false;

#pragma endregion

#pragma region Specs

public:
    Red::CName RegisterScriptableRecordType(const std::string& aName,
                                            const std::optional<std::string>& aParentName = std::nullopt);
    Red::CName RegisterScriptableProperty(Red::CName aRecordName, const std::string& aPropertyName, uint64_t aFlatType,
                                          const std::optional<std::string>& aForeignType = std::nullopt);

    void RegisterScriptableRecordSpecs();
    void DescribeScriptableRecordSpecs();
    void InsertScriptableRecordDefaults(const Core::SharedPtr<Red::TweakDBManager>& aManager);

private:
    struct ScriptablePropertySpec
    {
        std::string name;
        Red::CName cname;
        std::string appendix;
        uint64_t type;
        std::optional<std::string> foreignName;
        Red::CClass* foreignType;

        // TODO: default value
        bool isDescribed = false;
    };

    struct ScriptableRecordSpec
    {
        std::string name;
        std::string aliasName;
        std::string shortName;

        Red::CName cname;
        Red::CName aliasCName;
        Red::CName shortCName;

        uint32_t hash;

        ScriptableRecordClass* type;
        std::optional<std::string> parent;
        Core::Map<Red::CName, Core::SharedPtr<ScriptablePropertySpec>> props;

        bool isRegistered = false;
        bool isDescribed = false;
        bool isInserted = false;
    };

    Core::SharedPtr<ScriptableRecordSpec> GetRecordSpec(Red::CName aName) const;
    bool RegisterScriptableRecordSpec(const Core::SharedPtr<ScriptableRecordSpec>& aSpec);
    bool DescribeScriptableRecordSpec(const Core::SharedPtr<ScriptableRecordSpec>& aSpec);
    bool DescribeScriptablePropertySpec(ScriptableRecordClass* aClass,
                                        const Core::SharedPtr<ScriptablePropertySpec>& aSpec);
    void InsertScriptableRecordDefaults(const Core::SharedPtr<ScriptableRecordSpec>& aSpec,
                                        const Core::SharedPtr<Red::TweakDBManager>& aManager);
    void InsertScriptableRecordDefaults(const Red::CClass* aClass,
                                        const Core::SharedPtr<Red::TweakDBManager>& aManager);
    bool UnregisterScriptableRecordSpec(const Core::SharedPtr<ScriptableRecordSpec>& aSpec);

    static Red::CName RegisterPropertyFunctionName(const std::string& aName);

    mutable std::shared_mutex m_specsMutex;
    Core::Map<Red::CName, Core::SharedPtr<ScriptableRecordSpec>> m_specs;
    Core::Map<uint32_t, Core::SharedPtr<ScriptableRecordSpec>> m_specsByHash;

#pragma endregion

#pragma region ScriptableRecordClass

    ScriptableRecordClass* GetRecordClass(uint32_t aHash) const;
    ScriptableRecordClass* CreateRecordClass(const Core::SharedPtr<ScriptableRecordSpec>& aSpec);
    bool DestroyRecordClass(ScriptableRecordClass* aClass);

    mutable std::shared_mutex m_classesMutex;
    Core::Map<uint32_t, Core::SharedPtr<ScriptableRecordClass>> m_classes;

#pragma endregion
};
} // namespace App