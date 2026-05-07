#pragma once

#include <string_view>

#include "Core/Logging/LoggingAgent.hpp"
#include "ScriptableRecordTypes.hpp"

namespace App
{
/**
 * @brief An enumeration of the different types of property getter functions that can be registered for scriptable
 * TweakDB records, used to identify the appropriate function handler for a given function based on its name. The
 * function handlers corresponding to each getter type are implemented in App::ScriptablePropertyHandler and its derived
 * classes.
 */
enum class GetterType : uint8_t
{
    GetRecordArray,
    RecordArrayContains,
    GetRecordItem,
    GetRecordItemHandle,
    GetRecord,
    GetRecordHandle,
    GetArrayCount,
    GetArrayItem,
    ArrayContains,
    GetResRefArray,
    GetResRefItem,
    GetResRef,
    Get
};

struct Context
{
    std::string appendix;
    GetterType getterType;
    TweakTypeSpecPtr typeSpec;
    Core::DeferredPtr<Red::TweakDBManager> tweakManager;
};

using ContextPtr = Core::SharedPtr<Context>;

/**
 * @brief A base class for handling the registration and invocation of script functions that serve as property getters
 * for scriptable TweakDB records. Each derived class corresponds to a specific type of getter function, identified by
 * the GetterType enumeration, and implements the necessary logic for:
 *
 * - Handling the function's invocation at runtime.
 * - Constructing a function hash based on the expected signature of the getter function.
 * - Defining the expected signature of the getter function.
 *
 * This class (and its derived classes) can only be created at compile time.
 */
class ScriptablePropertyHandler
{
public:
    /**
     * @brief Constructs a ScriptablePropertyHandler with the given type, prefix, and suffix.
     *
     * The prefix and suffix are used to generate the names of the script functions that this handler will manage, which
     * follow the format @c <prefix><property_name><suffix>. For example, if the prefix is @c Get and the suffix is @c
     * Item, a property named @c MyProperty would correspond to a function named @c GetMyPropertyItem.
     *
     * @param aPrefix The prefix added to the names of generated script functions for this handler.
     * @param aSuffix The suffix added to the names of generated script functions for this handler.
     */
    consteval explicit ScriptablePropertyHandler(const std::string_view aPrefix, const std::string_view aSuffix)
        : m_prefix(aPrefix)
        , m_suffix(aSuffix)
        , m_prefixLength(aPrefix.length())
        , m_suffixLength(aSuffix.length())
    {
    }

    /**
     * @brief Virtual destructor for ScriptablePropertyHandler.
     */
    virtual ~ScriptablePropertyHandler() = default;

    /**
     * @brief Handles the invocation of a script function corresponding to this property handler's specialization at
     * runtime, performing the necessary logic to retrieve the relevant property value from TweakDB based on the given
     * execution context.
     *
     * @param aInstance The scriptable record instance on which the function is being invoked.
     * @param aFrame The stack frame for the function invocation, which can be used to access the function's arguments.
     * @param aOut A pointer to the memory location where the function's return value should be stored, if applicable.
     * @param aContext The execution context containing the property specification and pointers to required services.
     */
    virtual void HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                                  const Context* aContext) const = 0;

    /**
     * @brief Generates the hash of the function name corresponding to the given record and property specifications for
     * this property handler. This has is used to establish a link between a function parsed from redscript to the
     * property specification associated with it.
     *
     * The function name is generated based on the naming convention defined by this handler's
     * prefix and suffix, as well as the function name specified in the property specification. For example, if the
     * prefix is @c Get and the suffix is @c Item, a property with the function name @c MyProperty would correspond to a
     * function named @c GetMyPropertyItem.
     *
     * The hash is a combination of various string segments joined by semicolons. The segment order is as follows:
     *
     * - Record class name.
     * - Function return type, or "void" if it does not have a return.
     * - Generated function name for the handler.
     * - Types for all function arguments, sequentially, if the function has any.
     *
     * @param aRecordSpec The specification of the scriptable record type that this function belongs to.
     * @param aPropSpec The specification of the property that this function serves as a getter for.
     * @return The hash of the function name corresponding to the given record and property specifications for this
     * property handler.
     */
    [[nodiscard]] virtual Red::CName GetFunctionHash(const ScriptableRecordSpecPtr& aRecordSpec,
                                                     const ScriptablePropertySpecPtr& aPropSpec) const = 0;

