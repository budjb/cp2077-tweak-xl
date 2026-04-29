#include "RedReader.hpp"

#include "App/Tweaks/Record/ScriptableRecordManager.hpp"
#include "Red/TweakDB/Source/Parser.hpp"

App::RedReader::RedReader(const Core::SharedPtr<TweakContext>& aContext,
                          const Core::SharedPtr<Red::TweakDBManager>& aManager)
    : BaseTweakReader(aContext, aManager)
{
}

bool App::RedReader::Load(const std::filesystem::path& aGroupPath)
{
    m_path = aGroupPath;
    m_source = Red::TweakParser::Parse(aGroupPath);

    return IsLoaded();
}

bool App::RedReader::IsLoaded() const
{
    return m_source.get();
}

void App::RedReader::Unload()
{
    m_path = "";
    m_source.reset();
}

void App::RedReader::ReadSchemas()
{
    if (!IsLoaded())
        return;

    if (!m_source->isSchema)
        return;

    if (!m_source->package.empty())
    {
        m_source->usings.insert(m_source->usings.begin(), m_source->package);
    }

    for (const auto& group : m_source->groups)
    {
        HandleSchemaGroup(group);
    }
}

void App::RedReader::ReadValues(App::TweakChangeset& aChangeset)
{
    if (!IsLoaded())
        return;

    if (m_source->isSchema)
        return;

    if (m_source->isQuery)
    {
        LogError("Query package editing is not supported.");
        return;
    }

    if (!m_source->package.empty())
    {
        m_source->usings.insert(m_source->usings.begin(), m_source->package);
    }

    for (const auto& group : m_source->groups)
    {
        HandleGroup(aChangeset, group, m_source->package, m_source->package);
    }

    if (!m_source->package.empty())
    {
        for (const auto& flat : m_source->flats)
        {
            HandleFlat(aChangeset, flat, m_source->package, m_source->package);
        }
    }
}

void App::RedReader::HandleSchemaGroup(const Red::TweakGroupPtr& aGroup)
{
    if (!CheckConditions(aGroup->tags))
        return;

    if (aGroup->name.empty())
    {
        // TODO: how does this happen? Need to log error.
        return;
    }

    const auto name = Red::TweakDBUtil::NormalizeRecordName(aGroup->name);

    if (name != aGroup->name)
    {
        LogInfo("Normalizing record name {} to {}.", aGroup->name, name);
    }

    const auto parent = !aGroup->base.empty() ? std::optional(aGroup->base) : std::nullopt;

    if (!ScriptableRecordManager::Get()->RegisterScriptableRecordType(name, parent))
        return;

    for (const auto prop : aGroup->flats)
    {
        HandleSchemaProperty(name, prop);
    }
}

void App::RedReader::HandleSchemaProperty(const std::string& aRecordName, const Red::TweakFlatPtr& aFlat)
{
    const auto foreignType = !aFlat->foreignType.empty() ? std::optional(aFlat->foreignType) : std::nullopt;

    // TODO: plug in real original name as foreign name
    const auto propInfo = GetTweakPropertySpec("TODO", GetFlatTypeName(aFlat), foreignType);

    if (!propInfo)
    {
        LogError("{}: Unable to infer type for property {}.", aRecordName, aFlat->name);
        return;
    }

    ScriptableRecordManager::Get()->RegisterScriptableProperty(aRecordName.c_str(), aFlat->name, propInfo,
                                                               MakeValue(propInfo->flatType, aFlat->values));
}

