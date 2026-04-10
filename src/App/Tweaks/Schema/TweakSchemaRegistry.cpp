#include "TweakSchemaRegistry.hpp"

#include "Red/TweakDB/Schema.hpp"

namespace App
{
using namespace Red;

const TweakDBRecordSchema* TweakSchemaRegistry::GetSchema(const uint32_t hash) const
{
    if (const auto it = m_schemasByHash.find(hash); it != m_schemasByHash.end())
    {
        return it->second;
    }
    return nullptr;
}

const TweakDBRecordSchema* TweakSchemaRegistry::GetSchema(const CName& name) const
{
    if (const auto it = m_schemasByName.find(name); it != m_schemasByName.end())
    {
        return it->second;
    }
    return nullptr;
}

void TweakSchemaRegistry::RegisterSchema(const Core::SharedPtr<TweakDBRecordSchema>& schema)
{
    m_recordSchemas.insert(schema);
    m_schemasByHash[schema->GetHash()] = schema.get();
    m_schemasByName[schema->GetFullName()] = schema.get();
}

const Core::Set<Core::SharedPtr<TweakDBRecordSchema>>& TweakSchemaRegistry::GetSchemas() const
{
    return m_recordSchemas;
}

} // namespace App