    /**
     * @brief Extracts the base property name from a given function name by removing the handler's prefix and suffix.
     * For example, if the handler's prefix is @c Get and suffix is @c Item, a function name @c GetMyPropertyItem would
     * yield a base property name of @c MyProperty.
     *
     * @param aName The function name to extract the base property name from.
     * @return The base property name extracted from the given function name, or an empty string if the function name
     * does not conform to the expected format with this handler's prefix and suffix.
     */
    [[nodiscard]] std::string GetFunctionBaseName(const std::string& aName) const;

protected:
    /**
     * @brief Retrieves the TweakDB ID of a property's flat value by appending the property's flat appendix to the given
     * scriptable record instance's TweakDB ID.
     *
     * @param aInstance The scriptable record instance for which to retrieve the record's ID.
     * @param aContext The execution context containing the property specification from which to retrieve the flat ID
     * appendix.
     * @return The TweakDB ID of the property's flat value, which is used to retrieve the flat instance from TweakDB at
     * runtime.
     */
    static Red::TweakDBID GetFlatID(Red::Instance aInstance, const Context* aContext);

    /**
     * @brief Retrieves an array of record handles from a TweakDB flat value corresponding to a property, if the flat
     * value is valid and of the expected type. This is used by various property handlers to retrieve the array of
     * records associated with a property from TweakDB at runtime.
     *
     * @tparam THandle The type of handle to use for the records in the array, which can be either Red::Handle or
     * Red::WeakHandle.
     * @param aValue The TweakDB flat value from which to retrieve the array of record handles. This is expected to be
     * an array of TweakDB IDs.
     * @param aContext The execution context containing the property specification from which to retrieve the expected
     * type of the records in the array.
     * @return A shared pointer to a dynamic array of record handles retrieved from the given flat value, or nullptr if
     * the flat value is invalid or not of the expected type.
     */
    template<template<typename> typename THandle>
        requires(std::is_same_v<THandle<Red::TweakDBRecord>, Red::Handle<Red::TweakDBRecord>> ||
                 std::is_same_v<THandle<Red::TweakDBRecord>, Red::WeakHandle<Red::TweakDBRecord>>)
    static Core::SharedPtr<Red::DynArray<THandle<Red::TweakDBRecord>>> GetRecordArray(const Red::Value<>& aValue,
                                                                                      const Context* aContext);
    /**
     * @brief A prefix added to the names of generated script functions for this property handler based on the name of
     * the property.
     *
     * For example, if the prefix is @c Get and the property name is @c MyProperty, the generated function
     * name would be updated to @c GetMyProperty.
     */
    const std::string_view m_prefix;

    /**
     * @brief A suffix added to the names of generated script functions for this property handler based on the name of
     * the property.
     *
     * For example, if the suffix is @c Item and the property name is @c MyProperty, the generated function name would
     * be updated to @c GetMyPropertyItem.
     */
    const std::string_view m_suffix;

    /**
     * @brief The length of the prefix string, cached for efficiency in extracting the base property name from function
     * names.
     */
    const size_t m_prefixLength;

    /**
     * @brief The length of the suffix string, cached for efficiency in extracting the base property name from function
     * names.
     */
    const size_t m_suffixLength;
};

/**
 * @brief A template class for handling the registration and invocation of script functions that serve as property
 * getters for scriptable TweakDB records, providing the ability to create specialized handlers per getter type.
 */
template<GetterType>
class TTypedPropertyHandler : public ScriptablePropertyHandler
{
public:
    /**
     * @brief Constructs a TTypedPropertyHandler with no name prefix or suffix.
     */
    consteval TTypedPropertyHandler()
        : ScriptablePropertyHandler("", "")
    {
    }

    /**
     * @brief Constructs a TTypedPropertyHandler with the given name suffix and no prefix.
     *
     * @param aSuffix The suffix added to the names of generated script functions for this handler.
     */
    consteval explicit TTypedPropertyHandler(const std::string_view aSuffix)
        : ScriptablePropertyHandler("", aSuffix)
    {
    }