App::RedReader::GroupStatePtr App::RedReader::HandleGroup(App::TweakChangeset& aChangeset,
                                                          const Red::TweakGroupPtr& aGroup,
                                                          const std::string& aParentName,
                                                          const std::string& aParentPath)
{
    if (!CheckConditions(aGroup->tags))
        return {};

    auto groupState = ResolveGroupState(aChangeset, aGroup, aParentName, aParentPath);

    if (!groupState->isResolved)
    {
        LogError("{}: Unknown base group {}.", groupState->groupPath, aGroup->base);

        return groupState;
    }

    if (!groupState->isCompatible)
    {
        if (groupState->isRedefined)
            LogError("{}: Record type {} doesn't match previous definition {}.", groupState->groupPath,
                     ToName(groupState->resolvedType), ToName(groupState->requiredType));
        else
            LogError("{}: Record type {} is not compatible with {}.", groupState->groupPath,
                     ToName(groupState->resolvedType), ToName(groupState->requiredType));
        return groupState;
    }

    if (!groupState->isRecord)
    {
        for (const auto& flat : aGroup->flats)
        {
            HandleFlat(aChangeset, flat, groupState->groupName, groupState->groupPath);
        }
        return groupState;
    }

    const auto recordInfo = m_reflection->GetRecordInfo(groupState->resolvedType);

    if (!recordInfo)
    {
        LogError("{}: Cannot create record, the record type {} is abstract.", groupState->groupPath,
                 ToName(groupState->resolvedType));
        return groupState;
    }

    if (groupState->recordId == groupState->sourceId)
    {
        LogError("{}: Cannot clone {} from itself.", groupState->groupPath, groupState->groupName);
        return groupState;
    }

    aChangeset.MakeRecord(groupState->recordId, groupState->resolvedType, groupState->sourceId);
    aChangeset.RegisterName(groupState->recordId, groupState->groupName);

    for (const auto& flat : aGroup->flats)
    {
        const auto propInfo = recordInfo->GetPropInfo(flat->name.c_str());

        if (propInfo)
        {
            auto flatState = HandleFlat(aChangeset, flat, groupState->groupName, groupState->groupPath, propInfo->type,
                                        propInfo->foreignType);

            if (flatState && flatState->isProcessed && groupState->isOriginalBase)
            {
                aChangeset.ReinheritFlat(flatState->flatId, groupState->recordId, propInfo->appendix);
            }
        }
        else
        {
            HandleFlat(aChangeset, flat, groupState->groupName, groupState->groupPath);
        }
    }

    groupState->isProcessed = true;

    return groupState;
}

App::RedReader::GroupStatePtr App::RedReader::HandleInline(App::TweakChangeset& aChangeset,
                                                           const Red::TweakGroupPtr& aGroup,
                                                           const std::string& aParentName,
                                                           const std::string& aParentPath,
                                                           const Red::CClass* aRequiredType, int32_t aInlineIndex)
{
    auto inlineState = ResolveGroupState(aChangeset, aGroup, aParentName, aParentPath, aRequiredType, aInlineIndex);

    if (!inlineState->isResolved)
    {
        LogError("{}: Unknown base group {}.", inlineState->groupPath, aGroup->base);

        return inlineState;
    }

    if (!inlineState->isCompatible)
    {
        if (inlineState->isRedefined)
            LogError("{}: Record type {} doesn't match previous definition {}.", inlineState->groupPath,
                     ToName(inlineState->resolvedType), ToName(inlineState->requiredType));
        else
            LogError("{}: Record type {} is not compatible with {}.", inlineState->groupPath,
                     ToName(inlineState->resolvedType), ToName(inlineState->requiredType));

        return inlineState;
    }

    const auto recordInfo = m_reflection->GetRecordInfo(inlineState->resolvedType);

    if (!recordInfo)
    {
        LogError("{}: Cannot create record, the record type {} is abstract.", inlineState->groupPath,
                 ToName(inlineState->resolvedType));

        return inlineState;
    }

    inlineState->groupName = ComposeInlineName(aParentName, inlineState->resolvedType, m_path, aInlineIndex);
    inlineState->recordId = Red::TweakDBID(inlineState->groupName);

    aChangeset.MakeRecord(inlineState->recordId, inlineState->resolvedType, inlineState->sourceId);
    aChangeset.RegisterName(inlineState->recordId, inlineState->groupName);

    {
        FlatStatePtr flatState;

        for (const auto& flat : aGroup->flats)
        {
            const auto propInfo = recordInfo->GetPropInfo(flat->name.c_str());

            if (propInfo)
            {
                flatState = HandleFlat(aChangeset, flat, inlineState->groupName, inlineState->groupPath, propInfo->type,
                                       propInfo->foreignType);
            }
            else
            {
                flatState = HandleFlat(aChangeset, flat, inlineState->groupName, inlineState->groupPath);
            }

            if (!flatState)
                continue;

            if (!flatState->isProcessed)
                return inlineState;
        }
    }

    inlineState->isProcessed = true;

    return inlineState;
}

