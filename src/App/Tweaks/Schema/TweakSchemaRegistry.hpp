#pragma once

#include "Red/TweakDB/Schema.hpp"

namespace App
{

using RecordHash = uint32_t;

class TweakSchemaRegistry
{
public:
    [[nodiscard]] const Red::TweakDBRecordSchema* GetSchema(uint32_t hash) const;
    [[nodiscard]] const Red::TweakDBRecordSchema* GetSchema(const Red::CName& name) const;
    [[nodiscard]] const Core::Set<Core::SharedPtr<Red::TweakDBRecordSchema>>& GetSchemas() const;
    void RegisterSchema(const Core::SharedPtr<Red::TweakDBRecordSchema>& schema);

private:
    Core::Set<Core::SharedPtr<Red::TweakDBRecordSchema>> m_recordSchemas;
    Core::Map<RecordHash, Red::TweakDBRecordSchema*> m_schemasByHash;
    Core::Map<Red::CName, Red::TweakDBRecordSchema*> m_schemasByName;
};

} // namespace App
