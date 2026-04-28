#pragma once

#include <ffi.h>

#include "App/Tweaks/Record/ScriptableRecordClass.hpp"
#include "Core/Logging/LoggingAgent.hpp"
#include "Red/TweakDB/Manager.hpp"

namespace App
{
class TweakService;

/**
 * @brief A manager for handling the registration and lifecycle of scriptable record types, their properties, and the
 * underlying components that allow them to function.
 */
class ScriptableRecordManager : public Core::LoggingAgent
{
public:
    /**
     * @brief Retrieves the singleton instance of the ScriptableRecordManager. If the instance does not already exist,
     * it will be created.
     *
     * @return A pointer to the singleton instance of the ScriptableRecordManager.
     */
    static ScriptableRecordManager* Get();

    /**
     * @brief Destructs this ScriptableRecordManager instance and releases all resources owned by it, including
     * registered scriptable record specifications, their associated RTTI registrations, and active libFFI closures.
     */
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

    Red::CName RegisterScriptableProperty(Red::CName aRecordName, const std::string& aPropertyName,
                                          const Red::TweakDBUtil::PropertyFlatInfoPtr& aTypeInfo,
                                          const Red::InstancePtr<>& aDefaultValue = nullptr);

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
    /**
     * @brief A struct representing the context of a scriptable record property getter closure.
     */
    struct Context
    {
        /**
         * Path to append to a scriptable record's TweakDB ID when retrieving a property's value.
         */
        std::string appendix;

        // TODO: doc this again
        Red::TweakDBUtil::PropertyFlatInfoPtr typeInfo;
    };

    /**
     * @brief A struct representing an entry in the scriptable record property getter closure registry.
     */
    struct Closure
    {
        /**
         * @brief A pointer to the FFI closure object.
         */
        ffi_closure* closure{};

        /**
         * @brief A pointer to the executable memory allocated for the closure.
         */
        void* executable{};

        /**
         * @brief The execution context provided to invocations of the closure.
         */
        Context context;
    };

    /**
     * @brief A struct representing the specification of a scriptable property, including all the information necessary
     * to register the property as part of a scriptable record type and retrieve its value from TweakDB.
     */
    struct ScriptablePropertySpec
    {
        /**
         * @brief The name of the property, adhering to typical TweakDB property naming conventions (camelCase).
         */
        std::string name;

        /**
         * @brief The CName of the property, which is used as the hash index when looking up properties in the record
         * spec's properties map.
         */
        Red::CName cname;

        /**
         * @brief The path to append to a scriptable record's TweakDB ID when retrieving this property's value. This
         * should be in the format @c .<property_name> .
         */
        std::string appendix;

        Red::TweakDBUtil::PropertyFlatInfoPtr typeInfo;

        Red::InstancePtr<> defaultValue;

        /**
         * @brief Whether this property has been described, or had its RTTI class property created and set up.
         */
        bool isDescribed = false;
    };

    /**
     * @brief A struct representing the specification of a scriptable record type, including all the information
     * necessary to register the type as an RTTI class, describe the class by adding properties and functions, and
     * insert default instances of the record into TweakDB.
     */
    struct ScriptableRecordSpec
    {
        /**
         * @brief The fully-qualified name of the record type, following typical TweakDB naming conventions. This should
         * be in the format @c gamedata<record_name>_Record .
         */
        std::string name;

        /**
         * @brief The script alias name of the record type, following typical TweakDB naming conventions. This should be
         * in the format @c <record_name>_Record .
         */
        std::string aliasName;

        /**
         * @brief The short name of the record type, following typical TweakDB naming conventions. This should be the
         * fully qualified name with the @c gamedata prefix and @c _Record suffix removed, so it should correspond to
         * the <record_name> portion of the other name formats.
         */
        std::string shortName;

        /**
         * @brief The registered CName of the record's fully-qualified name.
         */
        Red::CName cname;

        /**
         * @brief The registered CName of the record's alias name.
         */
        Red::CName aliasCName;

        /**
         * @brief The registered CName of the record's short name.
         */
        Red::CName shortCName;

        /**
         * @brief The uint32_t hash of the record type based on its short name.
         */
        uint32_t hash;

