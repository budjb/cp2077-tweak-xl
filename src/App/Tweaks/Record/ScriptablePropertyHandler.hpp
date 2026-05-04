#pragma once

#include <type_traits>

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
    static constexpr auto ScriptablePropertyHandlerPrefix = "_ScriptablePropertyHandler";

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

    /**
     * @brief Creates and registers the native function for a scriptable record property that retrieves an array of
     * related records based on a foreign key relationship. This function implements the getter for properties that
     * represent arrays of foreign keys to other TweakDB record types.
     *
     * The native function should not be directly called as it requires an execution context to be placed on the call
     * stack via a wrapper script function. Native functions that return the same TweakDB record type will be reused for
     * all properties of that type.
     *
     * @param aClass The class type of the TweakDB record pointed to by the foreign key.
     * @return A pointer to the created native function that retrieves an array of related records based on a foreign
     * key relationship.
     */
    static Red::CGlobalFunction* CreateGetRecordsFunction(const Red::CClass* aClass)
    {
        static Red::CRTTISystem* rtti = RED4ext::CRTTISystem::Get();

        const auto* type = GetWHandleArrayType(aClass);
        const std::string name = GetFunctionName("GetRecords", std::nullopt, type->GetName());

        if (auto* func = GetFunction(name.c_str()))
            return func;

        auto* func = Red::CGlobalFunction::Create(name.c_str(), name.c_str(), &GetRecordArrayHandler);
        func->AddParam(type->GetName(), "outList", true, false);
        rtti->RegisterFunction(func);

        AddFunction(func->shortName, func);

        return func;
    }

    /**
     * @brief Creates and registers the native function for a scriptable record property that checks whether a given
     * weak handle to a TweakDB instance is contained in the array of related records based on a foreign key
     * relationship. This function is intended to be used as a helper for properties that represent arrays of foreign
     * keys to other TweakDB record types.
     *
     * The native function should not be directly called as it requires an execution context to be placed on the call
     * stack via a wrapper script function. Native functions that accept the same TweakDB record type will be reused for
     * all properties of that type.
     *
     * @param aClass The class type of the TweakDB record pointed to by the foreign key.
     * @return A pointer to the created native function that checks whether a given weak handle to a TweakDB instance is
     * contained in the array of related records based on a foreign key relationship.
     */
    static Red::CGlobalFunction* CreateRecordArrayContainsFunction(const Red::CClass* aClass)
    {
        static Red::CRTTISystem* rtti = RED4ext::CRTTISystem::Get();
        static constexpr auto boolName = Red::CName{Red::ERTDBFlatType::Bool};

        const auto* type = GetWHandleType(aClass);
        const std::string name = GetFunctionName("RecordArrayContains", boolName, type->GetName());

        if (auto* func = GetFunction(name.c_str()))
            return func;

        auto* func = Red::CGlobalFunction::Create(name.c_str(), name.c_str(), &RecordArrayContainsHandler);
        func->AddParam(type->GetName(), "item", false, false);
        func->SetReturnType(boolName);
        rtti->RegisterFunction(func);

        AddFunction(func->shortName, func);

        return func;
    }
    /**
     * @brief Creates and registers the native function for a scriptable record property that retrieves an individual
     * related record based on a foreign key relationship from an array. This function implements the getter for
     * properties that represent foreign keys to other TweakDB record types.
     *
     * The native function should not be directly called as it requires an execution context to be placed on the call
     * stack via a wrapper script function. Native functions that return the same TweakDB record type will be reused for
     * all properties of that type.
     *
     * @param aClass The class type of the TweakDB record pointed to by the foreign key.
     * @return A pointer to the created native function that retrieves an individual related record based on a foreign
     * key relationship from an array.
     */
    static Red::CGlobalFunction* CreateGetRecordItemFunction(const Red::CClass* aClass)
    {
        static Red::CRTTISystem* rtti = RED4ext::CRTTISystem::Get();
        static constexpr auto intName = Red::CName{Red::ERTDBFlatType::Int};

        const auto* type = GetWHandleType(aClass);
        const std::string name = GetFunctionName("GetRecordItem", type->GetName(), intName);

        if (auto* func = GetFunction(name.c_str()))
            return func;

        auto* func = Red::CGlobalFunction::Create(name.c_str(), name.c_str(), &GetRecordItemHandler);
        func->AddParam(intName, "item", false, false);
        func->SetReturnType(type->GetName());
        rtti->RegisterFunction(func);

        AddFunction(func->shortName, func);

        return func;
    }

    /**
     * @brief Creates and registers the native function for a scriptable record property that retrieves an individual
     * related record based on a foreign key relationship from an array. This function implements the getter for
     * properties that represent foreign keys to other TweakDB record types when a stronger reference is needed.
     *
     * The native function should not be directly called as it requires an execution context to be placed on the call
     * stack via a wrapper script function. Native functions that return the same TweakDB record type will be reused for
     * all properties of that type.
     *
     * @param aClass The class type of the TweakDB record pointed to by the foreign key.
     * @return A pointer to the created native function that retrieves an individual related record based on a foreign
     * key relationship from an array.
     */
    static Red::CGlobalFunction* CreateGetRecordItemHandleFunction(const Red::CClass* aClass)
    {
        static Red::CRTTISystem* rtti = RED4ext::CRTTISystem::Get();
        static constexpr auto intName = Red::CName{Red::ERTDBFlatType::Int};

        const auto* type = GetHandleType(aClass);
        const std::string name = GetFunctionName("GetRecordItemHandle", type->GetName(), intName);

        if (auto* func = GetFunction(name.c_str()))
            return func;

        auto* func = Red::CGlobalFunction::Create(name.c_str(), name.c_str(), &GetRecordHandleHandler);
        func->AddParam(intName, "item", false, false);
        func->SetReturnType(type->GetName());
        rtti->RegisterFunction(func);

        AddFunction(func->shortName, func);

        return func;
    }
    /**
     * @brief Creates and registers the native function for a scriptable record property that retrieves a related record
     * based on a foreign key relationship. This function implements the getter for properties that represent foreign
     * keys to other TweakDB record types.
     *
     * The native function should not be directly called as it requires an execution context to be placed on the call
     * stack via a wrapper script function. Native functions that return the same TweakDB record type will be reused for
     * all properties of that type.
     *
     * @param aClass The class type of the TweakDB record pointed to by the foreign key.
     * @return A pointer to the created native function that retrieves a related record based on a foreign key
     * relationship.
     */
    static Red::CGlobalFunction* CreateGetRecordFunction(const Red::CClass* aClass)
    {
        static Red::CRTTISystem* rtti = RED4ext::CRTTISystem::Get();

        const auto* type = GetWHandleType(aClass);
        const std::string name = GetFunctionName("GetRecord", type->GetName());

        if (auto* func = GetFunction(name.c_str()))
            return func;

        auto* func = Red::CGlobalFunction::Create(name.c_str(), name.c_str(), &GetRecordHandler);
        func->SetReturnType(type->GetName());
        rtti->RegisterFunction(func);

        AddFunction(func->shortName, func);

        return func;
    }

    /**
     * @brief Creates and registers the native function for a scriptable record property that retrieves a related record
     * based on a foreign key relationship. This function implements the getter for properties that represent foreign
     * keys to other TweakDB record types when a stronger reference is needed.
     *
     * The native function should not be directly called as it requires an execution context to be placed on the call
     * stack via a wrapper script function. Native functions that return the same TweakDB record type will be reused for
     * all properties of that type.
     *
     * @param aClass The class type of the TweakDB record pointed to by the foreign key.
     * @return A pointer to the created native function that retrieves a related record based on a foreign key
     * relationship.
     */
    static Red::CGlobalFunction* CreateGetRecordHandleFunction(const Red::CClass* aClass)
    {
        static Red::CRTTISystem* rtti = RED4ext::CRTTISystem::Get();

        const auto* type = GetHandleType(aClass);
        const std::string name = GetFunctionName("GetRecordHandle", type->GetName());

        if (auto* func = GetFunction(name.c_str()))
            return func;

        auto* func = Red::CGlobalFunction::Create(name.c_str(), name.c_str(), &GetRecordHandleHandler);
        func->SetReturnType(type->GetName());
        rtti->RegisterFunction(func);

        AddFunction(func->shortName, func);

        return func;
    }

    /**
     * @brief Creates and registers the native function for a scriptable record property that retrieves the property's
     * value from TweakDB. This function is suitable for retrieving most scriptable record property types, including
     * arrays, but is not intended for use with foreign keys to other TweakDB records.
     *
     * The native function should not be directly called as it requires an execution context to be placed on the call
     * stack via a wrapper script function. Native functions that return the same type will be reused for all properties
     * of that type.
     *
     * @param aType The hash of the property type to retrieve from TweakDB and return.
     * @return A pointer to the created native function that retrieves a property value from TweakDB.
     */
    static Red::CGlobalFunction* CreateGetFunction(const Red::CName aType)
    {
        static Red::CRTTISystem* rtti = RED4ext::CRTTISystem::Get();

        const std::string name = GetFunctionName("Get", aType);

        if (auto* func = GetFunction(name.c_str()))
            return func;

        auto* func = Red::CGlobalFunction::Create(name.c_str(), name.c_str(), &GetHandler);
        func->SetReturnType(aType);
        rtti->RegisterFunction(func);

        AddFunction(func->shortName, func);

        return func;
    }

    /**
     * @brief Creates and registers the native function for a scriptable record property that retrieves the number of
     * elements in an array property from TweakDB. This function is suitable for use with any type of array property.
     *
     * The native function should not be directly called as it requires an execution context to be placed on the call
     * stack via a wrapper script function. This native function will be reused for all array properties regardless of
     * their element type.
     *
     * @return A pointer to the created native function that retrieves the number of elements in an array property from
     * TweakDB.
     */
    static Red::CGlobalFunction* CreateGetArrayCountFunction()
    {
        static Red::CRTTISystem* rtti = RED4ext::CRTTISystem::Get();

        const std::string name = GetFunctionName("GetArrayCount", Red::ERTDBFlatType::Int);

        if (auto* func = GetFunction(name.c_str()))
            return func;

        auto* func = Red::CGlobalFunction::Create(name.c_str(), name.c_str(), &GetArrayCountHandler);
        func->SetReturnType(Red::ERTDBFlatType::Int);
        rtti->RegisterFunction(func);

        AddFunction(func->shortName, func);

        return func;
    }

    /**
     * @brief Creates and registers the native function for a scriptable record property that retrieves an individual
     * item from an array property based on its index from TweakDB. This function is suitable for use with any type of
     * array property other than arrays of foreign keys to other TweakDB records.
     *
     * The native function should not be directly called as it requires an execution context to be placed on the call
     * stack via a wrapper script function. Native functions that return the same type will be reused for all properties
     * of that type.
     *
     * @param aType The hash of the property type of the array item to retrieve from TweakDB and return.
     * @return A pointer to the created native function that retrieves an individual item from an array property based
     * on its index from TweakDB.
     */
    static Red::CGlobalFunction* CreateGetArrayItemFunction(const Red::CName aType)
    {
        static Red::CRTTISystem* rtti = RED4ext::CRTTISystem::Get();
        static constexpr auto intName = Red::CName{Red::ERTDBFlatType::Int};

        auto type = aType;

        if (Red::TweakDBUtil::IsArrayType(type))
            type = Red::TweakDBUtil::GetElementTypeName(type);

        const std::string name = GetFunctionName("GetArrayItem", type, intName);

        if (auto* func = GetFunction(name.c_str()))
            return func;

        auto* func = Red::CGlobalFunction::Create(name.c_str(), name.c_str(), &GetArrayItemHandler);
        func->AddParam(intName, "index", false, false);
        func->SetReturnType(type);
        rtti->RegisterFunction(func);

        AddFunction(func->shortName, func);

        return func;
    }

    /**
     * @brief Creates and registers the native function for a scriptable record property that checks whether a given
     * item is contained in an array property from TweakDB. This function is suitable for use with any type of array
     * property other than arrays of foreign keys to other TweakDB records.
     *
     * The native function should not be directly called as it requires an execution context to be placed on the call
     * stack via a wrapper script function. Native functions that accept the same type will be reused for all properties
     * of that type.
     *
     * @param aType The hash of the property type of the item to check for containment in the array property from
     * TweakDB.
     * @return A pointer to the created native function that checks whether a given item is contained in an array
     * property from TweakDB.
     */
    static Red::CGlobalFunction* CreateArrayContainsFunction(const Red::CName aType)
    {
        static Red::CRTTISystem* rtti = RED4ext::CRTTISystem::Get();
        static constexpr auto boolName = Red::CName{Red::ERTDBFlatType::Bool};

        auto type = aType;

        if (Red::TweakDBUtil::IsArrayType(type))
            type = Red::TweakDBUtil::GetElementTypeName(type);

        const std::string name = GetFunctionName("ArrayContains", boolName, type);

        if (auto* func = GetFunction(name.c_str()))
            return func;

        auto* func = Red::CGlobalFunction::Create(name.c_str(), name.c_str(), &ArrayContainsHandler);
        func->AddParam(type, "item", false, false);
        func->SetReturnType(boolName);
        rtti->RegisterFunction(func);

        AddFunction(func->shortName, func);

        return func;
    }

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

    static constexpr auto ArrayPrefix = Red::GetTypePrefixStr<Red::DynArray>();
    static constexpr auto WHandlePrefix = Red::GetTypePrefixStr<Red::WeakHandle>();
    static constexpr auto HandlePrefix = Red::GetTypePrefixStr<Red::Handle>();

    static Red::rtti::IType* GetHandleType(const Red::CClass* aClass)
    {
        std::string name = HandlePrefix.data();
        name.append(aClass->GetName().ToString());
        return Red::CRTTISystem::Get()->GetType(Red::CNamePool::Add(name.c_str()));
    }

    static Red::rtti::IType* GetWHandleType(const Red::CClass* aClass)
    {
        std::string name = WHandlePrefix.data();
        name.append(aClass->GetName().ToString());
        return Red::CRTTISystem::Get()->GetType(Red::CNamePool::Add(name.c_str()));
    }

    static Red::rtti::IType* GetWHandleArrayType(const Red::CClass* aClass)
    {
        std::string name = ArrayPrefix.data();
        name.append(WHandlePrefix.data());
        name.append(aClass->GetName().ToString());
        return Red::CRTTISystem::Get()->GetType(Red::CNamePool::Add(name.c_str()));
    }

    template<typename... Args,
             typename = std::enable_if_t<(std::is_same_v<std::remove_cvref_t<Args>, Red::CName> && ...)>>
    static std::string GetFunctionName(const std::string& aName, const std::optional<Red::CName>& aReturn, Args... args)
    {
        const std::string ret = aReturn.has_value() ? std::string{*aReturn->ToString()} : std::string{"Void"};

        std::string name = ScriptablePropertyHandlerPrefix;
        name.append(";");
        name.append(ret);
        name.append(";");
        name.append(aName);

        auto process = [&](const auto& arg) {
            name.append(";");
            name.append(arg.ToString());
        };

        (process(args), ...);

        return name;
    }

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

    /**
     * @brief Retrieves a registered function handler from the internal registry based on the provided function name.
     * The function name is expected to be in the format generated by the @c GetFunctionName function, which includes
     * the handler type, return type, and parameter types.
     *
     * @param aName The name of the function handler to retrieve, expected to be in the format generated by the @c
     * GetFunctionName function.
     * @return A pointer to the registered function handler corresponding to the provided name, or @c nullptr if no such
     * function handler is found in the registry.
     */
    static Red::CGlobalFunction* GetFunction(const Red::CName aName)
    {
        std::shared_lock lockR(s_mutex);

        if (const auto it = s_functions.find(aName); it != s_functions.end())
            return it->second;

        return nullptr;
    }

    /**
     * @brief Adds a function handler to the internal registry with the provided name. The function name is expected to
     * be in the format generated by the @c GetFunctionName function, which includes the handler type, return type, and
     * parameter types.
     *
     * @param aName The name of the function handler to add, expected to be in the format generated by the @c
     * GetFunctionName function.
     * @param aFunc A pointer to the function handler to add to the registry.
     */
    static void AddFunction(Red::CName aName, Red::CGlobalFunction* aFunc)
    {
        std::unique_lock lockW(s_mutex);
        s_functions.emplace(aName, aFunc);
    }

    /**
     * @brief A shared mutex used for synchronizing access to the internal registry of function handlers, allowing for
     * thread-safe retrieval and addition of function handlers.
     */
    static inline std::shared_mutex s_mutex;

    /**
     * @brief An internal registry mapping function names to their corresponding function handler pointers. The function
     * names are expected to be in the format generated by the @c GetFunctionName function, which includes the handler
     * type, return type, and parameter types. This registry allows for efficient reuse of function handlers based on
     * their signatures when handling property access for scriptable TweakDB records.
     */
    static inline Core::Map<Red::CName, Red::CGlobalFunction*> s_functions;
};
} // namespace App
