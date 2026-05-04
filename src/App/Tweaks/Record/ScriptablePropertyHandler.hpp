#pragma once

#include "ScriptableRecordManager.hpp"

namespace App
{
/**
 * @brief A static utility class responsible for handling the functions related to properties of scriptable TweakDB
 * records. The function handlers contained within this class are reusable across any dynamically-defined TweakDB record
 * type.
 *
 * Crucially, all function handlers expect an execution context to be provided to the invocation via a pointer in the
 * call stack directly after the ParamEnd opcode. This context contains necessary information for the function handlers
 * to determine which property is being accessed and details about the flat and property types, as well as pointers to
 * @c Red::TweakDBManager and @c App::ScriptableRecordManager.
 */
class ScriptablePropertyHandler
{
public:
    /**
     * @brief A type alias for a strong handle to a TweakDB record instance.
     */
    using RecordHandle = ScriptableRecordManager::RecordHandle;

    /**
     * @brief A type alias for a weak handle to a TweakDB record instance.
     */
    using RecordWHandle = ScriptableRecordManager::RecordWHandle;

    /**
     * @brief A type alias for an array of weak handles to TweakDB record instances.
     */
    using RecordArray = ScriptableRecordManager::RecordArray;

    /**
     * @brief A type alias for a shared pointer to an array of weak handles to TweakDB record instances.
     */
    using RecordArrayPtr = ScriptableRecordManager::RecordArrayPtr;

    /**
     * @brief A type alias for the execution context used in scriptable property handlers, containing necessary
     * information for property access and manipulation.
     */
    using Context = ScriptableRecordManager::Context;

    /**
     * @brief Deletes the default constructor to prevent instantiation of the ScriptablePropertyHandler class, as it is
     * intended to be used as a static utility class for handling functions related to properties of scriptable TweakDB
     * records.
     */
    ScriptablePropertyHandler() = delete;

    void RegisterFunctions();

    template<auto T>
    void RegisterFunction(const std::function<void(Red::CGlobalFunction*)>& aCustomizer);

    /**
     * @brief Handles the retrieval of an array of foreign keys to other TweakDB record instances from a scriptable
     * TweakDB record property.
     *
     * While the TweakDB flat instance associated with this property contains an array TweakDB record IDs, the array
     * returned from this function will consist of weak handles to the actual instances of the TweakDB records pointed
     * to by those IDs.
     *
     * This function will ensure that each target TweakDB record is of the expected type.
     *
     * @param aInstance The scriptable TweakDB record instance from which the property is being accessed.
     * @param aFrame The stack frame of the script execution, containing any relevant function argument pointers and the
     * pointer to the execution context.
     * @param aOut The output pointer where the resulting array of weak handles to TweakDB record instances will be
     * stored.
     * @param a4 The hash of the expected return type. This is unused.
     */
    static void GetRecordArrayHandler(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut, int64_t a4);

    /**
     * @brief Handles the retrieval of the number of elements in an array property of a scriptable TweakDB record.
     *
     * @param aInstance The scriptable TweakDB record instance from which the property is being accessed.
     * @param aFrame The stack frame of the script execution, containing any relevant function argument pointers and the
     * pointer to the execution context.
     * @param aOut The output pointer where the resulting array count will be stored as an integer.
     * @param a4 The hash of the expected return type. This is unused.
     */
    static void GetArrayCountHandler(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut, int64_t a4);

    /**
     * @brief Handles the retrieval of a foreign key to a TweakDB record from an array of foreign keys property of a
     * scriptable TweakDB record by its index. The target TweakDB record, if found, will be returned as a weak handle to
     * the TweakDB record instance.
     *
     * This function will ensure that the provided array index is within bounds and that the target TweakDB record is of
     * the expected type.
     *
     * @param aInstance The scriptable TweakDB record instance from which the property is being accessed.
     * @param aFrame The stack frame of the script execution, containing any relevant function argument pointers and the
     * pointer to the execution context.
     * @param aOut The output pointer where the resulting weak handle to the TweakDB record instance will be stored.
     * @param a4 The hash of the expected return type. This is unused.
     */
    static void GetRecordItemHandler(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut, int64_t a4);