        /**
         * @brief A pointer to the RTTI class of the record type. This will be populated during RTTI registration.
         */
        ScriptableRecordClass* type;

        /**
         * @brief The name of the parent record type, if any, from which this record type inherits. This should
         * correspond to the short name of a valid TweakDB record type. If not provided, the record will be a direct
         * subclass of @c App::ScriptableTweakDBRecord .
         */
        std::optional<std::string> parent;

        /**
         * @brief A map of the properties belonging to the record type, indexed by the CName of each property.
         */
        Core::Map<Red::CName, Core::SharedPtr<ScriptablePropertySpec>> props;

        /**
         * @brief Whether this record type has been registered, or had its RTTI class created.
         */
        bool isRegistered = false;

        /**
         * @brief Whether this record type has been described, or had its RTTI class set up with its parent class and
         * functions.
         */
        bool isDescribed = false;

        /**
         * @brief Whether this record type has been inserted into TweakDB with an initial instance and default property
         * values.
         */
        bool isInserted = false;
    };

    /**
     * @brief Constructs a ScriptableRecordManager instance.
     *
     * @see ScriptableRecordManager::Get() to retrieve the singleton instance of this class.
     */
    ScriptableRecordManager();

    /**
     * Creates a new closure that will retrieve the value of a specific property. The appendix and type info arguments
     * are provided to invocations of the closure, and the rest of the details required are provided by the game engine.
     *
     * @param aAppendix Appendix to apply to the TweakDB record instance's ID when retrieving property values.
     * @param aTypeInfo Type information of the property representing both its TweakDB flat type details and the
     * property type of the getter closure.
     * @return A pointer to the created closure's executable memory that conforms to the Red engine's required script
     * function signature, or @c nullptr if closure creation failed for any reason.
     */
    Red::ScriptingFunction_t<void*> CreateClosure(const std::string& aAppendix,
                                                  const Red::TweakDBUtil::PropertyFlatInfoPtr& aTypeInfo);

    Red::ScriptingFunction_t<void*> CreateClosure(const Context& aContext);

    /**
     * @brief Destroys a closure created by this manager and removes it from the closure registry.
     *
     * @param aClosure The closure to destroy.
     * @return Whether the closure was successfully destroyed and removed from the registry.
     */
    bool DestroyClosure(const Core::SharedPtr<Closure>& aClosure);

    /**
     * @brief Destroys a closure created by this manager and removes it from the closure registry.
     *
     * @param aClosure The closure to destroy.
     * @return Whether the closure was successfully destroyed and removed from the registry.
     */
    bool DestroyClosure(const ffi_closure* aClosure);

    /**
     * @brief The dispatch function for scriptable record property getter closures. This function is called whenever a
     * closure created by this manager is invoked, and is responsible for retrieving the appropriate property value from
     * TweakDB and returning it to the caller.
     *
     * @param aCif LibFFI call interface object. Unused.
     * @param aRet Pointer to the memory of the return value. Unused, as property getters return values through the
     * scripting function's @c out parameter.
     * @param aArgs Array of pointers to the arguments passed to the closure. The first three arguments are expected to
     * be the scriptable record instance, the stack frame of the function call, and the output pointer for the property
     * value, respectively.
     * @param aUserData Pointer to the closure's execution context, which contains the information necessary to retrieve
     * the correct property value from TweakDB.
     */
    static void FfiDispatch(ffi_cif* aCif, void* aRet, void** aArgs, void* aUserData);

    /**
     * @brief Retrieves the scriptable record specification with the given name from the registry.
     *
     * @param aName The fully-qualified name of the record type corresponding to the desired specification.
     * @return A shared pointer to the scriptable record specification, or @c nullptr if no specification with the given
     * name was found.
     */
    Core::SharedPtr<ScriptableRecordSpec> GetRecordSpec(Red::CName aName) const;

    /**
     * @brief Creates an App::RecordClass and registers it with RTTI based on the provided record specification. After
     * completion, only the skeleton type will exist with its parent or any functions. If successful, the specification
     * may subsequently be described.
     *
     * @param aSpec The specification of the scriptable record type to create.
     * @return Whether the record class was successfully created and registered with RTTI.
     */
    bool RegisterScriptableRecordSpec(const Core::SharedPtr<ScriptableRecordSpec>& aSpec);

