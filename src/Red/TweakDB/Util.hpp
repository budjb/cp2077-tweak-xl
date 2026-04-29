#pragma once

namespace Red::TweakDBUtil
{
/**
 * @brief Information about a TweakDB record property, including its TweakDB flat type and getter closure return type
 * details. This is useful for scriptable record types.
 *
 * @todo move this to App namespace
 */
struct PropertyFlatInfo
{
    /**
     * @brief The RTTI type that should be returned from the property's getter closure. For foreign key types, this
     * value may initially be null if the referenced type is a scriptable record that has not yet been described.
     */
    const CBaseRTTIType* propertyType{};

    /**
     * @brief The name of the RTTI type that should be returned from the property's getter closure, used for type
     * resolution when the property type is not yet available (e.g. for scriptable record foreign keys that have not yet
     * been described). The CName is guaranteed to be added to the CName pool.
     */
    CName propertyTypeName{};

    /**
     * @brief The RTTI type of the TweakDB flat that corresponds to this property.
     */
    const CBaseRTTIType* flatType{};

    /**
     * @brief The name of the RTTI type of the TweakDB flat that corresponds to this property.
     */
    CName flatTypeName;

    /**
     * @brief Whether this property is an array type.
     */
    bool isArray{};

    /**
     * @brief Whether this property represents a foreign key reference to another TweakDB record. If true, flat type
     * will be either a TweakDBID or array of TweakDBIDs, and the getter closure will return a handle or array of
     * handles to another TweakDB record type, respectively.
     */
    bool isForeignKey{};

    /**
     * @brief The name of the foreign type as specified by a YAML or Red Tweak file before normalization to a
     * fully-qualified TweakDB record name.
     */
    std::string foreignName;

    /**
     * @brief The RTTI type of the referenced record for foreign key properties. This may be null if the referenced type
     * is a scriptable record that has not yet been described, in which case foreignTypeName can be used for type
     * resolution once the referenced type is described.
     */
    const CClass* foreignType{};