    /**
     * @brief Constructs a TTypedPropertyHandler with the given name prefix and no suffix.
     *
     * @param aPrefix The prefix added to the names of generated script functions for this handler.
     * @param aSuffix The suffix added to the names of generated script functions for this handler.
     */
    consteval explicit TTypedPropertyHandler(const std::string_view aPrefix, const std::string_view aSuffix)
        : ScriptablePropertyHandler(aPrefix, aSuffix)
    {
    }
};

/**
 * @brief A property handler for getter functions that retrieve an array of foreign keys to TweakDB records from a
 * scriptable record property.
 *
 * The generated function name for this handler has no additional prefix or suffix beyond the base property name. For
 * example, a property named @c MyProperty would correspond to a function named @c MyProperty.
 *
 * The TweakDB flat associated with the property is expected to contain an array of TweakDB IDs that point to TweakDB
 * records of a specific type, as defined by the property. During processing, the results are validated to ensure that
 * target records conform to the expected type. On success, the handler will return an array of weak handles to the
 * target TweakDB records.
 */
class GetRecordArrayHandler : public TTypedPropertyHandler<GetterType::GetRecordArray>
{
public:
    consteval GetRecordArrayHandler() = default;
    [[nodiscard]] Red::CName GetFunctionHash(const ScriptableRecordSpecPtr& aRecordSpec,
                                             const ScriptablePropertySpecPtr& aPropSpec) const override;
    void HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                          const Context* aContext) const override;
};

/**
 * @brief A property handler for getter functions that check whether a TweakDB record is contained within an array of
 * foreign keys to TweakDB records associated with a scriptable record property.
 *
 * The generated function name for this handler has the suffix @c Contains and no prefix. For example, a property named
 * @c MyProperty would correspond to a function named @c MyPropertyContains.
 *
 * The TweakDB flat associated with the property is expected to contain an array of TweakDB IDs that point to TweakDB
 * records of a specific type, as defined by the property. During processing, the results are validated to ensure that
 * target records conform to the expected type. The handler takes a weak handle to a TweakDB record as an argument and
 * checks whether it is contained within the array of records associated with the property, returning true if it is and
 * false otherwise.
 */
class RecordArrayContainsHandler : public TTypedPropertyHandler<GetterType::RecordArrayContains>
{
public:
    consteval RecordArrayContainsHandler()
        : TTypedPropertyHandler("Contains")
    {
    }

    [[nodiscard]] Red::CName GetFunctionHash(const ScriptableRecordSpecPtr& aRecordSpec,
                                             const ScriptablePropertySpecPtr& aPropSpec) const override;
    void HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                          const Context* aContext) const override;
};

/**
 * @brief A property handler for getter functions that return a foreign key to a TweakDB record from an array of TweakDB
 * records from scriptable record property.
 *
 * The generated function name for this handler has the prefix @c Get and the suffix @c Item. For example, a property
 * named @c MyProperty would correspond to a function named @c GetMyPropertyItem.
 *
 * The TweakDB flat associated with the property is expected to contain an array of TweakDB IDs that point to TweakDB
 * records of a specific type, as defined by the property. During processing, the results are validated to ensure that
 * target records conform to the expected type. The handler takes an index as an argument and returns a weak handle to
 * the TweakDB record at that index within the array of records associated with the property.
 */
class GetRecordItemHandler : public TTypedPropertyHandler<GetterType::GetRecordItem>
{
public:
    consteval GetRecordItemHandler()
        : TTypedPropertyHandler("Get", "Item")
    {
    }

    [[nodiscard]] Red::CName GetFunctionHash(const ScriptableRecordSpecPtr& aRecordSpec,
                                             const ScriptablePropertySpecPtr& aPropSpec) const override;
    void HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                          const Context* aContext) const override;
};

/**
 * @brief A property handler for getter functions that return a foreign key to a TweakDB record from an array of TweakDB
 * records from scriptable record property, where the returned handle is a strong handle rather than a weak handle.
 *
 * The generated function name for this handler has the prefix @c Get and the suffix @c ItemHandle. For example, a
 * property named @c MyProperty would correspond to a function named @c GetMyPropertyItemHandle.
 *
 * The TweakDB flat associated with the property is expected to contain an array of TweakDB IDs that point to TweakDB
 * records of a specific type, as defined by the property. During processing, the results are validated to ensure that
 * target records conform to the expected type. The handler takes an index as an argument and returns a strong handle to
 * the TweakDB record at that index within the array of records associated with the property.
 */