    /**
     * @brief Adds functions and a parent class to an App::RecordClass based on the provided record specification. After
     * completion, the record class will be fully described and ready for use. If successful, the scriptable record's
     * default property values may subsequently be inserted into TweakDB.
     *
     * @param aSpec The specification of the scriptable record type to describe.
     * @return Whether the record class was successfully described with functions and a parent class.
     */
    bool DescribeScriptableRecordSpec(const Core::SharedPtr<ScriptableRecordSpec>& aSpec);

    /**
     * @brief Adds a function to an App::RecordClass based on the provided property specification, where the function
     * serves as the getter for the property. After completion, the record class will have a function that can be called
     * to retrieve the property's value from TweakDB. If successful, the property's default value may subsequently be
     * inserted into TweakDB.
     *
     * @param aClass The record class to which the property getter function will be added.
     * @param aSpec The specification of the scriptable property to describe.
     * @return Whether the property specification was successfully described with a getter function added to the record
     * class.
     */
    bool DescribeScriptablePropertySpec(ScriptableRecordClass* aClass,
                                        const Core::SharedPtr<ScriptablePropertySpec>& aSpec);

    /**
     * @brief Inserts default values for a scriptable record type based on the provided record specification.
     *
     * This involves creating an initial instance of the record type in TweakDB with the record ID @c
     * RTDB.<record_name>, where @c record_name is the short name of the record type, and inserting default values for
     * each direct property of the record with the TweakDB ID @c RTDB.<record_name>.<property_name>, where @c
     * property_name is the name of the property.
     *
     * @param aSpec
     * @param aManager
     */
    void InsertScriptableRecordDefaults(const Core::SharedPtr<ScriptableRecordSpec>& aSpec,
                                        const Core::SharedPtr<Red::TweakDBManager>& aManager);

    /**
     * @brief Inserts default values for a scriptable record type based on the provided RTTI class. This is an overload
     * of the previous function that retrieves the record specification based on the class's name and then calls the
     * previous function to perform the insertion.
     *
     * @param aClass The RTTI class of the record type for which to insert default values. This should correspond to the
     * class of a registered scriptable record type.
     * @param aManager The TweakDB manager to use for interacting with TweakDB when inserting default values.
     */
    void InsertScriptableRecordDefaults(const Red::CClass* aClass,
                                        const Core::SharedPtr<Red::TweakDBManager>& aManager);

    /**
     * @brief Unregisters a scriptable record type by destroying its corresponding RTTI class and removing it from the
     * record class registry. After completion, the record type will no longer be functional or accessible, and its
     * specification will be reset to an unregistered state. This function is intended to be used for hot-reloading
     * scriptable record types during development.
     *
     * @param aSpec The specification of the scriptable record type to unregister.
     * @return Whether the record type was successfully unregistered and removed from the registry.
     */
    bool UnregisterScriptableRecordSpec(const Core::SharedPtr<ScriptableRecordSpec>& aSpec);

    /**
     * @brief Registers a scriptable property function name with the Red::CNamePool based on the property's name.
     *
     * @param aName The name of the property for which to register the function name. This should be in camelCase and
     * will be converted to PascalCase for the function name.
     * @return The registered CName of the property function name, or @c Red::CName::Empty if registration failed for
     * any reason.
     */
    static Red::CName RegisterPropertyFunctionName(const std::string& aName);

    /**
     * @brief Retrieves the scriptable record class corresponding to the given hash from the registry.
     *
     * @param aHash The hash of the short name of the record class to retrieve.
     * @return A pointer to the scriptable record class, or @c nullptr if no class with the given hash was found.
     */
    ScriptableRecordClass* GetRecordClass(uint32_t aHash) const;

    /**
     * @brief Creates a scriptable record class based on the provided record specification and registers it with RTTI.
     *
     * @param aSpec
     * @return
     */
    ScriptableRecordClass* CreateRecordClass(const Core::SharedPtr<ScriptableRecordSpec>& aSpec);

