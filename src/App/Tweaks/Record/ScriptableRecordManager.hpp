#pragma once

#include "App/Tweaks/Record/ScriptableRecordClass.hpp"
#include "App/Tweaks/TweakPropertySpec.hpp"
#include "Core/Logging/LoggingAgent.hpp"
#include "Red/TweakDB/Manager.hpp"

namespace App
{
class TweakService;

/**
 * @brief A manager for handling the registration and lifecycle of scriptable record types, their properties, and the
 * underlying components that allow them to function.
 */
class ScriptableRecordManager
    : public Core::LoggingAgent
    , public Core::ShareFromThis<ScriptableRecordManager>
{
public:
    /**
     * @brief Defines a type alias for an array of WeakHandles to TweakDBRecords, which is used as the return type for
     * getter functions that retrieve arrays of related records based on foreign key relationships. This type alias
     * simplifies the code and improves readability by providing a clear and descriptive name for this specific type of
     * array, which is commonly used in the context of scriptable record properties that represent foreign key
     * relationships to TweakDBRecords.
     */
    using RecordArray = Red::DynArray<Red::WeakHandle<Red::TweakDBRecord>>;

    /**
     * @brief Defines a type alias for a shared pointer to an array of WeakHandles to TweakDBRecords, which is used to
     * manage the lifetime of the array when it is returned from getter functions that retrieve arrays of related
     * records based on foreign key relationships. This type alias simplifies memory management and improves readability
     * by providing a clear and descriptive name for a shared pointer to this specific type of array, which is commonly
     * used in the context of scriptable record properties that represent foreign key relationships to TweakDBRecords.
     */
    using RecordArrayPtr = Red::InstancePtr<RecordArray>;

    /**
     * @brief Defines a type alias for a WeakHandle to a TweakDBRecord, which is used as the return type for getter
     * functions that retrieve individual related records based on foreign key relationships. This type alias simplifies
     * the code and improves readability by providing a clear and descriptive name for this specific type of WeakHandle,
     * which is commonly used in the context of scriptable record properties that represent foreign key relationships to
     * TweakDBRecords.
     */
    using RecordWHandle = Red::WeakHandle<Red::TweakDBRecord>;

    /**
     * @brief Defines a type alias for a Handle to a TweakDBRecord, which is used as the return type for getter
     * functions that retrieve individual related records based on foreign key relationships when a stronger reference
     * is needed. This type alias simplifies the code and improves readability by providing a clear and descriptive name
     * for this specific type of Handle, which is commonly used in the context of scriptable record properties that
     * represent foreign key relationships to TweakDBRecords where ownership semantics may be required.
     */
    using RecordHandle = Red::Handle<Red::TweakDBRecord>;

    /**
     * @brief A struct representing the context of a scriptable record property getter function.
     */
    struct Context
    {
        /**
         * @brief Path to append to a scriptable record's TweakDB ID when retrieving a property's value.
         */
        std::string appendix;

        /**
         * @brief Type specification of the property representing both its TweakDB flat type details and the property
         * type of the getter function.
         */
        TweakPropertySpecPtr propSpec;

        /**
         * @brief A shared pointer to the scriptable record manager.
         */
        Core::SharedPtr<ScriptableRecordManager> recordManager;

        /**
         * @brief A deferred to the TweakDB manager, which is used to retrieve property values from TweakDB when the
         * function is invoked.
         */
        Core::DeferredPtr<Red::TweakDBManager> tweakManager;
    };

    /**
     * @brief Alias for a shared pointer to a Context instance.
     */
    using ContextPtr = Core::SharedPtr<Context>;

    /**
     * @brief A struct representing the specification of a scriptable property, including all the information necessary
     * to register the property as part of a scriptable record type and retrieve its value from TweakDB.
     */
    struct ScriptablePropertySpec
    {
        /**
         * @brief The name of the property, typically camelCase or PascalCase.
         */
        std::string name;

        /**
         * @brief The name of the getter function to create for this property, which should be in PascalCase and
         * typically match the property name but with the first letter capitalized.
         */
        std::string functionName;

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

        /**
         * @brief Type specification of the property representing both its TweakDB flat type details and the property
         * type of the getter function.
         */
        TweakPropertySpecPtr typeSpec;

        /**
         * @brief The default value of the property, which will be inserted into TweakDB for the record type with the ID
         * @c RTDB.<record_name>.<property_name> when the record type is inserted into TweakDB.
         */
        Red::InstancePtr<> defaultValue;

        /**
         * @brief Whether this property has been described, or had its RTTI class property created and set up.
         */
        bool isDescribed = false;
    };

    using ScriptablePropertySpecPtr = Core::SharedPtr<ScriptablePropertySpec>;

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
        Core::Map<Red::CName, ScriptablePropertySpecPtr> props;

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

    using ScriptableRecordSpecPtr = Core::SharedPtr<ScriptableRecordSpec>;

    /**
     * @brief Constructs a ScriptableRecordManager instance.
     *
     * @see ScriptableRecordManager::Get() to retrieve the singleton instance of this class.
     */
    explicit ScriptableRecordManager(const Core::DeferredPtr<Red::TweakDBManager>& aManager);

    /**
     * @brief Destructs this ScriptableRecordManager instance and releases all resources owned by it, including
     * registered scriptable record specifications, their associated RTTI registrations, and active libFFI functions.
     */
    ~ScriptableRecordManager();

    /**
     * @brief Returns a vector containing shared pointers to all registered scriptable record specifications.
     *
     * @return A vector containing shared pointers to all registered scriptable record specifications.
     */
    Core::Vector<ScriptableRecordSpecPtr> GetRecordSpecs() const;

    /**
     * @brief Attempts to construct a scriptable record instance and insert it into the given TweakDB instance. This
     * function is intended to be used as a means to directly inject scriptable record types into TweakDB and, in
     * particular, intercept normal Red::TweakDB::CreateRecord() invocations so that custom, scriptable record types may
     * function.
     *
     * If this function returns false, the normal record creation should follow to allow built-in TweakDB record types
     * to be built.
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
     * @brief Registers a scriptable property specification with a scriptable record type. This involves creating and
     * registering RTTI functions for the property based on the provided specification.
     *
     * @param aRecordName The name of the scriptable record type to register the property with. This should correspond
     * to the name of a registered scriptable record type.
     * @param aPropertyName The name of the property to register.
     * @param aTypeSpec The type specification of the property, containing both its TweakDB flat type details and the
     * property type of the getter function.
     * @param aDefaultValue The default value of the property that will be inherited by instances of the record type
     * when no explicit value is provided for the instance.
     * @return The CName of the registered property, or @c Red::CName::Empty if registration failed for any reason.
     */
    Red::CName RegisterScriptableProperty(Red::CName aRecordName, const std::string& aPropertyName,
                                          const TweakPropertySpecPtr& aTypeSpec,
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
     */
    void InsertScriptableRecordDefaults();

#ifndef NDEBUG
    /**
     * @brief A functional test for validating that scriptable records are properly registered, described, and inserted
     * into TweakDB. Only enabled in debug builds.
     */
    void TestScriptableRecord();
#endif

private:
    /**
     * @brief Defines a type alias for a function that receives a pointer to a CClassFunction and allows for
     * customization of the function, such as adding arguments and a return type, before it is registered.
     */
    using FunctionCustomizer = std::function<void(Red::CClassFunction*)>;

    /**
     * @brief Creates and registers relevant RTTI functions for a property of a scriptable record class based on the
     * provided property specification.
     *
     * @param aClass The scriptable record class to which functions will be added.
     * @param aSpec The specification of the scriptable property for which functions will be created, containing all
     * necessary information to determine which functions to create and how to set them up.
     */
    void CreateGetterFunctions(ScriptableRecordClass* aClass, const ScriptablePropertySpecPtr& aSpec);

    /**
     * @brief Creates and registers a function for a scriptable record property.
     *
     * Due to how functions work within the scripting VM, function invocations do not receive details about the identity
     * of the function call itself. The various handler functions that implement the scriptable record property getters
     * are reused across all scriptable record types, so execution context containing details of the property being
     * operated on must be provided to the invocation.
     *
     * This function creates a wrapping "script" function that proxies to the given "native" function. Before invoking
     * the "native" function, the "script" function injects a pointer to the given execution context on the call stack
     * directly after the "ParamEnd" opcode.
     *
     * @param aClass The scriptable record class to which the property function will be added.
     * @param aName The name of the property function to create.
     * @param aContext The execution context to provide to the function invocation via the call stack.
     * @param aNativeFunc The native function that implements the logic of the property getter, which will be proxied to
     * by the script function after the execution context is injected.
     * @return A pointer to the created "script" function.
     */
    Red::CBaseFunction* CreateScriptFunction(ScriptableRecordClass* aClass, const std::string& aName,
                                             const ContextPtr& aContext, Red::CGlobalFunction* aNativeFunc);

    // TODO: keep this?
    void LogPropertyFunction(const Red::CClassFunction* aFunc);

    // TODO: keep this?
    std::string GetFriendlyTypeName(const RED4ext::rtti::IType* aType);

    // TODO: keep this?
    std::string GetFriendlyClassName(const Red::CClass* aClass);

    /**
     * @brief Creates and registers a function for a scriptable record property that retrieves an array of related
     * records based on a foreign key relationship. This function is intended to be used as the getter for properties
     * that represent arrays of foreign keys to other TweakDB record types.
     *
     * The resulting function will have the following signature, where [Prop] is the provided property name:
     *
     * @code void [Prop](DynArray<WeakHandle<TweakDBRecord>* out)@endcode
     *
     * @param aClass The scriptable record class to which the property function will be added.
     * @param aName The name of the property function to create.
     * @param aContext The execution context to provide to the function invocation via the call stack, containing
     * necessary details of the property and record type to retrieve the related records from TweakDB.
     */
    void CreateGetRecords(ScriptableRecordClass* aClass, const std::string& aName, const ContextPtr& aContext);

    /**
     * @brief Creates and registers a function for a scriptable record property that retrieves an individual related
     * record based on a foreign key relationship from an array. This function is intended to be used as the getter for
     * properties that represent foreign keys to other TweakDB record types.
     *
     * The resulting function will have the following signature, where [Prop] is the provided property name:
     *
     * @code WeakHandle<TweakDBRecord> Get[Prop]Item(int index)@endcode
     *
     * @param aClass The scriptable record class to which the property function will be added.
     * @param aName The base name of the property function to create.
     * @param aContext The execution context to provide to the function invocation via the call stack, containing
     * necessary details of the property and record type to retrieve the related record from TweakDB.
     */
    void CreateGetRecordItem(ScriptableRecordClass* aClass, const std::string& aName, const ContextPtr& aContext);

    /**
     * @brief Creates and registers a function for a scriptable record property that retrieves an individual related
     * record based on a foreign key relationship from an array. This function is intended to be used as the getter for
     * properties that represent foreign keys to other TweakDB record types.
     *
     * The resulting function will have the following signature, where [Prop] is the provided property name:
     *
     * @code Handle<TweakDBRecord> Get[Prop]ItemHandle(int index)@endcode
     *
     * @param aClass The scriptable record class to which the property function will be added.
     * @param aName The base name of the property function to create.
     * @param aContext The execution context to provide to the function invocation via the call stack, containing
     * necessary details of the property and record type to retrieve the related record from TweakDB.
     */
    void CreateGetRecordItemHandle(ScriptableRecordClass* aClass, const std::string& aName, const ContextPtr& aContext);

    /**
     * @brief Creates and registers a function for a scriptable record property that checks whether a given related
     * record is contained in the array of related records based on a foreign key relationship. This function is
     * intended to be used as a helper for properties that represent arrays of foreign keys to other TweakDB record
     * types.
     *
     * The resulting function will have the following signature, where [Prop] is the provided property name:
     *
     * @code bool [Prop]Contains(WeakHandle<TweakDBRecord> item)@endcode
     *
     * @param aClass The scriptable record class to which the property function will be added.
     * @param aName The base name of the property function to create.
     * @param aContext The execution context to provide to the function invocation via the call stack, containing
     * necessary details of the property and record type to retrieve the related records from TweakDB and check for
     * containment.
     */
    void CreateRecordArrayContains(ScriptableRecordClass* aClass, const std::string& aName, const ContextPtr& aContext);

    /**
     * @brief Creates and registers a function for a scriptable record property that retrieves an individual related
     * record based on a foreign key relationship. This function is intended to be used as the getter for properties
     * that represent foreign keys to other TweakDB record types.
     *
     * The resulting function will have the following signature, where [Prop] is the provided property name:
     *
     * @code WeakHandle<TweakDBRecord> [Prop]()@endcode
     *
     * @param aClass The scriptable record class to which the property function will be added.
     * @param aName The name of the property function to create.
     * @param aContext The execution context to provide to the function invocation via the call stack, containing
     * necessary details of the property and record type to retrieve the related record from TweakDB.
     */
    void CreateGetRecord(ScriptableRecordClass* aClass, const std::string& aName, const ContextPtr& aContext);

    /**
     * @brief Creates and registers a function for a scriptable record property that retrieves an individual related
     * record based on a foreign key relationship. This function is intended to be used as the getter for properties
     * that represent foreign keys to other TweakDB record types when a stronger reference is needed.
     *
     * The resulting function will have the following signature, where [Prop] is the provided property name:
     *
     * @code Handle<TweakDBRecord> [Prop]Handle()@endcode
     *
     * @param aClass The scriptable record class to which the property function will be added.
     * @param aName The name of the property function to create.
     * @param aContext The execution context to provide to the function invocation via the call stack, containing
     * necessary details of the property and record type to retrieve the related record from TweakDB.
     */
    void CreateGetRecordHandle(ScriptableRecordClass* aClass, const std::string& aName, const ContextPtr& aContext);

    /**
     * @brief Creates and registers a function for a scriptable record property that return the number of elements in an
     * array. This function is suitable for use with any type of array property.
     *
     * The resulting function will have the following signature, where [Prop] is the provided property name:
     *
     * @code int Get[Prop]Count()@endcode
     *
     * @param aClass The scriptable record class to which the property function will be added.
     * @param aName The base name of the property function to create.
     * @param aContext The execution context to provide to the function invocation via the call stack, containing
     * necessary details of the property to retrieve the array from TweakDB and determine its count.
     */
    void CreateGetArrayCount(ScriptableRecordClass* aClass, const std::string& aName, const ContextPtr& aContext);

    /**
     * @brief Creates and registers a function for a scriptable record property that retrieves an individual item from
     * an array based on its index. This function is suitable for use with any type of array property other than array
     * of foreign keys to other TweakDB records.
     *
     * The resulting function will have the following signature, where [Prop] is the provided property name and [Type]
     * is the property type defined in the provided execution context:
     *
     * @code [Type] Get[Prop]Item(int index)@endcode
     *
     * @param aClass The scriptable record class to which the property function will be added.
     * @param aName The base name of the property function to create.
     * @param aContext The execution context to provide to the function invocation via the call stack, containing
     * necessary details of the property to retrieve the array from TweakDB and return the appropriate
     * item based on the provided index.
     */
    void CreateGetArrayItem(ScriptableRecordClass* aClass, const std::string& aName, const ContextPtr& aContext);

    /**
     * @brief Creates and registers a function for a scriptable record property that checks whether a given item is
     * contained in an array. This function is suitable for use with any type of array property other than array of
     * foreign keys to other TweakDB records.
     *
     * The resulting function will have the following signature, where [Prop] is the provided property name and [Type]
     * is the property type defined in the provided execution context:
     *
     * @code bool [Prop]Contains([Type] item)@endcode
     *
     * @param aClass
     * @param aName
     * @param aContext
     */
    void CreateArrayContains(ScriptableRecordClass* aClass, const std::string& aName, const ContextPtr& aContext);

    /**
     * @brief Creates and registers a function for a scriptable record property that retrieves the property's value from
     * TweakDB. This function is suitable for retrieving most scriptable record property types, including arrays, but is
     * not intended for use with foreign keys to other TweakDB records.
     *
     * The resulting function will have the following signature, where [Prop] is the provided property name and [Type]
     * is the property type defined in the provided execution context:
     *
     * @code [Type] [Prop]()@endcode
     *
     * @param aClass
     * @param aName
     * @param aContext
     */
    void CreateGet(ScriptableRecordClass* aClass, const std::string& aName, const ContextPtr& aContext);

    /**
     * @brief Creates the bytecode that implements the "script" function responsible for injecting a pointer to the
     * provided execution context into the callstack and calling the provided "native" function.
     *
     * @param aContext The execution context to provide to the function invocation via the call stack, containing
     * necessary details of the property being operated on.
     * @param aFunc The actual function to invoke when the property function is called, which will be called by the
     * generated bytecode.
     * @return A buffer containing the bytecode that implements the "script" function responsible for injecting a
     * pointer to the provided execution context into the callstack and calling the provided "native" function.
     */
    Red::RawBuffer CreateFunctionBytecode(const ContextPtr& aContext, Red::CBaseFunction* aFunc);

    /**
     * @brief Retrieves the scriptable record specification with the given name from the registry.
     *
     * @param aName The fully-qualified name of the record type corresponding to the desired specification.
     * @return A shared pointer to the scriptable record specification, or @c nullptr if no specification with the given
     * name was found.
     */
    ScriptableRecordSpecPtr GetRecordSpec(Red::CName aName) const;

    /**
     * @brief Creates an App::RecordClass and registers it with RTTI based on the provided record specification. After
     * completion, only the skeleton type will exist with its parent or any functions. If successful, the specification
     * may subsequently be described.
     *
     * @param aSpec The specification of the scriptable record type to create.
     * @return Whether the record class was successfully created and registered with RTTI.
     */
    bool RegisterScriptableRecordSpec(const ScriptableRecordSpecPtr& aSpec);

    /**
     * @brief Adds functions and a parent class to an App::RecordClass based on the provided record specification. After
     * completion, the record class will be fully described and ready for use. If successful, the scriptable record's
     * default property values may subsequently be inserted into TweakDB.
     *
     * @param aSpec The specification of the scriptable record type to describe.
     * @return Whether the record class was successfully described with functions and a parent class.
     */
    bool DescribeScriptableRecordSpec(const ScriptableRecordSpecPtr& aSpec);

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
    bool DescribeScriptablePropertySpec(ScriptableRecordClass* aClass, const ScriptablePropertySpecPtr& aSpec);

    /**
     * @brief Inserts default values for a scriptable record type based on the provided record specification.
     *
     * This involves creating an initial instance of the record type in TweakDB with the record ID @c
     * RTDB.<record_name>, where @c record_name is the short name of the record type, and inserting default values for
     * each direct property of the record with the TweakDB ID @c RTDB.<record_name>.<property_name>, where @c
     * property_name is the name of the property.
     *
     * @param aSpec
     */
    void InsertScriptableRecordDefaults(const ScriptableRecordSpecPtr& aSpec);

    /**
     * @brief Inserts default values for a scriptable record type based on the provided RTTI class. This is an overload
     * of the previous function that retrieves the record specification based on the class's name and then calls the
     * previous function to perform the insertion.
     *
     * @param aClass The RTTI class of the record type for which to insert default values. This should correspond to the
     * class of a registered scriptable record type.
     */
    void InsertScriptableRecordDefaults(const Red::CClass* aClass);

    /**
     * @brief Unregisters a scriptable record type by destroying its corresponding RTTI class and removing it from the
     * record class registry. After completion, the record type will no longer be functional or accessible, and its
     * specification will be reset to an unregistered state. This function is intended to be used for hot-reloading
     * scriptable record types during development.
     *
     * @param aSpec The specification of the scriptable record type to unregister.
     * @return Whether the record type was successfully unregistered and removed from the registry.
     */
    bool DeregisterScriptableRecord(const ScriptableRecordSpecPtr& aSpec);

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
    ScriptableRecordClass* CreateRecordClass(const ScriptableRecordSpecPtr& aSpec);

    static void NoOpScriptFunction(Red::IScriptable*, Red::CStackFrame*, void*, int64_t);

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
     * @brief A mutex for synchronizing access to the scriptable record specification registry for thread safety.
     */
    mutable std::shared_mutex m_specsMutex;

    /**
     * @brief A registry of scriptable record specifications, indexed by the CName of each record type's fully-qualified
     * name.
     */
    Core::Map<Red::CName, ScriptableRecordSpecPtr> m_specs;

    /**
     * @brief A mutex for synchronizing access to the scriptable record class registry for thread safety.
     */
    mutable std::shared_mutex m_classesMutex;

    /**
     * @brief A registry of scriptable record classes, indexed by the hash of each record type's short name.
     */
    Core::Map<uint32_t, Core::SharedPtr<ScriptableRecordClass>> m_classes;

    /**
     * @brief A pointer to the TweakDB manager.
     */
    Core::DeferredPtr<Red::TweakDBManager> m_tweakManager;

    /**
     * @brief A mutex for synchronizing access to the scriptable record property getter function execution contexts for
     * thread safety.
     */
    mutable std::shared_mutex m_contextsMutex;

    /**
     * @brief A collection of shared pointers to getter function execution contexts, kept alive for the lifetime of
     * this manager so that the raw pointers baked into bytecode remain valid.
     */
    Core::Map<Red::CName, Core::Map<Red::CName, ContextPtr>> m_contexts;
};

} // namespace App
