#pragma once

#include "App/Tweaks/Record/ScriptablePropertyHandler.hpp"
#include "App/Tweaks/Record/ScriptableRecordClass.hpp"
#include "App/Tweaks/Record/ScriptableRecordTypes.hpp"
#include "Core/Logging/LoggingAgent.hpp"
#include "Red/ScriptBundle.hpp"
#include "Red/TweakDB/Manager.hpp"

namespace App
{
/**
 * @brief A manager for handling the registration and lifecycle of scriptable record types, their properties, and the
 * underlying components that allow them to function.
 */
class ScriptableRecordManager : public Core::LoggingAgent
{
public:
    /**
     * @brief Constructs a ScriptableRecordManager instance with the given TweakDB manager.
     *
     * @param aManager A deferred pointer to the TweakDB manager used to interact with TweakDB.
     * @param aPropertyHandler A shared pointer to the ScriptablePropertyHandler used to register script functions for
     * scriptable record properties and handle their invocation at runtime.
     */
    explicit ScriptableRecordManager(const Core::DeferredPtr<Red::TweakDBManager>& aManager,
                                     const Core::SharedPtr<ScriptablePropertyHandler>& aPropertyHandler);

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
     * @return A shared pointer to the scriptable record's specification, or @c nullptr if registration failed for any
     * reason.
     */
    ScriptableRecordSpecPtr RegisterScriptableRecordType(const std::string& aName,
                                                         const std::optional<std::string>& aParentName = std::nullopt);

    /**
     * @brief Registers a scriptable property specification with a scriptable record type.
     *
     * @param aRecordName The name of the scriptable record type that this property belongs to.
     * @param aPropertyName The name of the property to register.
     * @param aTypeSpec The type specification of the property, containing both its TweakDB flat type details and the
     * property type of the getter function.
     * @param aDefaultValue The default value of the property that will be inherited by instances of the record type
     * when no explicit value is provided for the instance.
     * @return A shared pointer to the registered property specification, or @c nullptr if registration failed for any
     * reason.
     */
    ScriptablePropertySpecPtr RegisterScriptableProperty(const std::string& aRecordName,
                                                         const std::string& aPropertyName,
                                                         const TweakTypeSpecPtr& aTypeSpec,
                                                         const Red::InstancePtr<>& aDefaultValue = nullptr);
    /**
     * @brief Registers a scriptable property specification with a scriptable record type. This involves creating and
     * registering RTTI functions for the property based on the provided specification.
     *
     * @param aRecordSpec The specification of the scriptable record type that this property belongs to.
     * @param aPropertyName The name of the property to register.
     * @param aTypeSpec The type specification of the property, containing both its TweakDB flat type details and the
     * property type of the getter function.
     * @param aDefaultValue The default value of the property that will be inherited by instances of the record type
     * when no explicit value is provided for the instance.
     * @return A shared pointer to the registered property specification, or @c nullptr if registration failed for any
     * reason.
     */
    ScriptablePropertySpecPtr RegisterScriptableProperty(const ScriptableRecordSpecPtr& aRecordSpec,
                                                         const std::string& aPropertyName,
                                                         const TweakTypeSpecPtr& aTypeSpec,
                                                         const Red::InstancePtr<>& aDefaultValue = nullptr);

    /**
     * @brief Creates and registers RTTI classes for all pending scriptable record specifications registered with this
     * object. RTTI type registration only creates classes without any properties, functions, or inheritance.
     */
    void RegisterRTTITypes();

    /**
     * @brief Describes, or completes setup of, RTTI classes for all pending scriptable record specifications registered
     * with this object. During this process each scriptable record class will be assigned its parent class. Property
     * getter registrations will be staged with the @c ScriptablePropertyHandler but no functions will be created on the
     * class.
     */
    void DescribeRTTITypes();

    /**
     * @brief Inserts default values for all scriptable records specifications registered with this object into the
     * provided TweakDB manager. This should only be called after all specs have been registered and described.
     *
     * In particular, a TweakDB flat instance will be inserted into TweakDB with the default value for any direct
     * property of the record with the TweakDB ID
     * @c RTDB.<record_name>.<property_name>, where @c property_name is the name of the property.
     */
    void InsertDefaults();