class GetRecordItemHandleHandler : public TTypedPropertyHandler<GetterType::GetRecordItemHandle>
{
public:
    consteval GetRecordItemHandleHandler()
        : TTypedPropertyHandler("Get", "ItemHandle")
    {
    }

    [[nodiscard]] Red::CName GetFunctionHash(const ScriptableRecordSpecPtr& aRecordSpec,
                                             const ScriptablePropertySpecPtr& aPropSpec) const override;
    void HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                          const Context* aContext) const override;
};

/**
 * @brief A property handler for getter functions that retrieve a foreign key to a TweakDB record from a scriptable
 * record property.
 *
 * The generated function name for this handler has no additional prefix or suffix beyond the base property name. For
 * example, a property named @c MyProperty would correspond to a function named @c MyProperty.
 *
 * The TweakDB flat associated with the property is expected to contain a TweakDB ID that points to a TweakDB record of
 * a specific type, as defined by the property. During processing, the result is validated to ensure that the target
 * record conforms to the expected type. On success, the handler will return a weak handle to the target TweakDB record.
 */
class GetRecordHandler : public TTypedPropertyHandler<GetterType::GetRecord>
{
public:
    consteval GetRecordHandler() = default;

    [[nodiscard]] Red::CName GetFunctionHash(const ScriptableRecordSpecPtr& aRecordSpec,
                                             const ScriptablePropertySpecPtr& aPropSpec) const override;
    void HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                          const Context* aContext) const override;
};

/**
 * @brief A property handler for getter functions that retrieve a foreign key to a TweakDB record from a scriptable
 * record property, where the returned handle is a strong handle rather than a weak handle.
 *
 * The generated function name for this handler has the suffix @c Handle and no prefix. For example, a property named
 * @c MyProperty would correspond to a function named @c MyPropertyHandle.
 *
 * The TweakDB flat associated with the property is expected to contain a TweakDB ID that points to a TweakDB record of
 * a specific type, as defined by the property. During processing, the result is validated to ensure that the target
 * record conforms to the expected type. On success, the handler will return a strong handle to the target TweakDB
 * record.
 */
class GetRecordHandleHandler : public TTypedPropertyHandler<GetterType::GetRecordHandle>
{
public:
    consteval GetRecordHandleHandler()
        : TTypedPropertyHandler("Handle")
    {
    }

    [[nodiscard]] Red::CName GetFunctionHash(const ScriptableRecordSpecPtr& aRecordSpec,
                                             const ScriptablePropertySpecPtr& aPropSpec) const override;
    void HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                          const Context* aContext) const override;
};

/**
 * @brief A property handler for getter functions that retrieve the count of items in an array associated with a
 * scriptable record property.
 *
 * The generated function name for this handler has the prefix @c Get and the suffix @c Count. For example, a property
 * named
 * @c MyProperty would correspond to a function named @c GetMyPropertyCount.
 *
 * The TweakDB flat associated with the property is expected to contain an array of any type. During processing, the
 * result is validated to ensure that it is an array. On success, the handler will return the count of items in the
 * array.
 */
class GetArrayCountHandler : public TTypedPropertyHandler<GetterType::GetArrayCount>
{
public:
    consteval GetArrayCountHandler()
        : TTypedPropertyHandler("Get", "Count")
    {
    }

    [[nodiscard]] Red::CName GetFunctionHash(const ScriptableRecordSpecPtr& aRecordSpec,
                                             const ScriptablePropertySpecPtr& aPropSpec) const override;
    void HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                          const Context* aContext) const override;
};

/**
 * @brief A property handler for getter functions that retrieve an item at a specific index from an array associated
 * with a scriptable record property.
 *
 * The generated function name for this handler has the prefix @c Get and the suffix @c Item. For example, a property
 * named @c MyProperty would correspond to a function named @c GetMyPropertyItem.
 *
 * The TweakDB flat associated with the property is expected to contain an array of any type other than foreign keys to
 * other TweakDB records. During processing, the result is validated to ensure that it is an array and that its elements
 * are of the expected type as defined by the property specification. On success, the handler will return the item at
 * the specified index in the array.
 */