    /**
     * @brief Handles the retrieval of a foreign key to a TweakDB record from an array of foreign keys property of a
     * scriptable TweakDB record by its index. The target TweakDB record, if found, will be returned as a strong handle
     * to the TweakDB record instance.
     *
     * @param aInstance The scriptable TweakDB record instance from which the property is being accessed.
     * @param aFrame The stack frame of the script execution, containing any relevant function argument pointers and the
     * pointer to the execution context.
     * @param aOut The output pointer where the resulting strong handle to the TweakDB record instance will be stored.
     * @param a4 The hash of the expected return type. This is unused.
     */
    static void GetRecordItemHandleHandler(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                                           int64_t a4);

    /**
     * @brief Handles the check for whether a given TweakDB record instance, provided as a weak handle, is contained
     * within an array of foreign keys belonging to a property of a scriptable TweakDB record.
     *
     * @param aInstance The scriptable TweakDB record instance from which the property is being accessed.
     * @param aFrame The stack frame of the script execution, containing any relevant function argument pointers and the
     * pointer to the execution context.
     * @param aOut The output pointer where the resulting boolean value indicating whether the record is contained in
     * the array will be stored.
     * @param a4 The hash of the expected return type. This is unused.
     */
    static void RecordArrayContainsHandler(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                                           int64_t a4);

    /**
     * @brief Handles the retrieval of a foreign key to a TweakDB record from a property of a scriptable TweakDB record.
     * The target TweakDB record, if found, will be returned as a weak handle to the TweakDB record instance.
     *
     * @param aInstance The scriptable TweakDB record instance from which the property is being accessed.
     * @param aFrame The stack frame of the script execution, containing any relevant function argument pointers and the
     * pointer to the execution context.
     * @param aOut The output pointer where the resulting weak handle to the TweakDB record instance will be stored.
     * @param a4 The hash of the expected return type. This is unused.
     */
    static void GetRecordHandler(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut, int64_t a4);

    /**
     * @brief Handles the retrieval of a foreign key to a TweakDB record from a property of a scriptable TweakDB record.
     * The target TweakDB record, if found, will be returned as a strong handle to the TweakDB record instance.
     *
     * @param aInstance The scriptable TweakDB record instance from which the property is being accessed.
     * @param aFrame The stack frame of the script execution, containing any relevant function argument pointers and the
     * pointer to the execution context.
     * @param aOut The output pointer where the resulting strong handle to the TweakDB record instance will be stored.
     * @param a4   The hash of the expected return type. This is unused.
     */
    static void GetRecordHandleHandler(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut, int64_t a4);

    /**
     * @brief Handles the retrieval of an item from an array property of a scriptable TweakDB record by its index.
     *
     * @param aInstance The scriptable TweakDB record instance from which the property is being accessed.
     * @param aFrame The stack frame of the script execution, containing any relevant function argument pointers and the
     * pointer to the execution context.
     * @param aOut The output pointer where the resulting item from the array will be stored.
     * @param a4 The hash of the expected return type. This is unused.
     */
    static void GetArrayItemHandler(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut, int64_t a4);

    /**
     * @brief Handles the check for whether a given element is contained within an array property of a scriptable
     * TweakDB record.
     *
     * @param aInstance The scriptable TweakDB record instance from which the property is being accessed.
     * @param aFrame The stack frame of the script execution, containing any relevant function argument pointers and the
     * pointer to the execution context.
     * @param aOut The output pointer where the resulting boolean value indicating whether the element is contained in
     * the array will be stored.
     * @param a4 The hash of the expected return type. This is unused.
     */
    static void ArrayContainsHandler(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut, int64_t a4);

    /**
     * @brief Handles the retrieval of a property value from a scriptable TweakDB record where the property's value does
     * not require any additional handling.
     *
     * @param aInstance The scriptable TweakDB record instance from which the property is being accessed.
     * @param aFrame The stack frame of the script execution, containing any relevant function argument pointers and the
     * pointer to the execution context.
     * @param aOut The output pointer where the resulting property value will be stored.
     * @param a4 The hash of the expected return type. This is unused.
     */
    static void GetHandler(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut, int64_t a4);

    template<auto V>
    static consteval std::array<char, nameof::nameof_pointer<V>().size() + 2> CreateNativeFunctionName()
    {
        constexpr auto baseName = ::nameof::nameof_pointer<V>();
        static_assert(!baseName.empty(), "Handler function name cannot be empty.");

        std::array<char, baseName.size() + 2> name{};
        name[0] = '_';
        for (size_t i = 0; i < baseName.size(); ++i)
        {
            name[i + 1] = baseName[i];
        }
        name[baseName.size() + 1] = '\0';

        return name;
    }

