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

    /**
     * @brief Attempts to construct a scriptable record instance and insert it into the given TweakDB instance. This
     * function is intended to be used as a means to directly inject scriptable record types into TweakDB and, in
     * particular, intercept normal Red::TweakDB::CreateRecord() invocations so that custom, scriptable record types may
     * function.
     *
     * @param aTweakDB Instance of TweakDB to insert records into.
     * @param aHash The hash of the record type to create. This should correspond to the hash of a registered scriptable
     * record type.
     * @param aRecordId The TweakDB ID of the record to create.
     * @return Whether the scriptable record was successfully created and inserted into the given TweakDB instance.
     */
    bool CreateScriptableRecord(Red::TweakDB* aTweakDB, uint32_t aHash, Red::TweakDBID aRecordId);

    /**
     * @brief Attempts to construct a scriptable record instance of the given class and insert it into the given TweakDB
     * instance. This function is intended to be used as a means to directly inject scriptable record types into TweakDB
     * and, in particular, intercept normal Red::TweakDB::CreateRecord() invocations so that custom, scriptable record
     * types may function.
     *
     * @param aTweakDB Instance of TweakDB to insert records into.
     * @param aClass The class of the record to create. This should correspond to a registered scriptable record class.
     * @param aRecordId The TweakDB ID of the record to create.
     * @return Whether the scriptable record was successfully created and inserted into the given TweakDB instance.
     */
    bool CreateScriptableRecord(Red::TweakDB* aTweakDB, ScriptableRecordClass* aClass, Red::TweakDBID aRecordId);

    /**
     * @brief Registers a scriptable record type specification with this manager.
     *
     * The name of the scriptable record type will be normalized to adhere to the typical naming conventions of TweakDB
     * record types. Specifically, the type's fully-qualified name follows the format @c gamedata<record_name>_Record,
     * the script alias name follows the format @c <record_name>_Record, and the short name is the fully-qualified name
     * with the @c gamedata prefix and @c _Record suffix removed. For example, if a scriptable record type is registered
     * with the name @c Vehicle, the fully-qualified name will be @c gamedataVehicle_Record, the alias name will be
     * @c Vehicle_Record, and the short name will be @c Vehicle.
     *
     * If a valid TweakDB record type is provided as the superclass of the scriptable record type, the scriptable record
     * type will inherit all TweakDB properties of the superclass. If the class' parent name is not provided, it will
     * automatically be set to the base scriptable record type.
     *
     * @param aName The unique name of the scriptable record type to register.
     * @param aParentName The unique name of the parent scriptable record type to inherit from. This is optional and, if
     * not provided, will default to the base scriptable record type.
     * @return The CName of the registered scriptable record type, or @c Red::CName::Empty if registration failed for
     * any reason.
     */
    Red::CName RegisterScriptableRecordType(const std::string& aName,
                                            const std::optional<std::string>& aParentName = std::nullopt);

    /**
     * @brief Registers a scriptable property specification with this manager under the given record type.
     *
     * @param aRecordName The name of the record type to register the property under. This should correspond to the
     * fully-qualified name of a registered scriptable record type.
     * @param aPropertyName The name of the property to register. This should adhere to typical TweakDB property naming
     * conventions, which are camelCase.
     * @param aFlatType The flat type of the property to register. This should correspond to a valid TweakDB flat type.
     * @param aForeignType The name of the foreign type of the property to register, if the property's flat type is a
     * foreign key. This should correspond to the fully-qualified name of a valid TweakDB record type.
     * @return The CName of the registered scriptable property, or @c Red::CName::Empty if registration failed for any
     * reason.
     */
    Red::CName RegisterScriptableProperty(Red::CName aRecordName, const std::string& aPropertyName, uint64_t aFlatType,
                                          const std::optional<std::string>& aForeignType = std::nullopt);

    /**
     * @brief Creates and registers RTTI classes for all pending scriptable record specifications registered with this
     * object. RTTI type registration only creates classes without any properties, functions, or inheritance.
     */
    void RegisterScriptableRecordSpecs();

    /**
     * @brief Describes, or completes setup, of RTTI classes for all pending scriptable record specifications registered
     * with this object. RTTI type description completes setup of classes by adding properties, functions, and
     * inheritance.
     */
    void DescribeScriptableRecordSpecs();

    /**
     * @brief Inserts default values for all scriptable records specifications registered with this object into the
     * provided TweakDB manager. This should only be called after all specs have been registered and described.
     *
     * In particular, an initial instance of the scriptable record will be inserted into TweakDB with the record ID
     * @c RTDB.<record_name>, where @c record_name is the short name of the record type. After this process completes,
     * scriptable record types are fully-loaded and ready to use. Additionally, a TweakDB flat instance will be inserted
     * into TweakDB with the default value for any direct property of the record with the TweakDB ID
     * @c RTDB.<record_name>.<property_name>, where @c property_name is the name of the property.
     *
     * Once this process is finished, scriptable record types are complete and ready for use.
     *
     * @param aManager The TweakDB manager to use for interacting with TweakDB.
     */
    void InsertScriptableRecordDefaults(const Core::SharedPtr<Red::TweakDBManager>& aManager);

#ifndef NDEBUG
    /**
     * @brief A functional test for validating that scriptable records are properly registered, described, and inserted
     * into TweakDB. Only enabled in debug builds.
     *
     * @param aManager The TweakDB manager to use for interacting with TweakDB.
     */
    void TestScriptableRecord(const Core::SharedPtr<Red::TweakDBManager>& aManager);
#endif

private:
    Red::CRTTISystem* m_rtti;

#ifndef NDEBUG
    /**
     * @brief Registers a test scriptable record type for use in testing and validating scriptable record functionality
     * during development. Only enabled in debug builds.
     */
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