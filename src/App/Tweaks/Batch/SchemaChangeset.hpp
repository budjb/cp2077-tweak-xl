#pragma once
#include "App/Tweaks/Record/ScriptableRecordManager.hpp"
#include "App/Tweaks/TweakTypeSpec.hpp"
#include "Core/Logging/LoggingAgent.hpp"
#include "TweakChangelog.hpp"

namespace App
{
class SchemaChangeset
    : public Core::LoggingAgent
    , public Core::ShareFromThis<SchemaChangeset>
{
public:
    struct PropertyEntry
    {
        const std::string name;
        const TweakTypeSpecPtr type;
        const Red::InstancePtr<> defaultValue;
    };

    struct RecordEntry
    {
        const std::string name;
        const std::optional<std::string> parent;

        Core::Map<Red::CName, PropertyEntry> properties;
    };

    bool MakeRecord(const std::string& aName, const std::optional<std::string>& aParent);
    bool MakeProperty(const std::string& aRecordName, const std::string& aPropName, const TweakTypeSpecPtr& aTypeInfo,
                      const Red::InstancePtr<>& aDefaultValue);

    void Commit(const Core::SharedPtr<ScriptableRecordManager>& aRecordManager,
                const Core::SharedPtr<TweakChangelog>& aChangelog);

    bool IsEmpty() const;

private:
    Core::Map<Red::CName, RecordEntry> m_pendingRecords;
};
} // namespace App
