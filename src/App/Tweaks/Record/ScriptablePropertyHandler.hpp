#pragma once

#include "App/Tweaks/TweakPropertySpec.hpp"
#include "Red/TweakDB/Manager.hpp"
#include "ScriptableRecordClass.hpp"
#include "ScriptableRecordManager.hpp"

namespace App
{
class ScriptablePropertyHandler
{
public:
    using RecordWHandleArray = Red::DynArray<Red::WeakHandle<Red::TweakDBRecord>>;
    using RecordWHandleArrayPtr = Red::InstancePtr<RecordWHandleArray>;
    using RecordWHandle = Red::WeakHandle<Red::TweakDBRecord>;
    using RecordHandle = Red::Handle<Red::TweakDBRecord>;

    static void CreateGetFKArray(ScriptableRecordClass* aClass, const std::string& aName,
                                 const Core::SharedPtr<ScriptableRecordManager::ScriptablePropertySpec>& aSpec);
    static void CreateGetArraySize(ScriptableRecordClass* aClass, const std::string& aName,
                                   const Core::SharedPtr<ScriptableRecordManager::ScriptablePropertySpec>& aSpec);
    static void CreateGetRecordWHandleAt(ScriptableRecordClass* aClass, const std::string& aName,
                                         const Core::SharedPtr<ScriptableRecordManager::ScriptablePropertySpec>& aSpec);
    static void CreateGetRecordHandleAt(ScriptableRecordClass* aClass, const std::string& aName,
                                        const Core::SharedPtr<ScriptableRecordManager::ScriptablePropertySpec>& aSpec);
    static void CreateRecordArrayContains(
        ScriptableRecordClass* aClass, const std::string& aName,
        const Core::SharedPtr<ScriptableRecordManager::ScriptablePropertySpec>& aSpec);
    static void CreateGetRecordWHandle(ScriptableRecordClass* aClass, const std::string& aName,
                                       const Core::SharedPtr<ScriptableRecordManager::ScriptablePropertySpec>& aSpec);
    static void CreateGetRecordHandle(ScriptableRecordClass* aClass, const std::string& aName,
                                      const Core::SharedPtr<ScriptableRecordManager::ScriptablePropertySpec>& aSpec);
    static void CreateGetResRefArray(ScriptableRecordClass* aClass, const std::string& aName,
                                     const Core::SharedPtr<ScriptableRecordManager::ScriptablePropertySpec>& aSpec);
    static void CreateGetResRefArraySize(ScriptableRecordClass* aClass, const std::string& aName,
                                         const Core::SharedPtr<ScriptableRecordManager::ScriptablePropertySpec>& aSpec);
    static void CreateGetResRefArrayItem(ScriptableRecordClass* aClass, const std::string& aName,
                                         const Core::SharedPtr<ScriptableRecordManager::ScriptablePropertySpec>& aSpec);
    static void CreateGetArray(ScriptableRecordClass* aClass, const std::string& aName,
                               const Core::SharedPtr<ScriptableRecordManager::ScriptablePropertySpec>& aSpec);
    static void CreateGetArrayItem(ScriptableRecordClass* aClass, const std::string& aName,
                                   const Core::SharedPtr<ScriptableRecordManager::ScriptablePropertySpec>& aSpec);
    static void CreateArrayContains(ScriptableRecordClass* aClass, const std::string& aName,
                                    const Core::SharedPtr<ScriptableRecordManager::ScriptablePropertySpec>& aSpec);
    static void CreateGet(ScriptableRecordClass* aClass, const std::string& aName,
                          const Core::SharedPtr<ScriptableRecordManager::ScriptablePropertySpec>& aSpec);

private:
    /**
     * @brief A specialized script function handler for retrieving an array of WeakHandles to TweakDBRecords from a
     * scriptable record property. The handler will attempt to retrieve the TweakDB flat value containing the array of
     * TweakDB IDs, retrieve the corresponding records for each ID in the array, verify that each record's type matches
     * the expected type, and return an array of WeakHandles to the corresponding TweakDBRecords. If the TweakDB flat is
     * not found, the value is not an array, or any of the types do not match, no value is returned.
     *
     * @param aInstance The scriptable instance on which the function was called. This should be a scriptable record
     * instance from which the property value can be retrieved based on the record ID and property specifications.
     * @param aFrame The stack frame for the function call, which can be used to access any parameters passed to the
     * function and to retrieve the function's execution context.
     * @param aOut A pointer to the memory location where the retrieved array of WeakHandles should be stored if the
     * retrieval is successful. The handler will write the value to this location if it is able to successfully retrieve
     * and type-check the value from TweakDB.
     * @param a4 The type hash of the return value. This is unused but required to satisfy the signature for a script
     * function handler.
     */
    static void HandleGetRecordArray(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut, int64_t a4);