    template<auto V>
    static std::string GetNativeFunctionName()
    {
        return CreateNativeFunctionName<V>().data();
    }

private:
    /**
     * @brief The size of an opcode in the script VM, used for calculating parameter offsets in the stack frame when
     * handling function calls.
     */
    static constexpr auto OpSize = sizeof(char);

    /**
     * @brief The size of a pointer on the current platform, used for calculating parameter offsets in the stack frame
     * when handling function calls.
     */
    static constexpr auto PtrSize = sizeof(void*);

    /**
     * @brief Retrieves the TweakDB ID corresponding to the property being accessed on a scriptable TweakDB record
     * instance, using the provided execution context to determine the appropriate appendix to append to the record ID.
     *
     * @param aInstance The scriptable TweakDB record instance from which the property is being accessed.
     * @param aContext The execution context containing information about the property being accessed, including the
     * appendix to append to the record ID.
     * @return The TweakDB ID corresponding to the property being accessed.
     */
    static Red::TweakDBID GetFlatID(Red::Instance aInstance, const ScriptableRecordManager::Context* aContext);

    /**
     * @brief Retrieves an array of weak handles to TweakDB record instances from a property of a scriptable TweakDB
     * record. The provided value is expected to be the flat value of the property being accessed, in the form of an
     * array of TweakDB IDs. The provided execution context provides the type information of the expected TweakDB record
     * and is used to ensure the expected types are returned.
     *
     * @param aValue The flat value of the property being accessed, expected to be an array of TweakDB IDs.
     * @param aContext The execution context containing information about the property being accessed, including the
     * expected type of the TweakDB records pointed to by the TweakDB IDs in the array.
     * @return An array of weak handles to TweakDB record instances corresponding to the TweakDB IDs in the provided
     * value.
     */
    static RecordArrayPtr GetRecordArray(const Red::Value<>& aValue, const Context* aContext);

    /**
     * @brief Retrieves a strong handle to a TweakDB record instance from an array of foreign keys property of a
     * scriptable TweakDB record by its index. The provided value is expected to be the flat value of the property being
     * accessed, in the form of an array of TweakDB IDs. The provided execution context provides the type information of
     * the expected TweakDB record and is used to ensure the expected type is returned.
     *
     * @param aValue The flat value of the property being accessed, expected to be an array of TweakDB IDs.
     * @param aContext The execution context containing information about the property being accessed, including the
     * expected type of the TweakDB record pointed to by the TweakDB ID at the specified index in the array.
     * @param aIndex The index of the TweakDB ID in the array for which to retrieve the corresponding TweakDB record
     * instance.
     * @return A strong handle to the TweakDB record instance corresponding to the TweakDB ID at the specified index in
     * the array.
     */
    static RecordHandle GetRecordItemHandle(const Red::Value<>& aValue, const Context* aContext, int aIndex);

    /**
     * @brief Retrieves a weak handle to a TweakDB record instance from an array of foreign keys property of a
     * scriptable TweakDB record by its index. The provided value is expected to be the flat value of the property being
     * accessed, in the form of an array of TweakDB IDs. The provided execution context provides the type information of
     * the expected TweakDB record and is used to ensure the expected type is returned.
     *
     * @param aValue The flat value of the property being accessed, expected to be an array of TweakDB IDs.
     * @param aContext The execution context containing information about the property being accessed, including the
     * expected type of the TweakDB record pointed to by the TweakDB ID at the specified index in the array.
     * @param aIndex The index of the TweakDB ID in the array for which to retrieve the corresponding TweakDB record
     * instance.
     * @return A weak handle to the TweakDB record instance corresponding to the TweakDB ID at the specified index in
     * the array.
     */
    static RecordWHandle GetRecordItem(const Red::Value<>& aValue, const Context* aContext, int aIndex);