class GetArrayItemHandler : public TTypedPropertyHandler<GetterType::GetArrayItem>
{
public:
    consteval GetArrayItemHandler()
        : TTypedPropertyHandler("Get", "Item")
    {
    }

    [[nodiscard]] Red::CName GetFunctionHash(const ScriptableRecordSpecPtr& aRecordSpec,
                                             const ScriptablePropertySpecPtr& aPropSpec) const override;
    void HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                          const Context* aContext) const override;
};

/**
 * @brief A property handler for getter functions that check whether a specific item is contained within an array
 * associated with a scriptable record property.
 *
 * The generated function name for this handler has the suffix @c Contains and the prefix @c Get. For example, a
 * property named @c MyProperty would correspond to a function named @c GetMyPropertyContains.
 *
 * The TweakDB flat associated with the property is expected to contain an array of any type other than foreign keys to
 * other TweakDB records. During processing, the result is validated to ensure that it is an array and that its elements
 * are of the expected type as defined by the property specification. The handler takes an item as an argument and
 * checks whether it is contained within the array associated with the property, returning true if it is and false
 * otherwise.
 */
class ArrayContainsHandler : public TTypedPropertyHandler<GetterType::ArrayContains>
{
public:
    consteval ArrayContainsHandler()
        : TTypedPropertyHandler("Contains")
    {
    }

    [[nodiscard]] Red::CName GetFunctionHash(const ScriptableRecordSpecPtr& aRecordSpec,
                                             const ScriptablePropertySpecPtr& aPropSpec) const override;
    void HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                          const Context* aContext) const override;
};

/**
 * @brief A property handler for getter functions that retrieve an array of resource references from a scriptable record
 * property.
 *
 * The generated function name for this handler has no additional prefix or suffix beyond the base property name. For
 * example, a property named @c MyProperty would correspond to a function named @c MyProperty.
 *
 * The TweakDB flat associated with the property is expected to contain an array of @c RaRef<CResource>. During
 * processing, the result is validated to ensure that it is an array and that its elements are of the expected type. On
 * success, the handler will return the array of @c ResRef .
 */
class GetResRefArrayHandler : public TTypedPropertyHandler<GetterType::GetResRefArray>
{
public:
    consteval GetResRefArrayHandler() = default;
    [[nodiscard]] Red::CName GetFunctionHash(const ScriptableRecordSpecPtr& aRecordSpec,
                                             const ScriptablePropertySpecPtr& aPropSpec) const override;
    void HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                          const Context* aContext) const override;
};

/**
 * @brief A property handler for getter functions that retrieve an item at a specific index from an array of resource
 * references associated with a scriptable record property.
 *
 * The generated function name for this handler has the prefix @c Get and the suffix @c Item. For example, a property
 * named @c MyProperty would correspond to a function named @c GetMyPropertyItem.
 *
 * The TweakDB flat associated with the property is expected to contain an array of @c RaRef<CResource>. During
 * processing, the result is validated to ensure that it is of the expected type. On success, the handler will return
 * the @c ResRef at the specified index in the array.
 */
class GetResRefItemHandler : public TTypedPropertyHandler<GetterType::GetResRefItem>
{
public:
    consteval GetResRefItemHandler()
        : TTypedPropertyHandler("Get", "Item")
    {
    }

    [[nodiscard]] Red::CName GetFunctionHash(const ScriptableRecordSpecPtr& aRecordSpec,
                                             const ScriptablePropertySpecPtr& aPropSpec) const override;
    void HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                          const Context* aContext) const override;
};

/**
 * @brief A property handler for getter functions that retrieve a resource reference from a scriptable record property.
 *
 * The generated function name for this handler has no additional prefix or suffix beyond the base property name. For
 * example, a property named @c MyProperty would correspond to a function named @c MyProperty.
 *
 * The TweakDB flat associated with the property is expected to contain a @c RaRef<CResource>. During processing, the
 * result is validated to ensure that it is of the expected type. On success, the handler will return the @c ResRef .
 */