    /**
     * @brief A generic script function handler for retrieving the count of items in an array property of a scriptable
     * record. The handler will attempt to retrieve the TweakDB flat value containing the array, determine the number of
     * items in the array, and return that count as an integer. If the TweakDB flat is not found or the value is not an
     * array, no value is returned.
     *
     * @param aInstance The scriptable instance on which the function was called. This should be a scriptable record
     * instance from which the property value can be retrieved based on the record ID and property specifications.
     * @param aFrame The stack frame for the function call, which can be used to access any parameters passed to the
     * function and to retrieve the function's execution context.
     * @param aOut A pointer to the memory location where the retrieved count should be stored if the retrieval is
     * successful. The handler will write the count to this location if it is able to successfully retrieve the array
     * from TweakDB and determine its length.
     * @param a4 The type hash of the return value. This is unused but required to satisfy the signature for a script
     * function handler.
     */
    static void HandleGetArrayCount(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut, int64_t a4);

    /**
     * @brief A specialized script function handler for retrieving a WeakHandle to an item at a specified index from an
     * array property of a scriptable record. The handler will attempt to retrieve the TweakDB flat value containing the
     * array of TweakDB IDs, retrieve the record identified by the TweakDB ID at the given index in the array, verify
     * that the record's type matches the expected type, and return a WeakHandle to the corresponding TweakDBRecord. If
     * the TweakDB flat is not found, the index is out of bounds, or the types do not match, no value is returned.
     *
     * @param aInstance The scriptable instance on which the function was called. This should be a scriptable record
     * instance from which the property value can be retrieved based on the record ID and property specifications.
     * @param aFrame The stack frame for the function call, which can be used to access any parameters passed to the
     * function and to retrieve the function's execution context.
     * @param aOut A pointer to the memory location where the retrieved WeakHandle should be stored if the retrieval is
     * successful. The handler will write the value to this location if it is able to successfully retrieve and
     * type-check the value from TweakDB.
     * @param a4 The type hash of the return value. This is unused but required to satisfy the signature for a script
     * function handler.
     */
    static void HandleGetRecordItem(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut, int64_t a4);

    /**
     * @brief A specialized script function handler for retrieving a Handle to an item at a specified index from an
     * array property of a scriptable record. The handler will attempt to retrieve the TweakDB flat value containing the
     * array of TweakDB IDs, retrieve the record identified by the TweakDB ID at the given index in the array, verify
     * that the record's type matches the expected type, and return a Handle to the corresponding TweakDBRecord. If the
     * TweakDB flat is not found, the index is out of bounds, or the types do not match, no value is returned.
     *
     * @param aInstance The scriptable instance on which the function was called. This should be a scriptable record
     * instance from which the property value can be retrieved based on the record ID and property specifications.
     * @param aFrame The stack frame for the function call, which can be used to access any parameters passed to the
     * function and to retrieve the function's execution context.
     * @param aOut A pointer to the memory location where the retrieved Handle should be stored if the retrieval is
     * successful. The handler will write the value to this location if it is able to successfully retrieve and
     * type-check the value from TweakDB.
     * @param a4 The type hash of the return value. This is unused but required to satisfy the signature for a script
     * function handler.
     */
    static void HandleGetRecordItemHandle(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                                          int64_t a4);

    /**
     * @brief A specialized script function handler for checking if the given record is contained within an array
     * property of a scriptable record by its TweakDB ID.
     *
     * @param aInstance The scriptable instance on which the function was called. This should be a scriptable record
     * instance from which the property value can be retrieved based on the record ID and property specifications.
     * @param aFrame The stack frame for the function call, which can be used to access any parameters passed to the
     * function and to retrieve the function's execution context.
     * @param aOut A pointer to the memory location where the result of the containment check should be stored. The
     * handler will write a boolean value to this location indicating whether the specified record is contained within
     * the array property.
     * @param a4 The type hash of the return value. This is unused but required to satisfy the signature for a script
     * function handler.
     */
    static void HandleRecordArrayContains(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut,
                                          int64_t a4);