    /**
     * @brief Checks whether a given TweakDB record instance, provided as a weak handle, is contained within an array of
     * foreign keys belonging to a property of a scriptable TweakDB record. The provided value is expected to be the
     * flat value of the property being accessed, in the form of an array of TweakDB IDs. The provided execution context
     * provides the type information of the expected TweakDB record and is used to ensure the expected types are
     * checked.
     *
     * @param aValue The flat value of the property being accessed, expected to be an array of TweakDB IDs.
     * @param aContext The execution context containing information about the property being accessed, including the
     * expected type of the TweakDB record pointed to by the TweakDB IDs in the array.
     * @param aRecord The weak handle to the TweakDB record instance for which to check containment within the array of
     * foreign keys.
     * @return A boolean value indicating whether the given TweakDB record instance is contained within the array of
     * foreign keys.
     */
    static bool RecordArrayContains(const Red::Value<>& aValue, const Context* aContext, const RecordWHandle& aRecord);

    /**
     * @brief Retrieves a weak handle to a TweakDB record instance from a property of a scriptable TweakDB record. The
     * provided value is expected to be the flat value of the property being accessed, in the form of a TweakDB ID. The
     * provided execution context provides the type information of the expected TweakDB record and is used to ensure the
     * expected type is returned.
     *
     * @param aValue The flat value of the property being accessed, expected to be a TweakDB ID.
     * @param aContext The execution context containing information about the property being accessed, including the
     * expected type of the TweakDB record pointed to by the TweakDB ID in the value.
     * @return A weak handle to the TweakDB record instance corresponding to the TweakDB ID in the provided value.
     */
    static RecordWHandle GetRecord(const Red::Value<>& aValue, const Context* aContext);

    /**
     * @brief Retrieves a strong handle to a TweakDB record instance from a property of a scriptable TweakDB record. The
     * provided value is expected to be the flat value of the property being accessed, in the form of a TweakDB ID. The
     * provided execution context provides the type information of the expected TweakDB record and is used to ensure the
     * expected type is returned.
     *
     * @param aValue The flat value of the property being accessed, expected to be a TweakDB ID.
     * @param aContext The execution context containing information about the property being accessed, including the
     * expected type of the TweakDB record pointed to by the TweakDB ID in the value.
     * @return A strong handle to the TweakDB record instance corresponding to the TweakDB ID in the provided value.
     */
    static RecordHandle GetRecordHandle(const Red::Value<>& aValue, const Context* aContext);

    /**
     * @brief Retrieves the number of elements in an array property of a scriptable TweakDB record. The provided value
     * is expected to be the flat value of the property being accessed, in the form of an array. The provided execution
     * context provides the type information of the expected array.
     *
     * @param aValue The flat value of the property being accessed, expected to be an array.
     * @param aContext The execution context containing information about the property being accessed, including the
     * expected type of the array.
     * @return The number of elements in the array property of the scriptable TweakDB record.
     */
    static uint32_t GetArrayCount(const Red::Value<>& aValue, const Context* aContext);

    /**
     * @brief Checks whether a given element is contained within an array property of a scriptable TweakDB record. The
     * provided value is expected to be the flat value of the property being accessed, in the form of an array. The
     * provided execution context provides the type information of the expected array.
     *
     * @param aValue The flat value of the property being accessed, expected to be an array.
     * @param aContext The execution context containing information about the property being accessed, including the
     * expected type of the array.
     * @param item The element for which to check containment within the array property of the scriptable TweakDB
     * record.
     * @return A boolean value indicating whether the given element is contained within the array property of the
     * scriptable TweakDB record.
     */
    static bool ArrayContains(const Red::Value<>& aValue, const Context* aContext, Red::Instance item);

    /**
     * @brief Retrieves the execution context for a scriptable property handler function from the provided stack frame
     * of the script execution.
     *
     * The execution context contains necessary information for property access and manipulation, such as the TweakDB
     * manager instance, the property specification, and any relevant appendix for constructing TweakDB IDs.
     *
     * The frame's code pointer will be moved past the context's memory location as part of this operation.
     *
     * @param aFrame The stack frame of the script execution, containing any relevant function argument pointers and the
     * pointer to the execution context.
     * @return A pointer to the execution context for the scriptable property handler function.
     */
    static const Context* GetContext(Red::CStackFrame* aFrame);
};

template<auto T>
void ScriptablePropertyHandler::RegisterFunction(const std::function<void(Red::CGlobalFunction*)>& aCustomizer)
{
    static auto* rtti = Red::CRTTISystem::Get();

    const auto name = GetNativeFunctionName<T>();

    auto* func = Red::CGlobalFunction::Create<void*>(name.c_str(), name.c_str(), T);
    aCustomizer(func);
    rtti->RegisterFunction(func);
}
} // namespace App
