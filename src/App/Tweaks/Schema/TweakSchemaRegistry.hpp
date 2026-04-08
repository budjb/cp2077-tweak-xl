#pragma once

namespace App
{

using namespace Red;
using namespace Red::Schema;

using RecordHash = uint32_t;

class TweakSchemaRegistry
{
public:
    [[nodiscard]] const TweakDBRecordSchema* GetSchema(uint32_t hash) const;
    [[nodiscard]] const TweakDBRecordSchema* GetSchema(const CName& name) const;
    [[nodiscard]] const Core::Set<Core::SharedPtr<TweakDBRecordSchema>>& GetSchemas() const;
    void RegisterSchema(const Core::SharedPtr<TweakDBRecordSchema>& schema);

private:
    Core::Set<Core::SharedPtr<TweakDBRecordSchema>> m_recordSchemas;
    Core::Map<RecordHash, TweakDBRecordSchema*> m_schemasByHash;
    Core::Map<CName, TweakDBRecordSchema*> m_schemasByName;
};

} // namespace App