    /**
     * @brief A specialized script function handler for retrieving a WeakHandle to a TweakDBRecord based on a TweakDB
     * ID. The handler will attempt to retrieve the appropriate TweakDB flat value containing the target record's ID,
     * retrieve the record instance, verify that the record's type matches the expected type, and return a WeakHandle to
     * the corresponding TweakDBRecord if the type check succeeds. If the TweakDB flat is not found or the types do not
     * match, no value is returned.
     *
     * @param aInstance The scriptable instance on which the function was called. This should be a scriptable record
     * instance from which the property value can be retrieved based on the record ID and property specifications.
     * @param aFrame The stack frame for the function call, which can be used to access any parameters passed to the
     * function and to retrieve the function's execution context.
     * @param aOut A pointer to the memory location where the retrieved WeakHandle should be stored if the retrieval is
     * successful. The handler will write the value to this location if it is able to successfully retrieve and
     * type-check the value from TweakDB.
     * @param a4 The type hash of the return value. This is unused but required to satisfy the signature for a script
     * function handler.
     */
    static void HandleGetRecord(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut, int64_t a4);

    /**
     * @brief A specialized script function handler for retrieving a Handle to a TweakDBRecord based on a TweakDB ID.
     * The handler will attempt to retrieve the appropriate TweakDB flat value containing the target record's ID,
     * retrieve the record instance, verify that the record's type matches the expected type, and return a Handle to the
     * corresponding TweakDBRecord if the type check succeeds. If the TweakDB flat is not found or the types do not
     * match, no value is returned.
     *
     * @param aInstance The scriptable instance on which the function was called. This should be a scriptable record
     * instance from which the property value can be retrieved based on the record ID and property specifications.
     * @param aFrame The stack frame for the function call, which can be used to access any parameters passed to the
     * function and to retrieve the function's execution context.
     * @param aOut A pointer to the memory location where the retrieved Handle should be stored if the retrieval is
     * successful. The handler will write the value to this location if it is able to successfully retrieve and
     * type-check the value from TweakDB.
     * @param a4 The type hash of the return value. This is unused but required to satisfy the signature for a script
     * function handler.
     */
    static void HandleGetRecordHandle(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut, int64_t a4);

    /**
     * @brief A generic script function handler for retrieving an item at a specified index from an array property of a
     * scriptable record. The handler will attempt to retrieve the appropriate TweakDB flat value for the record and
     * property, verify that the flat's type matches the expected array type, and return the item at the specified
     * index.
     *
     * @param aInstance The scriptable instance on which the function was called. This should be a scriptable record
     * instance from which the property value can be retrieved based on the record ID and property specifications.
     * @param aFrame The stack frame for the function call, which can be used to access any parameters passed to the
     * function and to retrieve the function's execution context.
     * @param aOut A pointer to the memory location where the retrieved item should be stored if the retrieval is
     * successful. The handler will write the value to this location if it is able to successfully retrieve and
     * type-check the value from TweakDB.
     * @param a4 The type hash of the return value. This is unused but required to satisfy the signature for a script
     * function handler.
     */
    static void HandleGetArrayItem(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut, int64_t a4);

    /**
     * @brief A generic script function handler for checking if a given item is contained within an array property of a
     * scriptable record. The handler will attempt to retrieve the appropriate TweakDB flat value for the record and
     * property, verify that the flat's type matches the expected array type, and check if the item is contained within
     * the array.
     *
     * @param aInstance The scriptable instance on which the function was called. This should be a scriptable record
     * instance from which the property value can be retrieved based on the record ID and property specifications.
     * @param aFrame The stack frame for the function call, which can be used to access any parameters passed to the
     * function and to retrieve the function's execution context.
     * @param aOut A pointer to the memory location where the result of the containment check should be stored.
     * @param a4 The type hash of the return value. This is unused but required to satisfy the signature for a script
     * function handler.
     */
    static void HandleArrayContains(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut, int64_t a4);

    /**
     * @brief A generic script function handler for retrieving values of scriptable record properties that do not
     * require any sort of special handling or transformation. The handler will attempt to retrieve the appropriate
     * TweakDB flat value, verify that the flat's type matches the property's expected type, and return the value if the
     * type check succeeds. If the TweakDB flat is not found or the types do not match, not value is returned.
     *
     * @param aInstance The scriptable instance on which the function was called. This should be a scriptable record
     * instance from which the property value can be retrieved based on the record ID and property specifications.
     * @param aFrame The stack frame for the function call, which can be used to access any parameters passed to the
     * function and to retrieve the function's execution context.
     * @param aOut A pointer to the memory location where the retrieved property value should be stored if the retrieval
     * is successful. The handler will write the value to this location if it is able to successfully retrieve and
     * type-check the value from TweakDB.
     * @param a4 The type hash of the return value. This is unused but required to satisfy the signature for a script
     * function handler.
     */
    static void HandleGet(Red::IScriptable* aInstance, Red::CStackFrame* aFrame, void* aOut, int64_t a4);