    /**
     * @brief Destroys a scriptable record class by unregistering it from RTTI and removing it from the record class
     * registry. After completion, the record class will no longer be functional or accessible. This function is
     * intended to be used for hot-reloading scriptable record types during development.
     *
     * @param aClass The scriptable record class to destroy.
     * @return Whether the record class was successfully destroyed and removed from the registry.
     */
    bool DestroyRecordClass(ScriptableRecordClass* aClass);

    template<Red::ERTTIType>
    Red::ValuePtr<> ConvertValue(const Red::Value<>& aValue, const Red::TweakDBUtil::PropertyFlatInfoPtr& aTypeInfo,
                                 const Core::SharedPtr<TweakService>& aService)
    {
        return Red::MakeValue<>(aValue.type, aValue.instance);
    }

#ifndef NDEBUG
    /**
     * @brief Registers a test scriptable record type for use in testing and validating scriptable record functionality
     * during development. Only enabled in debug builds.
     */
    void RegisterTestScriptableRecord();
#endif

    /**
     * @brief A pointer to the RTTI system, which is used for registering scriptable record types as RTTI classes. This
     * is initialized in the constructor and should always be valid for the lifetime of this object.
     */
    Red::CRTTISystem* m_rtti;

    /**
     * @brief A registry of closures created by this manager.
     */
    Core::Vector<Core::SharedPtr<Closure>> m_closures;

    /**
     * @brief A mutex for synchronizing access to the closure registry for thread safety.
     */
    mutable std::mutex m_closuresMutex;

    /**
     * @brief The libFFI call interface object used for creating closures. This is lazily initialized on the first
     * closure creation and reused for subsequent closures.
     *
     * @todo This doesn't need to be lazy loaded.
     */
    ffi_cif m_cif{};

    /**
     * @brief Describes the argument types for the libFFI call interface object to implement a function signature
     * compatible with a scripting function.
     *
     * @see Red::ScriptingFunction_t<void*>
     */
    std::array<ffi_type*, 4> m_argTypes{{&ffi_type_pointer, &ffi_type_pointer, &ffi_type_pointer, &ffi_type_sint64}};

    /**
     * @brief Whether the libFFI call interface object has been successfully initialized and is ready for use.
     */
    bool m_cifReady = false;

    /**
     * @brief A mutex for synchronizing access to the scriptable record specification registry for thread safety.
     */
    mutable std::shared_mutex m_specsMutex;

    /**
     * @brief A registry of scriptable record specifications, indexed by the CName of each record type's fully-qualified
     * name.
     */
    Core::Map<Red::CName, Core::SharedPtr<ScriptableRecordSpec>> m_specs;

    /**
     * @brief A registry of scriptable record specifications, indexed by the hash of each record type's short name.
     */
    Core::Map<uint32_t, Core::SharedPtr<ScriptableRecordSpec>> m_specsByHash;

    /**
     * @brief A mutex for synchronizing access to the scriptable record class registry for thread safety.
     */
    mutable std::shared_mutex m_classesMutex;

    /**
     * @brief A registry of scriptable record classes, indexed by the hash of each record type's short name.
     */
    Core::Map<uint32_t, Core::SharedPtr<ScriptableRecordClass>> m_classes;
};

template<>
Red::ValuePtr<> ScriptableRecordManager::ConvertValue<Red::ERTTIType::Array>(
    const Red::Value<>& aValue, const Red::TweakDBUtil::PropertyFlatInfoPtr& aTypeInfo,
    const Core::SharedPtr<TweakService>& aService);

template<>
Red::ValuePtr<> ScriptableRecordManager::ConvertValue<Red::ERTTIType::Handle>(
    const Red::Value<>& aValue, const Red::TweakDBUtil::PropertyFlatInfoPtr& aTypeInfo,
    const Core::SharedPtr<TweakService>& aService);

template<>
Red::ValuePtr<> ScriptableRecordManager::ConvertValue<Red::ERTTIType::WeakHandle>(
    const Red::Value<>& aValue, const Red::TweakDBUtil::PropertyFlatInfoPtr& aTypeInfo,
    const Core::SharedPtr<TweakService>& aService);
} // namespace App