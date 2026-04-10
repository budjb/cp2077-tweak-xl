#include "YamlReader.hpp"

namespace
{
constexpr auto TypeAttrKey = "$type";
constexpr auto ValueAttrKey = "$value";

constexpr auto LegacyGroupsNodeKey = "groups";
constexpr auto LegacyMembersNodeKey = "members";
constexpr auto LegacyFlatsNodeKey = "flats";
constexpr auto LegacyTypeNodeKey = "type";
constexpr auto LegacyValueNodeKey = "value";
} // namespace

void App::YamlReader::ConvertLegacyNodes()
{
    if (const auto groupsNode = m_data[LegacyGroupsNodeKey]; groupsNode.IsMap())
    {
        for (const auto& groupIt : groupsNode)
        {
            const auto groupKey = groupIt.first;
            const auto groupNode = groupIt.second;

            if (!groupKey.IsDefined() || !groupNode.IsMap())
                continue;

            const auto groupTypeNode = groupNode[LegacyTypeNodeKey];
            const auto groupMembersNode = groupNode[LegacyMembersNodeKey];

            if (!groupTypeNode.IsDefined() || !groupMembersNode.IsMap())
                continue;

            auto convertedNode = YAML::Node();
            convertedNode[TypeAttrKey] = groupTypeNode;

            for (const auto& memberIt : groupMembersNode)
            {
                const auto memberKey = memberIt.first;

                if (const auto memberNode = memberIt.second; !memberKey.IsDefined() || !memberNode.IsMap())
                    continue;

                const auto memberTypeNode = groupNode[LegacyTypeNodeKey];
                const auto memberValueNode = groupNode[LegacyValueNodeKey];

                if (!memberTypeNode.IsDefined() || !memberValueNode.IsDefined())
                    continue;

                convertedNode[memberKey] = memberValueNode;
            }

            m_data[groupKey] = convertedNode;
        }

        m_data.remove(LegacyGroupsNodeKey);
    }

    if (const auto flatsNode = m_data[LegacyFlatsNodeKey]; flatsNode.IsMap())
    {
        for (const auto& flatIt : flatsNode)
        {
            const auto flatKey = flatIt.first;
            const auto flatNode = flatIt.second;

            if (!flatKey.IsDefined() || !flatNode.IsMap())
                continue;

            const auto flatTypeNode = flatNode[LegacyTypeNodeKey];
            const auto flatValueNode = flatNode[LegacyValueNodeKey];

            if (!flatTypeNode.IsDefined() || !flatValueNode.IsDefined())
                continue;

            auto convertedNode = YAML::Node();
            convertedNode[TypeAttrKey] = flatTypeNode;
            convertedNode[ValueAttrKey] = flatValueNode;

            m_data[flatKey] = convertedNode;
        }

        m_data.remove(LegacyFlatsNodeKey);
    }
}