    /**
     * @brief Gets an InstancePtr to a DynArray of WeakHandles to TweakDBRecords for the array of records identified by
     * the array of TweakDB IDs within the given value.
     *
     * @param aValue The value containing the array of TweakDB IDs that identify the records. This should be of the same
     * type as the property defined in the property specifications, which is expected to be an array type with a foreign
     * key relationship to TweakDB records.
     * @param aSpec The property specifications that define the expected type of the array and its foreign key
     * relationship. This is used to validate the input and determine how to access the array elements.
     * @return An InstancePtr to a DynArray of WeakHandles to TweakDBRecords for the records identified by the TweakDB
     * IDs in the array if the input is valid and the records can be retrieved, or nullptr if the input is invalid
     * (e.g., if the value does not contain a valid array of TweakDB IDs or if the records cannot be retrieved from the
     * TweakDBManager).
     */
    static RecordWHandleArrayPtr GetRecordArray(const Red::Value<>& aValue, const TweakPropertySpecPtr& aSpec);

    /**
     * @brief Gets a WeakHandle<TweakDBRecord> for the record at the specified index in the array of records identified
     * by the array of TweakDB IDs within the given value.
     *
     * @param aValue The value containing the array of TweakDB IDs that identify the records.
     * @param aSpec The property specifications that define the expected type of the array and its foreign key
     * relationship. This is used to validate the input and determine how to access the array elements.
     * @param aIndex The index of the record within the array for which to get the weak handle.
     * @return A WeakHandle<TweakDBRecord> for the record at the specified index if the input is valid and the record
     * can be retrieved, or an empty weak handle if the input is invalid (e.g., if the value does not contain a valid
     * array of TweakDB IDs, if the index is out of bounds, or if the record cannot be retrieved from the
     * TweakDBManager).
     */
    static RecordWHandle GetRecordItem(const Red::Value<>& aValue, const TweakPropertySpecPtr& aSpec, int aIndex);

    /**
     * @brief Gets a Handle<TweakDBRecord> for the record at the specified index in the array of records identified by
     * the array of TweakDB IDs within the given value.
     *
     * @param aValue The value containing the array of TweakDB IDs that identify the records.
     * @param aSpec The property specifications that define the expected type of the array and its foreign key
     * relationship. This is used to validate the input and determine how to access the array elements.
     * @param aIndex The index of the record within the array for which to get the handle.
     * @return A Handle<TweakDBRecord> for the record at the specified index if the input is valid and the record can be
     * retrieved, or an empty handle if the input is invalid (e.g., if the value does not contain a valid array of
     * TweakDB IDs, if the index is out of bounds, or if the record cannot be retrieved from the TweakDBManager).
     */
    static RecordHandle GetRecordItemHandle(const Red::Value<>& aValue, const TweakPropertySpecPtr& aSpec, int aIndex);

    /**
     * @brief Checks if the given record is present within the array of records identified by the array of TweakDB IDs
     * within the given value.
     *
     * @param aValue The value containing the array of TweakDB IDs that identify the records to check against.
     * @param aSpec The property specifications that define the expected type of the array and its foreign key
     * relationship. This is used to validate the input and determine how to access the array elements.
     * @param aRecord The record to check for presence within the array. This should be of the same type as the records
     * identified by the TweakDB IDs in the array defined in the property specifications.
     * @return true if the record is found within the array of records identified by the TweakDB IDs, false otherwise.
     */
    static bool RecordArrayContains(const Red::Value<>& aValue, const TweakPropertySpecPtr& aSpec,
                                    const RecordWHandle& aRecord);

    /**
     * @brief Gets a WeakHandle<TweakDBRecord> for the record identified by the TweakDB ID contained in the given value.
     *
     * @param aValue The value containing the TweakDB ID of the record for which to get the weak handle.
     * @param aSpec The property specifications that define the expected type of the record and its foreign key
     * relationship.
     * @return A WeakHandle<TweakDBRecord> for the record if the input is valid and the record can be retrieved, or an
     * empty weak handle if the input is invalid (e.g., if the value does not contain a valid TweakDB ID or if the
     * record cannot be retrieved from the TweakDBManager).
     */
    static RecordWHandle GetRecord(const Red::Value<>& aValue, const TweakPropertySpecPtr& aSpec);