    /**
     * @brief The name of the referenced record for foreign key properties, used for type resolution when the referenced
     * type is not yet available (e.g. for scriptable record foreign keys that have not yet been described). The CName
     * is guaranteed to be added to the CName pool.
     */
    std::optional<CName> foreignTypeName{};
};
RED4EXT_ASSERT_SIZE(PropertyFlatInfo, 0x68);

/**
 * @brief A shared pointer to a PropertyFlatInfo struct, used for caching property type information for scriptable
 * record properties.
 *
 * @todo move to App namespace.
 */
using PropertyFlatInfoPtr = Core::SharedPtr<PropertyFlatInfo>;

/**
 * @brief Parses the given string to infer the TweakDB property and flat details for a scriptable record. This is useful
 * for parsing types discovered from YAML record schemas and supports all fully-qualified RTTI type names that are valid
 * for use with TweakDB properties.
 *
 * @param aValue The string to parse TweakDB property and flat details from.
 * @return A PropertyFlatInfoPtr containing the parsed property and flat details, or nullptr if the given string is not
 * a valid TweakDB property type.
 */
PropertyFlatInfoPtr GetPropertyFlatInfo(const std::string& aValue);

/**
 * @brief Creates a set of details for a TweakDB property based on the given type hash and optional foreign type name.
 *
 * @param aValue The original foreign type name before it has been normalized to a fully-qualified TweakDB name.
 * @param aHash The hash of the property type, used for RTTI type resolution.
 * @param aForeignType An optional foreign type name for foreign key properties.
 * @return A PropertyFlatInfoPtr containing the parsed property and flat details, or nullptr if the given string is not
 * a valid TweakDB property type.
 */
PropertyFlatInfoPtr GetPropertyFlatInfo(const std::string& aValue, uint64_t aHash,
                                        const std::optional<std::string>& aForeignType = std::nullopt);

/**
 * @brief Gets the RTTI type of a TweakDB flat type by its type hash.
 *
 * @param aType The hash of the flat type to get.
 * @return The RTTI type corresponding to the given flat type hash, or nullptr if the type is not a valid TweakDB flat
 * type.
 */
CBaseRTTIType* GetFlatType(uint64_t aType);

/**
 * @brief Gets the RTTI type of a TweakDB flat type by its name.
 *
 * @param aTypeName The name of the flat type to get.
 * @return The RTTI type corresponding to the given flat type name, or nullptr if the type is not a valid TweakDB flat
 * type.
 */
CBaseRTTIType* GetFlatType(CName aTypeName);

/**
 * @brief Gets the RTTI type of a TweakDB array type corresponding to the given element type.
 *
 * @param aTypeName The name of the element type to get the corresponding array type for.
 * @return The RTTI type of the TweakDB array type corresponding to the given element type, or nullptr if the given type
 * is not a valid TweakDB element type.
 */
CBaseRTTIType* GetArrayType(CName aTypeName);

/**
 * @brief Gets the RTTI type of a TweakDB array type corresponding to the given element type.
 *
 * @param aType The RTTI type of the element type to get the corresponding array type for.
 * @return The RTTI type of the TweakDB array type corresponding to the given element type, or nullptr if the given type
 * is not a valid TweakDB element type.
 */
CBaseRTTIType* GetArrayType(const CBaseRTTIType* aType);

/**
 * @brief Gets the RTTI type of a TweakDB element type corresponding to the given array type.
 *
 * @param aTypeName The name of the array type to get the corresponding element type for.
 * @return The RTTI type of the TweakDB element type corresponding to the given array type, or nullptr if the given type
 * is not a valid TweakDB array type.
 */
CBaseRTTIType* GetElementType(CName aTypeName);

/**
 * @brief Gets the RTTI type of a TweakDB element type corresponding to the given array type.
 *
 * @param aType The RTTI type of the array type to get the corresponding element type for.
 * @return The RTTI type of the TweakDB element type corresponding to the given array type, or nullptr if the given type
 * is not a valid TweakDB array type.
 */
CBaseRTTIType* GetElementType(const CBaseRTTIType* aType);

/**
 * @brief Checks whether the given type name corresponds to a valid TweakDB flat type.
 *
 * @param aTypeName The name of the type to check.
 * @return true if the given type name corresponds to a valid TweakDB flat type, false otherwise.
 */
bool IsFlatType(CName aTypeName);

/**
 * @brief Checks whether the given RTTI type corresponds to a valid TweakDB flat type.
 *
 * @param aType The RTTI type to check.
 * @return true if the given RTTI type corresponds to a valid TweakDB flat type, false otherwise.
 */
bool IsFlatType(const CBaseRTTIType* aType);

/**
 * @brief Checks whether the given type name corresponds to a valid TweakDB array type.
 *
 * @param aTypeName The name of the type to check.
 * @return true if the given type name corresponds to a valid TweakDB array type, false otherwise.
 */
bool IsArrayType(CName aTypeName);

/**
 * @brief Checks whether the given RTTI type corresponds to a valid TweakDB array type.
 *
 * @param aType The RTTI type to check.
 * @return true if the given RTTI type corresponds to a valid TweakDB array type, false otherwise.
 */
bool IsArrayType(const CBaseRTTIType* aType);

/**
 * @brief Checks whether the given type name corresponds to a non-array TweakDB foreign key type.
 *
 * @param aTypeName The name of the type to check.
 * @return true if the given type name corresponds to a non-array TweakDB foreign key type, false otherwise.
 */
bool IsForeignKey(CName aTypeName);

/**
 * @brief Checks whether the given RTTI type corresponds to a non-array TweakDB foreign key type.
 *
 * @param aType The RTTI type to check.
 * @return true if the given RTTI type corresponds to a non-array TweakDB foreign key type, false otherwise.
 */
bool IsForeignKey(const CBaseRTTIType* aType);

/**
 * @brief Checks whether the given type name corresponds to an array TweakDB foreign key type.
 *
 * @param aTypeName The name of the type to check.
 * @return true if the given type name corresponds to an array TweakDB foreign key type, false otherwise.
 */
bool IsForeignKeyArray(CName aTypeName);

/**
 * @brief Checks whether the given RTTI type corresponds to an array TweakDB foreign key type.
 *
 * @param aType The RTTI type to check.
 * @return true if the given RTTI type corresponds to an array TweakDB foreign key type, false otherwise.
 */
bool IsForeignKeyArray(const CBaseRTTIType* aType);

/**
 * @brief Checks whether the given type name corresponds to a TweakDB resource reference token type (either "ResRef" or
 * "ScriptResRef").
 *
 * @param aTypeName The name of the type to check.
 * @return true if the given type name corresponds to a TweakDB resource reference token type, false otherwise.
 */
bool IsResRefToken(CName aTypeName);

/**
 * @brief Checks whether the given RTTI type corresponds to a TweakDB resource reference token type (either "ResRef" or
 * "ScriptResRef").
 *
 * @param aType The RTTI type to check.
 * @return true if the given RTTI type corresponds to a TweakDB resource reference token type, false otherwise.
 */
bool IsResRefToken(const CBaseRTTIType* aType);

/**
 * @brief Checks whether the given type name corresponds to a TweakDB resource reference token array type (either
 * "array:ResRef" or "array:ScriptResRef").
 *
 * @param aTypeName The name of the type to check.
 * @return true if the given type name corresponds to a TweakDB resource reference token array type, false otherwise.
 */
bool IsResRefTokenArray(CName aTypeName);

/**
 * @brief Checks whether the given RTTI type corresponds to a TweakDB resource reference token array type (either
 * "array:ResRef" or "array:ScriptResRef").
 *
 * @param aType The RTTI type to check.
 * @return true if the given RTTI type corresponds to a TweakDB resource reference token array type, false otherwise.
 */
bool IsResRefTokenArray(const CBaseRTTIType* aType);

/**
 * @brief Gets the name of the TweakDB array type corresponding to the given element type name.
 *
 * @param aTypeName The name of the element type to get the corresponding array type name for.
 * @return The name of the TweakDB array type corresponding to the given element type name, or an empty CName if the
 * given type name is not a valid TweakDB element type.
 */
CName GetArrayTypeName(CName aTypeName);

/**
 * @brief Gets the name of the TweakDB array type corresponding to the given element RTTI type.
 *
 * @param aType The RTTI type of the element type to get the corresponding array type name for.
 * @return The name of the TweakDB array type corresponding to the given element RTTI type, or an empty CName if the
 * given type is not a valid TweakDB element type.
 */
CName GetArrayTypeName(const CBaseRTTIType* aType);

/**
 * @brief Gets the name of the TweakDB element type corresponding to the given array type name.
 *
 * @param aTypeName The name of the array type to get the corresponding element type name for.
 * @return The name of the TweakDB element type corresponding to the given array type name, or an empty CName if the
 * given type name is not a valid TweakDB array type.
 */
CName GetElementTypeName(CName aTypeName);

/**
 * @brief Gets the name of the TweakDB element type corresponding to the given array RTTI type.
 *
 * @param aType The RTTI type of the array type to get the corresponding element type name for.
 * @return The name of the TweakDB element type corresponding to the given array RTTI type, or an empty CName if the
 * given type is not a valid TweakDB array type.
 */
CName GetElementTypeName(const CBaseRTTIType* aType);

/**
 * @brief Constructs an instance of the given TweakDB flat type by its name with its default (empty) value.
 *
 * @param aTypeName The name of the TweakDB flat type to construct an instance of.
 * @return An instance of the given TweakDB flat type, or nullptr if the given type name is not a valid TweakDB flat
 * type.
 */
InstancePtr<> Construct(CName aTypeName);

/**
 * @brief Constructs an instance of the given TweakDB flat type with its default (empty) value.
 *
 * @param aType The RTTI type of the TweakDB flat type to construct an instance of.
 * @return An instance of the given TweakDB flat type, or nullptr if the given RTTI type is not a valid TweakDB flat
 * type.
 */
InstancePtr<> Construct(const CBaseRTTIType* aType);

/**
 * @brief Constructs a value of the given TweakDB flat type by its name with its default (empty) value.
 *
 * @param aType The RTTI type of the TweakDB flat type to construct a value of.
 * @return A value of the given TweakDB flat type, or nullptr if the given RTTI type is not a valid TweakDB flat type.
 */
ValuePtr<> ConstructValue(const CBaseRTTIType* aType);

/**
 * @brief Constructs a value of the given TweakDB flat type by its name with its default (empty) value.
 *
 * @param aTypeName The name of the TweakDB flat type to construct a value of.
 * @return A value of the given TweakDB flat type, or nullptr if the given type name is not a valid TweakDB flat type.
 */
ValuePtr<> ConstructValue(CName aTypeName);

/**
 * @brief Gets the RTTI class type corresponding to the given TweakDB record type name.
 *
 * @param aTypeName The name of the TweakDB record type to get the RTTI class type for.
 * @return The RTTI class type corresponding to the given TweakDB record type name, or nullptr if the given type name
 * does not correspond to a valid TweakDB record type.
 */
CClass* GetRecordType(CName aTypeName);

/**
 * @brief Gets the RTTI class type corresponding to the given TweakDB record type name.
 *
 * @param aTypeName The name of the TweakDB record type to get the RTTI class type for.
 * @return The RTTI class type corresponding to the given TweakDB record type name, or nullptr if the given type name
 * does not correspond to a valid TweakDB record type.
 */
CClass* GetRecordType(const char* aTypeName);

/**
 * @brief Checks whether the given type name corresponds to a valid TweakDB record type, which is determined by whether
 * the type is a descendent of Red::TweakDBRecord.
 *
 * @param aTypeName The name of the type to check.
 * @return true if the given type name corresponds to a valid TweakDB record type, false otherwise.
 */
bool IsRecordType(CName aTypeName);

/**
 * @brief Checks whether the given RTTI type corresponds to a valid TweakDB record type, which is determined by whether
 * the type is a descendent of Red::TweakDBRecord.
 *
 * @param aType The RTTI type to check.
 * @return true if the given RTTI type corresponds to a valid TweakDB record type, false otherwise.
 */
bool IsRecordType(const CClass* aType);

/**
 * @brief Get the class name for a handle of a given record name (e.g. "Vehicle" -> "handle:gamedataVehicle_Record").
 *
 * Note that the provided name will be normalized to the fully-qualified TweakDB record name for proper type resolution.
 *
 * @param aName The name of the record type to get the class handle name for.
 * @return The class handle name corresponding to the given record name, or @c Red::CName::Empty if the provided name is
 * empty or invalid.
 */
std::string GetClassHandleName(const std::string& aName);

/**
 * @brief Get the class name for an array of handles of a given record name (e.g. "Vehicle" ->
 * "array:handle:gamedataVehicle_Record").
 *
 * Note that the provided name will be normalized to the fully-qualified TweakDB record name for proper type resolution.
 *
 * @param aName The name of the record type to get the class handle name for.
 * @return The class handle name corresponding to the given record name, or @c Red::CName::Empty if the provided name is
 * empty or invalid.
 */
std::string GetClassHandleArrayName(const std::string& aName);

/**
 * @brief Normalizes a TweakDB record name to conform to typical TweakDB naming standards. The name is processed as
 * follows:
 *
 * - The short name is extracted from the provided name (e.g. "foo_Record" -> "foo").
 * - The short name is capitalized (e.g. "foo" -> "Foo").
 * - The capitalized short name is converted to a fully-qualified record name by prepending "gamedata" and appending
 * "_Record" to it.
 *
 * @param aName The name of the record to normalize.
 * @return The normalized record name corresponding to the given name, or an empty string if the provided name is empty
 * or invalid.
 */
std::string NormalizeRecordName(const std::string& aName);

/**
 * @brief Gets the fully-qualified TweakDB record name corresponding to the given name. Fully-qualified TweakDB names
 * follow the convention "gamedata<Name>_Record", where "<Name>" is the capitalized short name of the record type.
 *
 * @tparam T The type to return the record name as, either std::string or CName.
 * @param aName The name of the record to get the fully-qualified name for.
 * @return The fully-qualified record name corresponding to the given name.
 */
template<typename T>
T GetRecordFullName(const std::string& aName);

/**
 * @brief Gets the fully-qualified TweakDB record name corresponding to the given name. Fully-qualified TweakDB names
 * follow the convention "gamedata<Name>_Record", where "<Name>" is the capitalized short name of the record type.
 *
 * @tparam T The type to return the record name as, either std::string or CName.
 * @param aName The name of the record to get the fully-qualified name for.
 * @return The fully-qualified record name corresponding to the given name.
 */
template<typename T>
T GetRecordFullName(const char* aName);

/**
 * @brief Gets the fully-qualified TweakDB record name corresponding to the given name. Fully-qualified TweakDB names
 * follow the convention "gamedata<Name>_Record", where "<Name>" is the capitalized short name of the record type.
 *
 * @tparam T The type to return the record name as, either std::string or CName.
 * @param aName The name of the record to get the fully-qualified name for.
 * @return The fully-qualified record name corresponding to the given name.
 */
template<typename T>
T GetRecordFullName(CName aName);

template<typename T>
T GetRecordAliasName(const std::string& aName);

template<typename T>
T GetRecordAliasName(const char* aName);

template<typename T>
T GetRecordAliasName(CName aName);

template<>
[[nodiscard]] std::string GetRecordFullName(const std::string& aName);

template<>
std::string GetRecordFullName(const char* aName);

template<>
[[nodiscard]] std::string GetRecordFullName(CName aName);

template<>
[[nodiscard]] std::string GetRecordAliasName(const std::string& aName);

template<>
[[nodiscard]] std::string GetRecordAliasName(const char* aName);

template<>
[[nodiscard]] std::string GetRecordAliasName(CName aName);

template<>
[[nodiscard]] CName GetRecordFullName(const std::string& aName);

template<>
[[nodiscard]] CName GetRecordFullName(const char* aName);

template<>
[[nodiscard]] CName GetRecordFullName(CName aName);

template<>
[[nodiscard]] CName GetRecordAliasName(const std::string& aName);

template<>
[[nodiscard]] CName GetRecordAliasName(const char* aName);

template<>
[[nodiscard]] CName GetRecordAliasName(CName aName);

template<typename T>
T GetRecordShortName(const std::string& aName);

template<typename T>
T GetRecordShortName(CName aName);

template<typename T>
T GetRecordShortName(const char* aName);

template<>
std::string GetRecordShortName(const std::string& aName);

template<>
std::string GetRecordShortName(CName aName);

template<>
std::string GetRecordShortName(const char* aName);

template<>
CName GetRecordShortName(const std::string& aName);

template<>
CName GetRecordShortName(CName aName);

template<>
CName GetRecordShortName(const char* aName);

std::string GetPropertyFunctionName(CName aName);

uint32_t GetRecordTypeHash(CName aName);
uint32_t GetRecordTypeHash(const std::string& aName);
uint32_t GetRecordTypeHash(const char* aName);
uint32_t GetRecordTypeHash(const CClass* aType);

TweakDBID GetRTDBFlatID(CName aRecord, CName aProp);
TweakDBID GetRTDBFlatID(CName aRecord, const std::string& aProp);
TweakDBID GetRTDBFlatID(CName aRecord, const char* aProp);
TweakDBID GetRTDBRecordID(CName aRecord);

std::string Capitalize(CName aName);
std::string Capitalize(const std::string& aName);
std::string Capitalize(const char* aName);
std::string Decapitalize(CName aName);
std::string Decapitalize(const std::string& aName);
std::string Decapitalize(const char* aName);

} // namespace Red::TweakDBUtil