    /**
     * @brief Inspects the provided script bundle for any script classes corresponding to registered scriptable record
     * types and, if any are found, modifies the bytecode of their functions to invoke the appropriate property handlers
     * at runtime.
     *
     * @param aClasses The array of script classes parsed from a script bundle to inspect for functions corresponding to
     * registered scriptable record types and modify to invoke property handlers at runtime.
     */
    void AdaptScriptClasses(const Red::DynArray<Red::ScriptClass*>& aClasses);

#ifndef NDEBUG
    /**
     * @brief A debug-enabled helper function for orchestrating the complete registration process of a scriptable record
     * specification for use in running test cases.
     *
     * @param aSpec The scriptable record specification to fully register, including RTTI registration, RTTI
     * description, and default value insertion into TweakDB.
     *
     * @return Whether the complete registration process for the given scriptable record specification was successful.
     * If this function returns false, the specification should not be used for testing as it may be in an incomplete or
     * invalid state.
     */
    bool SetupTestRecordSpec(const ScriptableRecordSpecPtr& aSpec);
#endif

private:
    /**
     * @brief Retrieves the scriptable record specification with the given name from the registry.
     *
     * @param aName The fully-qualified name of the record type corresponding to the desired specification.
     * @return A shared pointer to the scriptable record specification, or @c nullptr if no specification with the given
     * name was found.
     */
    ScriptableRecordSpecPtr GetRecordSpec(Red::CName aName) const;

    /**
     * @brief Creates a @c ScriptableRecordClass and registers it with RTTI based on the provided record specification.
     * After completion, only the skeleton type will exist with its parent or any functions. If successful, the
     * specification may subsequently be described after all other scriptable records have been registered.
     *
     * @param aSpec The specification of the scriptable record type to create.
     * @return Whether the record class was successfully created and registered with RTTI.
     */
    bool RegisterRTTIType(const ScriptableRecordSpecPtr& aSpec);

    /**
     * @brief Completes setup of a scriptable record type by describing its corresponding RTTI class based on the
     * provided record specification. This involves setting the class's parent based on the parent specification of the
     * record type, if it exists, and staging registration of property handler functions based on the property
     * specifications of the record type.
     *
     * After completion, the record class itself will be usable but will contain no functions until scripting parsing
     * and binding has been complete.
     *
     * @param aSpec The specification of the scriptable record type to describe.
     * @return Whether the record class was successfully described.
     */
    bool DescribeRTTIType(const ScriptableRecordSpecPtr& aSpec);

    /**
     * @brief Stages a property of a script function with one or more getter functions based on the given property
     * specification. This is a necessary step in the initialization process and sets up mappings for getter function
     * types based on functions parsed from RedScript.
     *
     * @param aRecordSpec The specification of the scriptable record type that this property belongs to.
     * @param aSpec The specification of the property to stage script getter functions for.
     * @return Whether the script function was successfully staged.
     */
    bool StagePropertyFunctions(const ScriptableRecordSpecPtr& aRecordSpec, const ScriptablePropertySpecPtr& aSpec);

    /**
     * @brief Inserts default values for a scriptable record type based on the provided record specification.
     *
     * This involves inserting default values for each direct property of the record with the TweakDB ID
     * @c RTDB.<record_name>.<property_name>, where @c property_name is the name of the property.
     *
     * @param aSpec The specification of the scriptable record type for which to insert default values. This should be a
     * valid record specification contained in the registry.
     */
    void InsertDefaults(const ScriptableRecordSpecPtr& aSpec);

    /**
     * @brief Inserts default values for a scriptable record type based on the provided RTTI class. This is an overload
     * of the previous function that retrieves the record specification based on the class's name and then calls the
     * previous function to perform the insertion.
     *
     * @param aClass The RTTI class of the record type for which to insert default values. This should correspond to the
     * class of a registered scriptable record type.
     */
    void InsertDefaults(const Red::CClass* aClass);

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

    /**
     * @brief Inspects the provided script class for any functions corresponding to the registered scriptable record
     * type with a name matching the class's name. For any that are found, their bytecode will be modified to invoke the
     * appropriate property handlers at runtime.
     *
     * @param aClassDef The script class parsed from RedScript to inspect for functions corresponding to the registered
     * scriptable record type with a name matching the class's name and modify to invoke property handlers at runtime.
     */
    void AdaptScriptClass(const Red::ScriptClass* aClassDef);

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
     * @brief A shared pointer to the registry for scriptable property getters.
     */
    Core::SharedPtr<ScriptablePropertyHandler> m_propertyHandler;
};
} // namespace App