class GetResRefHandler : public TTypedPropertyHandler<GetterType::GetResRef>
{
public:
    consteval GetResRefHandler() = default;
    [[nodiscard]] Red::CName GetFunctionHash(const ScriptableRecordSpecPtr& aRecordSpec,
                                             const ScriptablePropertySpecPtr& aPropSpec) const override;
    void HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                          const Context* aContext) const override;
};

/**
 * @brief A property handler for getter functions that retrieve a simple value (i.e., a value that is simply returned
 * from its TweakDB flat) from a scriptable record property.
 *
 * The generated function name for this handler has no additional prefix or suffix beyond the base property name. For
 * example, a property named @c MyProperty would correspond to a function named @c MyProperty.
 *
 * The TweakDB flat associated with the property is expected to contain a value of a simple type (e.g., int, float,
 * bool, enum, arrays, etc.) that can be directly returned without additional processing. During processing, the result
 * is validated to ensure that it is of the expected type as defined by the property specification. On success, the
 * handler will return the value from TweakDB.
 */
class GetValueHandler : public TTypedPropertyHandler<GetterType::Get>
{
public:
    consteval GetValueHandler() = default;

    [[nodiscard]] Red::CName GetFunctionHash(const ScriptableRecordSpecPtr& aRecordSpec,
                                             const ScriptablePropertySpecPtr& aPropSpec) const override;
    void HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                          const Context* aContext) const override;
};

/**
 * @brief A registry for managing the registration and invocation of script functions that serve as property getters for
 * scriptable TweakDB records.
 */
