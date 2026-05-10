#include "SchemaChangeset.hpp"

bool App::SchemaChangeset::MakeRecord(const std::string& aName, const std::optional<std::string>& aParent)
{
    const auto name = Red::NormalizeRecordName(aName);

    if (name.empty())
    {
        LogError("{}: Invalid record name.", aName);
        return false;
    }

    Red::CName cname{name.c_str()};

    if (m_pendingRecords.contains(cname))
    {
        LogError("Record {} is already defined; skipping...", name);
        return false;
    }

    if (name == aName)
    {
        LogInfo("Record name \"{}\" is normalized to \"{}\".", aName, name);
    }

    m_pendingRecords.insert({cname, {name, aParent}});

    return true;
}

bool App::SchemaChangeset::MakeProperty(const std::string& aRecordName, const std::string& aPropName,
                                        const TweakTypeSpecPtr& aTypeInfo, const Red::InstancePtr<>& aDefaultValue)
{
    // TODO: validations?

    const auto recordName = Red::NormalizeRecordName(aRecordName);
    const Red::CName recordCName{recordName.c_str()};

    if (!m_pendingRecords.contains(recordCName))
    {
        LogError("Cannot define property {} for record {}, the record is not defined.", aPropName, recordName);
        return false;
    }

    auto& record = m_pendingRecords[recordCName];

    const Red::CName propertyCName{aPropName.c_str()};

    if (record.properties.contains(propertyCName))
    {
        LogError("Property {} of record {} is already defined; skipping...", aPropName, recordName);
        return false;
    }

    record.properties.insert({propertyCName, {aPropName, aTypeInfo, aDefaultValue}});

    return true;
}
void App::SchemaChangeset::Commit(const Core::SharedPtr<ScriptableRecordManager>& aRecordManager,
                                  const Core::SharedPtr<TweakChangelog>& aChangelog)
{
    if (!aRecordManager)
        return;

    if (aChangelog)
    {
        // TODO: run revert logic here.
    }

    for (const auto& record : m_pendingRecords | std::views::values)
    {
        LogDebug("Registering scriptable record schema {}...", record.name);

        const auto recordSpec = aRecordManager->RegisterScriptableRecordType(record.name, record.parent);

        if (!recordSpec)
            continue;

        aChangelog->RegisterSchema(recordSpec->cname);

        for (const auto& property : record.properties | std::views::values)
        {
            aRecordManager->RegisterScriptableProperty(recordSpec, property.name, property.type, property.defaultValue);
        }
    }

    if (aRecordManager->IsRTTIReady())
    {
        const auto specs = aRecordManager->GetRecordSpecs();

        for (auto& spec : specs)
        {
            // TODO: skip soft deleted records? process here?
            LogDebug("Registering RTTI type {}...", spec->name);
            aRecordManager->RegisterRTTIType(spec);
        }

        for (auto& recordSpec : specs)
        {
            LogDebug("Describing RTTI type {}...", recordSpec->name);
            if (!aRecordManager->DescribeRTTIType(recordSpec))
                continue;

            for (auto& propSpec : recordSpec->props | std::views::values)
            {
                LogDebug("Creating functions for property {} of {}...", propSpec->name, recordSpec->name);
                (void)aRecordManager->CreatePropertyFunctions(recordSpec, propSpec);
            }
            // TODO: create property functions for THIS spec
        }

        if (aRecordManager->IsTweakDBReady())
        {
            LogDebug("Inserting default property values for scriptable records into TweakDB...");
            aRecordManager->InsertDefaultValues();
        }
        else
        {
            LogDebug(
                "Insertion of default property values for scriptable records deferred until TweakDB is initialized...");
        }
    }
    else
    {
        LogDebug("Registration of scriptable record RTTI types deferred until RTTI registration is ready...");
    }
}

bool App::SchemaChangeset::IsEmpty() const
{
    return m_pendingRecords.empty();
}
