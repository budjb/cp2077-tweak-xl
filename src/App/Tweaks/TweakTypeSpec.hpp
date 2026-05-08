#pragma once

namespace App
{
/**
 * @brief Contains a specification for App::ScriptableTweakDBRecord property, including its TweakDB flat type and getter
 * closure return type details.
 */
struct TweakTypeSpec
{
    /**
     * @brief The RTTI type that should be returned from the property's getter closure. For foreign key types, this
     * value may initially be null if the referenced type is a scriptable record that has not yet been described.
     * Another attempt to load the type will be made when registering the property's getter closure with RTTI. If the
     * type can not be found at that time, the property will not be created.
     */
    const Red::rtti::IType* propertyType{};

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
    const Red::rtti::IType* flatType{};

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
     * @brief Whether this property represents a ResRef or an array of ResRefs. If true, the flat type will be either a
     * ResRef or an array of ResRefs, and the getter closure will return a Red::ResRef or an array of Red::ResRefs,
     * respectively.
     */
    bool isResRef{};

    /**
     * @brief Whether this property represents a localization key or an array of localization keys. If true, the flat
     * type will be either a LocKey or an array of LocKeys, and the getter closure will return a Red::CName or an array
     * of Red::CNames, respectively.
     */
    bool isLocKey{};

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

/**
 * @brief A shared pointer to a TweakTypeSpec struct.
 */
using TweakTypeSpecPtr = Core::SharedPtr<TweakTypeSpec>;

/**
 * @brief Parses the given string and infers the TweakDB property and flat details for a scriptable record. This is
 * useful for parsing types discovered from YAML record schemas and supports all valid TweakDB RTTI types, including
 * foreign keys and arrays of foreign keys to other TweakDB record types.
 *
 * See Red::ERTDBFlatType for a list of all supported TweakDB flat type names. Getters of foreign key types may be
 * represented as the foreign record's class name (full, script alias, or short) for properties pointing to a single
 * foreign key or prefixed with "array:" for an array of foreign keys.
 *
 * @param aValue The string to parse TweakDB property and flat details from.
 * @return A property spec containing the parsed property and flat details, or nullptr if the given string is not
 * a valid TweakDB property type.
 */
TweakTypeSpecPtr GetTweakTypeSpec(const std::string& aValue);

/**
 * @brief Parses the given string and infers the TweakDB property and flat details for a scriptable record. This is
 * useful for parsing types discovered from YAML record schemas and supports all valid TweakDB RTTI types, including
 * foreign keys and arrays of foreign keys to other TweakDB record types.
 *
 * See Red::ERTDBFlatType for a list of all supported TweakDB flat type names. Getters of foreign key types may be
 * represented as the foreign record's class name (full, script alias, or short) for properties pointing to a single
 * foreign key or prefixed with "array:" for an array of foreign keys.
 *
 * @param aValue The string to parse TweakDB property and flat details from.
 * @return A property spec containing the parsed property and flat details, or nullptr if the given string is not
 * a valid TweakDB property type.
 */
TweakTypeSpecPtr GetTweakTypeSpec(const char* aValue);

/**
 * @brief Creates a set of details for a TweakDB property based on the given type hash and optional foreign type name.
 *
 * @param aName The name of the property type, used for RTTI type resolution.
 * @param aForeignType An optional foreign type name for foreign key properties.
 * @return A property spec containing the parsed property and flat details, or nullptr if the given string is not
 * a valid TweakDB property type.
 */
TweakTypeSpecPtr GetTweakTypeSpec(Red::CName aName, const std::optional<std::string>& aForeignType = std::nullopt);

/**
 * @brief Creates a set of details for a TweakDB property based on the given type hash and optional foreign type name.
 *
 * @tparam Type The hash of the property type, used for RTTI type resolution.
 * @param aValue An optional foreign type name for foreign key properties.
 * @return A property spec containing the parsed property and flat details, or nullptr if not valid.
 * @see Red::ERTDBFlatType
 */
template<uint64_t Type>
TweakTypeSpecPtr GetTweakTypeSpec(const std::optional<std::string> aValue = std::nullopt)
{
    static const Core::SharedPtr<TweakTypeSpec> spec = GetTweakTypeSpec(Type, aValue);
    return spec;
}

} // namespace App