class ScriptablePropertyHandlers
    : Core::LoggingAgent
    , Core::ShareFromThis<ScriptablePropertyHandlers>
{
public:
    /**
     * @brief Constructs a ScriptablePropertyHandlerRegistryScriptablePropertyHandlers instance with the given TweakDB
     * manager.
     *
     * @param aManager A deferred pointer to the TweakDB manager used to retrieve scriptable record property values from
     * TweakDB at runtime.
     */
    explicit ScriptablePropertyHandlers(const Core::DeferredPtr<Red::TweakDBManager>& aManager);

    /**
     * @brief Registers the global invocation handler function that serves as the single entry point for all property
     * getter functions with RTTI.
     */
    void RegisterInvocationHandler();

    /**
     * @brief Registers a script function as a property getter for a scriptable record type based on the given record
     * and property specifications.
     *
     * This process does not create any functions with RTTI or the scripting system, but merely stages downstream
     * processing so that functions parsed from redscript can be modified to invoke the correct property handler. Based
     * on the characteristics of the property one or many functions will be created according to typical TweakDB
     * conventions.
     *
     * This function must be called for each valid property of a scriptable record in order to function correctly.
     *
     * @param aRecordSpec The specification of the scriptable record type that this property belongs to.
     * @param aPropSpec The specification of the property that this function serves as a getter for.
     */
    void RegisterScriptableProperty(const ScriptableRecordSpecPtr& aRecordSpec,
                                    const ScriptablePropertySpecPtr& aPropSpec);

    /**
     * @brief Checks whether a given script function corresponds to a registered property handler for a scriptable
     * record type based on the given record specification, and if so, modifies the function's bytecode to invoke the
     * appropriate property handler at runtime.
     *
     * This process relies on a property of a scriptable record having previously been provided to the @c
     * RegisterScriptableProperty function so that the hash of the function's signature can be mapped to the appropriate
     * getter handler type.
     *
     * @param aRecordSpec The specification of the scriptable record type that this function belongs to.
     * @param aFunc The script function to check and potentially modify to invoke a property handler at runtime.
     * @return Whether the given function corresponds to a registered property handler and was successfully modified to
     * invoke the handler at runtime.
     */
    bool AdaptScriptFunction(const ScriptableRecordSpecPtr& aRecordSpec, Red::CClassFunction* aFunc);

private:
    /**
     * @brief The name of the global function registered with RTTI that serves as the invocation handler for all
     * property handler functions.
     *
     * This function is responsible for dispatching calls to the appropriate property
     * handler based on the execution context passed to it at runtime.
     */
    static constexpr auto InvocationHandlerName = "_ScriptablePropertyInvocationHandler";

    /**
     * @brief Handles the invocation of a script function that serves as a property getter for a scriptable record type
     * by dispatching the call to the appropriate property handler based on the execution context passed to it at
     * runtime.
     *
     * This function is registered with RTTI as a global function and serves as the single entry point for all property
     * handler function calls from redscript.
     *
     * When invoked, this function expects a pointer to the invocation's execution context as the first element in the
     * call stack frame. From this context, the handler can determine which specialized handler type may service the
     * request.
     *
     * @param aInstance The scriptable instance on which the function was invoked, which should be an instance of a
     * scriptable record type.
     * @param aFrame The call stack frame for this function invocation, which should contain a pointer to the execution
     * context as its first element.
     * @param aOut A pointer to the memory location where the function's return value should be written, if it has a
     * return value.
     * @param a4 The expected type of the return value. This is unused.
     */
    static void HandleInvocation(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut, int64_t a4);

    /**
     * @brief Retrieves the script execution context from the given stack frame. After execution of this function
     * completes, the stack frame will be advanced past the pointer to the execution context.
     *
     * @param aFrame The stack frame provided to the handler's invocation from which to retrieve the execution context.
     * @return A pointer to the execution context.
     */
    static Context* GetContext(Red::CStackFrame* aFrame);

    /**
     * @brief Retrieves the property handler corresponding to the given getter type, if it has been registered. This is
     * used to determine which handler to use for a given function based on the function's signature hash.
     *
     * @param aType The type of getter function for which to retrieve the handler.
     * @return A pointer to the property handler corresponding to the given getter type
     */
    static ScriptablePropertyHandler* GetHandler(GetterType aType);

    /**
     * @brief Retrieves the script execution context corresponding to the given record and property specifications,
     * creating and registering it if it does not already exist. The context is used to store necessary information for
     * the execution of a property handler function, such as the property specification and pointers to necessary
     * services.
     *
     * @param aGetterType The type of getter function for which to retrieve the context, used to determine which
     * property handler to use at runtime.
     * @param aRecordSpec The specification of the scriptable record type to create a context for.
     * @param aPropSpec The specification of the property to create a context for.
     * @return A shared pointer to the script execution context corresponding to the given record and property
     * specifications. If the context did not already exist, it will be created and registered before being returned.
     */
    ContextPtr CreateContext(GetterType aGetterType, const ScriptableRecordSpecPtr& aRecordSpec,
                             const ScriptablePropertySpecPtr& aPropSpec);

    /**
     * @brief Registers a script function as a property getter for a scriptable record type based on the given record
     * and property specifications for a specific getter type. As opposed to @c RegisterScriptableProperty, this
     * function is responsible for registering a single function corresponding to a specific getter type based on the
     * property.
     *
     * @param aRecordSpec The specification of the scriptable record type that this property belongs to.
     * @param aPropSpec The specification of the property that this function serves as a getter for.
     */
    template<GetterType>
    void RegisterPropertyFunction(const ScriptableRecordSpecPtr& aRecordSpec,
                                  const ScriptablePropertySpecPtr& aPropSpec);

    /**
     * @brief Modifies a given script function to invoke the appropriate property handler for a scriptable record type
     * at runtime based on the given record specification and the function's signature hash. This involves replacing the
     * function's bytecode with bytecode generated to invoke the appropriate property handler and passing the necessary
     * execution context to the handler at runtime.
     *
     * @param aFunction The script function to modify to invoke a property handler at runtime.
     * @param aContext The execution context to pass to the property handler at runtime.
     */
    void ReplaceScriptFunction(Red::CClassFunction* aFunction, const ContextPtr& aContext) const;

    /**
     * @brief Truncates the bytecode of a given script function, effectively removing all existing instructions from the
     * function. This is useful when a hot reload of TweakXL is performed and a scriptable record type or one of its
     * properties is removed. Affected functions need to have their bytecode removed to prevent invalid memory access
     * from occurring if they are invoked at runtime after the record or property they were associated with has been
     * removed.
     *
     * @param aFunction The script function for which to truncate the bytecode.
     */
    static void TruncateScriptFunction(Red::CClassFunction* aFunction);

    /**
     * @brief Generates the script bytecode for a given script function to invoke a property handler for a scriptable
     * record type at runtime based on the given execution context and native function object corresponding to the
     * property handler. This is used to replace the bytecode of a script function with bytecode that invokes the
     * appropriate property handler at runtime.
     *
     * The function does not modify the given script function directly, but it does inspect it to ensure the correct
     * bytecode layout for its function signature.
     *
     * @param aContext The execution context containing necessary information for the execution of the property handler
     * function, such as the property specification and pointers to necessary services.
     * @param aFunction The script function for which to generate the bytecode to invoke a property handler at runtime.
     * @return The generated bytecode for the given script function to invoke a property handler for a scriptable record
     * type at runtime based on the given execution context and native function object corresponding to the property
     * handler.
     */
    Red::RawBuffer CreateFunctionBytecode(const ContextPtr& aContext, Red::CClassFunction* aFunction) const;

    /**
     * @brief Generates the hash of the function name corresponding to the given record specification and function for a
     * property handler. This hash is used to establish a link between a function parsed from redscript to the property
     * specification associated with it based on the function's signature, which allows the correct property handler to
     * be determined for the function.
     *
     * The function name is generated based on the naming convention defined by the property handler's prefix and
     * suffix, as well as the function name specified in the property specification. For example, if the prefix is @c
     * Get and the suffix is @c Item, a property with the function name @c MyProperty would correspond to a function
     * named @c GetMyPropertyItem.
     *
     * The hash is a combination of various string segments joined by semicolons. The segment order is as follows:
     *
     * - Record class name.
     * - Function return type, or "void" if it does not have a return.
     * - Generated function name for the handler.
     * - Types for all function arguments, sequentially, if the function has any.
     *
     * @param aRecordSpec The specification of the scriptable record type that this function belongs to.
     * @param aFunction The redscript script function for which to generate the hash of the function name corresponding
     * to the given record specification and function for a property handler.
     * @return The hash of the function name corresponding to the given record specification and function for a property
     * handler.
     */
    static Red::CName GetFunctionHash(const ScriptableRecordSpecPtr& aRecordSpec, Red::CClassFunction* aFunction);

    /**
     * @brief A deferred pointer to the TweakDB manager used to retrieve scriptable record property values from TweakDB
     * at runtime. This is used by property handlers to access TweakDB when retrieving property values for scriptable
     * records.
     */
    Core::DeferredPtr<Red::TweakDBManager> m_manager;

    /**
     * @brief A pointer to the RTTI system used to register the global invocation handler function and generate bytecode
     * that invokes it at runtime. This is used to interact with the scripting system for handling the invocation of
     * property handler functions from redscript.
     */
    Red::CRTTISystem* m_rtti;

    /**
     * @brief A mutex for synchronizing access to the mapping of function signature hashes to getter handler types for
     * thread safety.
     */
    std::shared_mutex m_functionTypesMutex;

    /**
     * @brief A pointer to the global function registered with RTTI that serves as the invocation handler for all
     * property handler functions.
     */
    Red::CGlobalFunction* m_invocationHandler = nullptr;

    /**
     * @brief A mapping of function signature hashes to getter handler types, indexed by the CName of the record class
     * name and the CName of the function name corresponding to the property specification for a script function. This
     * is used to determine which property handler to invoke for a given script function based on the function's
     * signature when adapting script functions to invoke property handlers at runtime.
     */
    Core::Map<Red::CName, Core::Map<Red::CName, GetterType>> m_functionTypes;

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

    static inline GetRecordArrayHandler s_getRecordArrayHandler{};
    static inline RecordArrayContainsHandler s_recordArrayContainsHandler{};
    static inline GetRecordItemHandler s_getRecordItemHandler{};
    static inline GetRecordItemHandleHandler s_getRecordItemHandleHandler{};
    static inline GetRecordHandler s_getRecordHandler{};
    static inline GetRecordHandleHandler s_getRecordHandleHandler{};
    static inline GetArrayCountHandler s_getArrayCountHandler{};
    static inline GetArrayItemHandler s_getArrayItemHandler{};
    static inline ArrayContainsHandler s_getArrayContainsHandler{};
    static inline GetResRefArrayHandler s_getResRefArrayHandler{};
    static inline GetResRefItemHandler s_getResRefItemHandler{};
    static inline GetResRefHandler s_getResRefHandler{};
    static inline GetValueHandler s_getValueHandler{};
};

} // namespace App