    /**
     * @brief Gets a Handle<TweakDBRecord> for the record identified by the TweakDB ID contained in the given value.
     *
     * @param aValue The value containing the TweakDB ID of the record for which to get the handle.
     * @param aSpec The property specifications that define the expected type of the record and its foreign key
     * relationship.
     * @return A Handle<TweakDBRecord> for the record if the input is valid and the record can be retrieved, or an empty
     * handle if the input is invalid (e.g., if the value does not contain a valid TweakDB ID or if the record cannot be
     * retrieved from the TweakDBManager).
     */
    static RecordHandle GetRecordHandle(const Red::Value<>& aValue, const TweakPropertySpecPtr& aSpec);

    /**
     * @brief Gets the size of the array contained in the given value based on the provided property specifications.
     *
     * @param aValue The value containing the array for which to get the size. This should be a DynArray of the
     * appropriate type as defined in the property specifications.
     * @param aSpec The property specifications that define the expected type of the array and its elements. This is
     * used to validate the input and determine how to access the array elements.
     * @return The size of the array if the input is valid, or 0 if the input is invalid (e.g., if the value does not
     * contain an array of the expected type).
     */
    static uint32_t GetArrayCount(const Red::Value<>& aValue, const TweakPropertySpecPtr& aSpec);

    /**
     * @brief Checks if the specified item is contained within the array contained in the given value.
     *
     * @param aValue The value containing the array to check. This should be a DynArray of the appropriate type as
     * defined in the property specifications.
     * @param aSpec The property specifications that define the expected type of the array and its elements. This is
     * used to validate the input and determine how to access the array elements.
     * @param item The item to check for containment within the array. This should be of the same type as the elements
     * of the array defined in the property specifications.
     * @return true if the item is found within the array, false otherwise.
     */
    static bool ArrayContains(const Red::Value<>& aValue, const TweakPropertySpecPtr& aSpec, Red::Instance item);

    /**
     * @brief Assigns a TweakDB record to the specified instance based on the provided record ID, record type, and
     * foreign key specifications.
     *
     * This function retrieves the record from the TweakDBManager using the record ID and checks if it matches the
     * expected record type defined in the property specifications. If the record is found and is of the correct type,
     * it assigns the record instance to the provided instance pointer.
     *
     * @param aInstance The instance pointer to which the record should be assigned. This should point to a
     * Handle<TweakDBRecord> or WeakHandle<TweakDBRecord> depending on the context.
     * @param aId The TweakDBID of the record to retrieve and assign.
     * @param aType The RTTI type of the property for which the record is being assigned. This is used to determine
     * whether a strong or weak handle is required.
     * @param aRecordType The RTTI class of the expected record type as defined in the property specifications. This is
     * used to validate that the retrieved record is of the correct type.
     */
    static void AssignRecord(Red::Instance aInstance, Red::TweakDBID aId, const Red::CBaseRTTIType* aType,
                             const Red::CClass* aRecordType);

    /**
     * @brief Gets the RTTI type for a Handle<TweakDBRecord> of the specified record class.
     *
     * @param aClass The record class for which to get the Handle type.
     * @return The RTTI type for Handle<TweakDBRecord> of the specified record class, or nullptr if the type cannot be
     * found.
     */
    static Red::CBaseRTTIType* GetHandleType(const Red::CClass* aClass);

    /**
     * @brief Gets the RTTI type for a WeakHandle<TweakDBRecord> of the specified record class.
     *
     * @param aClass The record class for which to get the WeakHandle type.
     * @return The RTTI type for WeakHandle<TweakDBRecord> of the specified record class, or nullptr if the type cannot
     * be found.
     */
    static Red::CBaseRTTIType* GetWHandleType(const Red::CClass* aClass);

    /**
     * @brief Gets the RTTI type for a DynArray<WeakHandle<TweakDBRecord>> of the specified record class.
     *
     * @param aClass The record class for which to get the WeakHandle array type.
     * @return The RTTI type for DynArray<WeakHandle<TweakDBRecord>> of the specified record class, or nullptr if the
     * type cannot be found.
     */
    static Red::CBaseRTTIType* GetWHandleArrayType(const Red::CClass* aClass);

    /**
     * @brief Sets the TweakDBManager instance to be used by the property handlers for retrieving records based on their
     * IDs.
     *
     * @param aManager The TweakDBManager instance to set. This should be called before any property handlers are
     * invoked to ensure that they can retrieve records correctly.
     */
    static void SetTweakDBManager(const Core::DeferredPtr<Red::TweakDBManager>& aManager);

    /**
     * @brief The TweakDBManager instance used by the property handlers to retrieve records based on their IDs.
     */
    inline static Core::DeferredPtr<Red::TweakDBManager> s_manager;
};
} // namespace App