App::RedReader::FlatStatePtr App::RedReader::HandleFlat(App::TweakChangeset& aChangeset, const Red::TweakFlatPtr& aFlat,
                                                        const std::string& aParentName, const std::string& aParentPath,
                                                        const Red::CBaseRTTIType* aRequiredType,
                                                        const Red::CClass* aForeignType)
{
    if (!CheckConditions(aFlat->tags))
        return {};

    auto flatState = ResolveFlatState(aChangeset, aFlat, aParentName, aParentPath, aRequiredType, aForeignType);

    if (!flatState->isResolved)
    {
        LogError("{}: Missing type declaration.", flatState->flatPath);

        return flatState;
    }

    if (!flatState->isCompatible)
    {
        LogError("{}: Type {} doesn't match previous definition {}.", flatState->flatPath,
                 ToName(flatState->resolvedType, flatState->resolvedKey),
                 ToName(flatState->requiredType, flatState->requiredKey));

        return flatState;
    }

    if (!aRequiredType)
    {
        aChangeset.RegisterName(flatState->flatId, flatState->flatName);
    }

    if (flatState->isForeignKey)
    {
        auto index = 0;

        for (const auto& value : aFlat->values)
        {
            if (value->type == Red::ETweakValueType::Inline)
            {
                auto inlineState = HandleInline(aChangeset, value->group, flatState->flatName, flatState->flatPath,
                                                flatState->resolvedKey, (flatState->isArray ? index : -1));

                if (!inlineState->isProcessed)
                    return flatState;

                value->type = Red::ETweakValueType::String;
                value->data.emplace_back(inlineState->groupName);
            }

            ++index;
        }
    }

    if (aFlat->operation != Red::ETweakFlatOp::Assign)
    {
        if (!flatState->isArray)
        {
            LogError("{}: Compound operations are only supported for array types.", flatState->flatPath);

            return flatState;
        }

        auto index = 0;

        for (const auto& value : aFlat->values)
        {
            const auto flatValue = MakeValue(flatState, value);

            if (!flatValue)
            {
                LogError("{}: Invalid value, expected \"{}\".", ComposePath(flatState->flatPath, index),
                         ToName(flatState->resolvedType));

                return flatState;
            }

            if (aFlat->operation == Red::ETweakFlatOp::Append)
            {
                aChangeset.AppendElement(flatState->flatId, flatState->elementType, flatValue);
            }
            else if (aFlat->operation == Red::ETweakFlatOp::Remove)
            {
                aChangeset.RemoveElement(flatState->flatId, flatState->elementType, flatValue);
            }

            ++index;
        }
    }
    else
    {
        const auto flatValue = MakeValue(flatState, aFlat->values);

        if (!flatValue)
        {
            LogError("{}: Invalid value, expected \"{}\".", flatState->flatPath, ToName(flatState->resolvedType));

            return flatState;
        }

        aChangeset.SetFlat(flatState->flatId, flatState->resolvedType, flatValue);
    }

    flatState->isProcessed = true;

    return flatState;
}

App::RedReader::GroupStatePtr App::RedReader::ResolveGroupState(App::TweakChangeset& aChangeset,
                                                                const Red::TweakGroupPtr& aGroup,
                                                                const std::string& aParentName,
                                                                const std::string& aParentPath,
                                                                const Red::CClass* aBaseType, int32_t aInlineIndex)
{
    auto state = Core::MakeShared<GroupState>();

    if (!aGroup->name.empty())
    {
        state->groupPath = ComposePath(aParentPath, aGroup->name);
        state->groupName = ComposeGroupName(aParentName, aGroup->name);
        state->recordId = state->groupName;
    }
    else
    {
        state->groupPath = ComposePath(aParentPath, aInlineIndex);
    }

    const auto instanceType = ResolveRecordInstanceType(aChangeset, state->recordId);

    if (aGroup->base.empty())
    {
        state->isResolved = true;

        if (instanceType)
        {
            state->isRecord = true;
            state->isRedefined = true;
            state->isCompatible = !aBaseType || instanceType->IsA(aBaseType);
            state->requiredType = aBaseType;
            state->resolvedType = instanceType;
        }
        else if (aBaseType)
        {
            state->isRecord = true;
            state->isCompatible = true;
            state->requiredType = aBaseType;
            state->resolvedType = aBaseType;
        }
        else
        {
            state->isCompatible = true;
        }
    }
    else
    {
        state->resolvedType = Red::TweakDBUtil::GetRecordType(aGroup->base.c_str());

        if (state->resolvedType)
        {
            state->isResolved = true;
            state->isRecord = true;
        }
        else
        {
            state->sourceId = aGroup->base;
            state->resolvedType = ResolveRecordInstanceType(aChangeset, state->sourceId);

            if (state->resolvedType)
            {
                state->isResolved = true;
                state->isRecord = true;
            }
            else
            {
                for (const auto& package : m_source->usings)
                {
                    if (package == Red::TweakSource::SchemaPackage)
                        continue;

                    state->sourceId = ComposeGroupName(package, aGroup->base);
                    state->resolvedType = ResolveRecordInstanceType(aChangeset, state->sourceId);

                    if (state->resolvedType)
                    {
                        state->isResolved = true;
                        state->isRecord = true;
                        break;
                    }
                }

                // TODO: Actual free group inheritance
                if (!state->isResolved)
                {
                    for (const auto& group : m_source->groups)
                    {
                        if (group->name == aGroup->base)
                        {
                            state->isResolved = true;
                            state->isCompatible = true;
                            break;
                        }
                    }
                }
            }
        }

        if (state->isResolved && state->isRecord)
        {
            if (instanceType)
            {
                state->isRedefined = true;
                state->isCompatible = state->resolvedType == instanceType;
                state->requiredType = instanceType;
            }
            else if (aBaseType)
            {
                state->isCompatible = state->resolvedType->IsA(aBaseType);
                state->requiredType = aBaseType;
            }
            else
            {
                state->isCompatible = true;
                state->requiredType = state->resolvedType;
            }

            state->isOriginalBase = IsOriginalBaseRecord(state->recordId);
        }
    }

    return state;
}

