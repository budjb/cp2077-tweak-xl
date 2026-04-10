#include "Reflection.hpp"

namespace Red
{

#pragma region TweakDBPropertySchema

#pragma region TweakDBPropertySchema::Builder

TweakDBPropertySchema::Builder::Builder(std::string name)
    : m_name(std::move(name))
{
}

TweakDBPropertySchema::Builder& TweakDBPropertySchema::Builder::FlatType(const TDBFlatType& aFlatType)
{
    this->m_flatType = const_cast<TDBFlatType*>(&aFlatType);
    return *this;
}

TweakDBPropertySchema::Builder& TweakDBPropertySchema::Builder::Offset(const uintptr_t aOffset)
{
    this->m_offset = aOffset;
    return *this;
}

TweakDBPropertySchema::Builder& TweakDBPropertySchema::Builder::ForeignClass(const ResolvableType& aClass)
{
    this->m_foreignClass = aClass;
    return *this;
}

TweakDBPropertySchema::Builder& TweakDBPropertySchema::Builder::PropertyStorage(const Red::PropertyStorage aOwnership)
{
    this->m_propertyStorage = aOwnership;
    return *this;
}

TweakDBPropertySchema::Builder& TweakDBPropertySchema::Builder::DefaultValueOffset(int32_t aOffset)
{
    this->m_defaultValueOffset = aOffset;
    this->m_defaultValue = nullptr;
    return *this;
}

TweakDBPropertySchema::Builder& TweakDBPropertySchema::Builder::DefaultValue(void* aValue)
{
    this->m_defaultValueOffset = 0;
    this->m_defaultValue = Core::MakeUnique<void*>(aValue);
    return *this;
}

TweakDBPropertySchema::PropertyPtr TweakDBPropertySchema::Builder::Build() const
{
    return Core::SharedPtr<TweakDBPropertySchema>(new TweakDBPropertySchema(
        m_name, *m_flatType, m_offset, m_propertyStorage, m_defaultValue, m_foreignClass, m_defaultValueOffset));
}

#pragma endregion

const CName& TweakDBPropertySchema::GetName() const
{
    return name;
}

const std::string& TweakDBPropertySchema::GetFlatSuffix() const
{
    return flatSuffix;
}

const CName& TweakDBPropertySchema::GetFunctionName() const
{
    return functionName;
}

const TDBFlatType& TweakDBPropertySchema::GetType() const
{
    return type;
}

uintptr_t TweakDBPropertySchema::GetOffset() const
{
    return m_offset;
}

const ResolvableType& TweakDBPropertySchema::GetForeignClass() const
{
    return m_foreignClass;
}

PropertyStorage TweakDBPropertySchema::GetPropertyStorage() const
{
    return m_propertyStorage;
}

const Core::SharedPtr<void*>& TweakDBPropertySchema::GetDefaultValue() const
{
    return m_defaultValue;
}

int32_t TweakDBPropertySchema::GetDefaultValueOffset() const
{
    return m_defaultValueOffset;
}

TweakDBPropertySchema::TweakDBPropertySchema(const std::string& aName, const TDBFlatType& aType,
                                             const uintptr_t aOffset, const PropertyStorage aPropertyStorage,
                                             const Core::SharedPtr<void*>& aDefaultValue,
                                             const ResolvableType& aForeignClass, const int32_t aDefaultValueOffset)
    : name(aName | ToCName)
    , flatSuffix(std::move("." + aName))
    , functionName(aName | Capitalize | ToCName)
    , type(aType)
    , m_offset(aOffset)
    , m_propertyStorage(aPropertyStorage)
    , m_defaultValue(aDefaultValue)
    , m_foreignClass(aForeignClass)
    , m_defaultValueOffset(aDefaultValueOffset)
{
}

TweakDBPropertySchema::PropertyPtr TweakDBPropertySchema::From(const ExtraFlat& aExtraFlat)
{
    const TDBFlatType* flatType = TDBFlatType::Get(aExtraFlat.flatType);

    if (!flatType)
    {
        // TODO: log warning
        return nullptr;
    }

    auto propBuilder = Builder(aExtraFlat.name);
    propBuilder.PropertyStorage(PropertyStorage::FLAT);
    propBuilder.FlatType(*flatType);

    if (flatType->IsForeignKey())
    {
        if (aExtraFlat.foreignTypeName.IsNone())
        {
            // TODO: log warning
            return nullptr;
        }

        propBuilder.ForeignClass(ResolvableType(aExtraFlat.foreignTypeName));
    }

    return propBuilder.Build();
}

#pragma endregion

#pragma region TweakDBRecordSchema

#pragma region Builder

TweakDBRecordSchema::Builder::Builder(const CClass* aType)
    : m_class(ResolvableType(aType))
{
}

TweakDBRecordSchema::Builder::Builder(const CName& aName)
    : m_class(ResolvableType(aName))
{
}

TweakDBRecordSchema::Builder::Builder(const std::string& aName)
    : m_class(ResolvableType(aName))
{
}

TweakDBRecordSchema::Builder& TweakDBRecordSchema::Builder::SchemaType(const ESchemaType aType)
{
    this->m_schemaType = aType;
    return *this;
}

TweakDBRecordSchema::Builder& TweakDBRecordSchema::Builder::Parent(const CClass* aParent)
{
    this->m_parentClass = ResolvableType(aParent);
    return *this;
}

TweakDBRecordSchema::Builder& TweakDBRecordSchema::Builder::Parent(const std::string& aParent)
{
    this->m_parentClass = ResolvableType(aParent);
    return *this;
}

TweakDBRecordSchema::Builder& TweakDBRecordSchema::Builder::Parent(const TweakDBRecordSchema& aParent)
{
    this->m_parentClass = aParent.GetClass();

    for (const auto prop : aParent.GetProperties() | std::views::values)
        this->m_properties.push_back(prop);

    return *this;
}

TweakDBRecordSchema::Builder& TweakDBRecordSchema::Builder::Property(const PropertyPtr& aProperty)
{
    m_properties.push_back(aProperty);
    return *this;
}

TweakDBRecordSchema::SchemaPtr TweakDBRecordSchema::Builder::Build() const
{
    return Core::SharedPtr<TweakDBRecordSchema>(
        new TweakDBRecordSchema(m_class, m_parentClass, m_schemaType, m_properties));
}

TweakDBRecordSchema::Builder& TweakDBRecordSchema::Builder::operator+=(const TweakDBRecordSchema& other)
{
    return this->Parent(other);
}

#pragma endregion

TweakDBRecordSchema::TweakDBRecordSchema(const ResolvableType& aClass, const ResolvableType& aParentClass,
                                         const ESchemaType& aType, const PropertiesList& aProperties)
    : m_fullName(TweakDBReflection::GetRecordFullName(const_cast<ResolvableType&>(aClass).GetName()) | ToCName)
    , m_aliasName(TweakDBReflection::GetRecordAliasName(const_cast<ResolvableType&>(aClass).GetName()) | ToCName)
    , m_shortName(TweakDBReflection::GetRecordShortName(const_cast<ResolvableType&>(aClass).GetName()) | ToCName)
    , m_class(aClass)
    , m_parentClass(aParentClass)
    , m_hash(TweakDBReflection::GetRecordTypeHash(m_shortName))
    , m_schemaType(aType)
    , m_properties(ToMap(aProperties, [](const PropertyPtr& p) -> const CName& { return p->GetName(); }))
{
}

const ResolvableType& TweakDBRecordSchema::GetClass() const
{
    return m_class;
}

const CName& TweakDBRecordSchema::GetFullName() const
{
    return m_fullName;
}

const CName& TweakDBRecordSchema::GetAliasName() const
{
    return m_aliasName;
}

const CName& TweakDBRecordSchema::GetShortName() const
{
    return m_shortName;
}

const ResolvableType& TweakDBRecordSchema::GetParentClass() const
{
    return m_parentClass;
}

uint32_t TweakDBRecordSchema::GetHash() const
{
    return m_hash;
}

ESchemaType TweakDBRecordSchema::GetSchemaType() const
{
    return m_schemaType;
}

TweakDBRecordSchema::PropertyPtr TweakDBRecordSchema::GetProperty(const CName& aName) const
{
    if (const auto it = m_properties.find(aName); it != m_properties.end())
        return it->second;

    return nullptr;
}

TweakDBRecordSchema::PropertyPtr TweakDBRecordSchema::GetProperty(const char* aName) const
{
    return GetProperty(CName(aName));
}

const TweakDBRecordSchema::PropertiesMap& TweakDBRecordSchema::GetProperties() const
{
    return m_properties;
}

#pragma endregion

} // namespace Red
