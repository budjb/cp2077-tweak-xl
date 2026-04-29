#pragma once

namespace App
{
/**
 * @brief Contains a specification for App::ScriptableTweakDBRecord property, including its TweakDB flat type and getter
 * closure return type details.
 */
struct TweakPropertySpec
{
    /**
     * @brief The RTTI type that should be returned from the property's getter closure. For foreign key types, this
     * value may initially be null if the referenced type is a scriptable record that has not yet been described.
     * Another attempt to load the type will be made when registering the property's getter closure with RTTI. If the
     * type can not be found at that time, the property will not be created.
     */
    const Red::CBaseRTTIType* propertyType{};

    /**
     * @brief The name of the RTTI type that should be returned from the property's getter closure, used for type
     * resolution when the property type is not yet available (e.g. for scriptable record foreign keys that have not yet
     * been described). The CName is guaranteed to be added to the CName pool.
     */
    Red::CName propertyTypeName{};

    /**
     * @brief The RTTI type of the TweakDB flat that corresponds to this property. As TweakDB flat types are well-known
     * and are built-in to the game, this is guaranteed to be set.
     */
    const Red::CBaseRTTIType* flatType{};

    /**
     * @brief The name of the RTTI type of the TweakDB flat that corresponds to this property.
     */
    Red::CName flatTypeName;

    /**
     * @brief Whether this property is an array type.
     */
    bool isArray{};

    /**
     * @brief Whether this property represents a foreign key reference to another TweakDB record. If true, the flat type
     * will be either a TweakDBID or array of TweakDBIDs, and the getter closure will return a handle or array of
     * handles to another TweakDB record type, respectively.
     */
    bool isForeignKey{};

    /**
     * @brief The name of the foreign type as specified by a YAML or Red Tweak file before conversion to a
     * fully-qualified TweakDB record name.
     */
    std::string foreignName;

    /**
     * @brief The RTTI type of the referenced record for foreign key properties. This may be null if the referenced type
     * is a scriptable record that has not yet been described. Another attempt to load the type will be made when
     * registering the property's getter closure with RTTI. If the type can not be found at that time, the property will
     * not be created.
     */
    const Red::CClass* foreignType{};

    /**
     * @brief The name of the referenced record for foreign key properties, used for type resolution when the referenced
     * type is not yet available (e.g. for scriptable record foreign keys that have not yet been described). The CName
     * is guaranteed to be added to the CName pool.
     */
    Red::CName foreignTypeName;
};
RED4EXT_ASSERT_SIZE(TweakPropertySpec, 0x60);

/**
 * @brief A shared pointer to a TweakPropertySpec struct.
 */
using TweakPropertySpecPtr = Core::SharedPtr<TweakPropertySpec>;

/**
 * @brief Parses the given string and infers the TweakDB property and flat details for a scriptable record. This is
 * useful for parsing types discovered from YAML record schemas and supports all valid TweakDB RTTI types, including
 * foreign keys and arrays of foreign keys to other TweakDB record types.
 *
 * See Red::ERTDBFlatType for a list of all supported TweakDB flat type names. Getters of foreign key types may be
 * represented as their full name or through shorthand, implicit handles. For example, a foreign key property
 * referencing "gamedataVehicle_Record" may be represented as any of the following:
 *
 * - "gamedataVehicle_Record" -> "whandle:gamedataVehicle_Record" (full name with implicit weak handle syntax)
 * - "Vehicle" -> "whandle:Vehicle" (short name with implicit weak handle syntax)
 * - "whandle:gamedataVehicle_Record" (full name with explicit weak handle syntax)
 * - "whandle:Vehicle" (short name with explicit weak handle syntax)
 * - "handle:gamedataVehicle_Record" (full name with strong handle syntax)
 * - "handle:Vehicle" (short name with strong handle syntax)
 * - "array:gamedataVehicle_Record" -> "array:whandle:gamedataVehicle_Record" (full name with implicit weak handle array
 * syntax)
 * - "array:Vehicle" -> "array:whandle:Vehicle" (short name with implicit weak handle array syntax)
 * - "array:whandle:gamedataVehicle_Record" (full name with explicit weak handle array syntax)
 * - "array:whandle:Vehicle" (short name with explicit weak handle array syntax)
 * - "array:handle:gamedataVehicle_Record" (full name with explicit strong handle array syntax)
 * - "array:handle:Vehicle" (short name with explicit strong handle array syntax)
 *
 * @param aValue The string to parse TweakDB property and flat details from.
 * @return A property spec containing the parsed property and flat details, or nullptr if the given string is not
 * a valid TweakDB property type.
 */
TweakPropertySpecPtr GetTweakPropertySpec(const std::string& aValue);

/**
 * @brief Creates a set of details for a TweakDB property based on the given type hash and optional foreign type name.
 *
 * @param aValue The original foreign type name before it has been converted to a fully-qualified TweakDB name.
 * @param aHash The hash of the property type, used for RTTI type resolution.
 * @param aForeignType An optional foreign type name for foreign key properties.
 * @return A property spec containing the parsed property and flat details, or nullptr if the given string is not
 * a valid TweakDB property type.
 */
TweakPropertySpecPtr GetTweakPropertySpec(const std::string& aValue, uint64_t aHash,
                                          const std::optional<std::string>& aForeignType = std::nullopt);
} // namespace App