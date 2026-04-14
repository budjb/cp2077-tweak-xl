#include "TweakReader.hpp"
#include "App/Utils/Str.hpp"

namespace
{
constexpr auto PathSeparator = ".";
constexpr auto HashSeparator = "|";

constexpr auto GroupSeparator = ".";
constexpr auto PropSeparator = ".";
constexpr auto InlineSeparator = "$";

constexpr auto IndexOpen = "[";
constexpr auto IndexClose = "]";

constexpr auto ForeignKeyOpen = "<";
constexpr auto ForeignKeyClose = ">";
}

App::BaseTweakReader::BaseTweakReader(Core::SharedPtr<Red::TweakDBManager> aManager,
                                      Core::SharedPtr<App::TweakContext> aContext)
    : m_manager(std::move(aManager))
    , m_reflection(m_manager->GetReflection())
    , m_context(std::move(aContext))
{
}

bool App::BaseTweakReader::IsOriginalBaseRecord(Red::TweakDBID aRecordId)
{
    return Red::TweakDBReflection::IsOriginalBaseRecord(aRecordId);
}

std::string App::BaseTweakReader::ComposeGroupName(const std::string& aParentName, const std::string& aGroupName)
{
    if (aParentName.empty())
        return aGroupName;

    if (aGroupName.empty())
        return aParentName;

    auto groupName = aParentName;
    groupName.append(GroupSeparator);
    groupName.append(aGroupName);

    return groupName;
}

std::string App::BaseTweakReader::ComposeFlatName(const std::string& aParentName, const std::string& aFlatName)
{
    if (aParentName.empty())
        return aFlatName;

    if (aFlatName.empty())
        return aParentName;

    auto flatName = aParentName;
    flatName.append(PropSeparator);
    flatName.append(aFlatName);

    return flatName;
}

std::string App::BaseTweakReader::ComposeInlineName(const std::string& aParentName, const Red::CClass* aRecordType,
                                                    const std::filesystem::path& aSource, int32_t aItemIndex)
{
    auto inlineHash = aSource.string();
    inlineHash.append(HashSeparator);
    inlineHash.append(aParentName);
    inlineHash.append(HashSeparator);
    inlineHash.append(aRecordType->name.ToString());

    if (aItemIndex >= 0)
    {
        inlineHash.append(HashSeparator);
        inlineHash.append(std::to_string(aItemIndex));
        inlineHash.append(HashSeparator);
        inlineHash.append(std::to_string(++m_inlineIndexSuffix[inlineHash]));
    }

    auto inlineName = aParentName;
    inlineName.append(InlineSeparator);
    inlineName.append(ToHex(Red::FNV1a32(inlineHash.data(), inlineHash.size())));

    return inlineName;
}

std::string App::BaseTweakReader::ComposePath(const std::string& aParentPath, const std::string& aItemName)
{
    if (aParentPath.empty())
        return aItemName;

    if (aItemName.empty())
        return aParentPath;

    auto itemPath = aParentPath;
    itemPath.append(PathSeparator);
    itemPath.append(aItemName);

    return itemPath;
}

std::string App::BaseTweakReader::ComposePath(const std::string& aParentPath, int32_t aItemIndex)
{
    if (aParentPath.empty())
        return {};

    if (aItemIndex < 0)
        return aParentPath;

    auto itemPath = aParentPath;
    itemPath.append(IndexOpen);
    itemPath.append(std::to_string(aItemIndex));
    itemPath.append(IndexClose);

    return itemPath;
}

const Red::CBaseRTTIType* App::BaseTweakReader::ResolveFlatInstanceType(App::TweakChangeset& aChangeset,
                                                                        Red::TweakDBID aFlatId)
{
    const auto existingFlat = m_manager->GetFlat(aFlatId);
    if (existingFlat.instance)
    {
        return existingFlat.type;
    }

    const auto pendingFlat = aChangeset.GetFlat(aFlatId);
    if (pendingFlat)
    {
        return pendingFlat->type;
    }

    return nullptr;
}

const App::CClassProxy* App::BaseTweakReader::ResolveRecordInstanceType(TweakChangeset& aChangeset,
                                                                   const Red::TweakDBID aRecordId) const
{
    if (!aRecordId.IsValid())
        return nullptr;

    if (const auto existingRecordType = m_manager->GetRecordType(aRecordId))
    {
        return aChangeset.GetClass(existingRecordType);
    }

    if (const auto pendingRecord = aChangeset.GetRecord(aRecordId))
    {
        return aChangeset.GetClass(pendingRecord->type);
    }

    // TODO: create custom cclass proxy for custom types

    return nullptr;
}

std::string App::BaseTweakReader::ToName(const Red::CClass* aType)
{
    return Red::TweakDBReflection::GetRecordShortName<std::string>(aType->GetName());
}

std::string App::BaseTweakReader::ToName(const Red::CBaseRTTIType* aType, const Red::CClass* aKey)
{
    if (!aType)
        return "<UnknownType>";

    std::string name = aType->GetName().ToString();

    if (aKey)
    {
        name.append(ForeignKeyOpen);
        name.append(Red::TweakDBReflection::GetRecordShortName<std::string>(aKey->name));
        name.append(ForeignKeyClose);
    }

    return name;
}