App::RedReader::FlatStatePtr App::RedReader::ResolveFlatState(
    App::TweakChangeset& aChangeset, const Red::TweakFlatPtr& aFlat, const std::string& aParentName,
    const std::string& aParentPath, const Red::CBaseRTTIType* aRequiredType, const Red::CClass* aForeignType)
{
    auto state = Core::MakeShared<FlatState>();

    state->flatPath = ComposeFlatName(aParentPath, aFlat->name);
    state->flatName = ComposeFlatName(aParentName, aFlat->name);
    state->flatId = state->flatName;

    const auto instanceType = ResolveFlatInstanceType(aChangeset, state->flatId);

    if (aFlat->type == Red::ETweakFlatType::Undefined)
    {
        state->requiredType = aRequiredType;
        state->requiredKey = aForeignType;
        state->resolvedKey = aForeignType;

        if (instanceType)
        {
            state->isRedefined = true;
            state->resolvedType = instanceType;
        }
        else if (aRequiredType)
        {
            state->resolvedType = aRequiredType;
        }

        if (state->resolvedType)
        {
            state->isResolved = true;
            state->isCompatible = !state->requiredType || state->resolvedType == state->requiredType;
            state->isArray = Red::TweakDBUtil::IsArrayType(state->resolvedType);
            state->isForeignKey = state->isArray ? Red::TweakDBUtil::IsForeignKeyArray(state->resolvedType)
                                                 : Red::TweakDBUtil::IsForeignKey(state->resolvedType);
        }
    }
    else
    {
        state->resolvedType = Red::TweakDBUtil::GetFlatType(GetFlatTypeName(aFlat));

        if (state->resolvedType)
        {
            state->isResolved = true;
            state->isArray = Red::TweakDBUtil::IsArrayType(state->resolvedType);
            state->isForeignKey = state->isArray ? Red::TweakDBUtil::IsForeignKeyArray(state->resolvedType)
                                                 : Red::TweakDBUtil::IsForeignKey(state->resolvedType);

            if (instanceType)
            {
                state->isRedefined = true;
                state->requiredType = instanceType;
            }
            else if (aRequiredType)
            {
                state->requiredType = aRequiredType;
            }
            else
            {
                state->requiredType = state->resolvedType;
            }

            state->isCompatible = state->resolvedType == state->requiredType;

            if (state->isForeignKey)
            {
                state->requiredKey = aForeignType;
                state->resolvedKey = Red::TweakDBUtil::GetRecordType(aFlat->foreignType.c_str());

                if (state->isCompatible && state->requiredKey)
                {
                    state->isCompatible = state->resolvedKey && state->resolvedKey->IsA(state->requiredKey);
                }
            }
        }
    }

    if (state->isArray)
    {
        state->elementType = Red::TweakDBUtil::GetElementType(state->resolvedType);
    }

    return state;
}

bool App::RedReader::CheckConditions(const Core::Vector<std::string>& aTags)
{
    if (!aTags.empty())
    {
        for (const auto& tag : aTags)
        {
            if (tag == "EP1")
            {
                return m_context->CheckInstalledDLC(tag);
            }
        }
    }

    return true;
}
