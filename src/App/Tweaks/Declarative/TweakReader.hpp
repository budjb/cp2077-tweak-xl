#pragma once

#include "App/Tweaks/Batch/TweakChangeset.hpp"
#include "App/Tweaks/TweakContext.hpp"

namespace App
{
class ITweakReader
{
public:
    virtual ~ITweakReader() = default;
    virtual void SetManager(const Core::SharedPtr<Red::TweakDBManager>& aManager) = 0;
    virtual bool Load(const std::filesystem::path& aPath) = 0;
    [[nodiscard]] virtual bool IsLoaded() const = 0;
    virtual void Unload() = 0;
    virtual void ReadSchemas(TweakChangeset& aChangeset) = 0;
    virtual void ReadValues(TweakChangeset& aChangeset) = 0;
};

class BaseTweakReader : public ITweakReader
{
public:
    explicit BaseTweakReader(const Core::SharedPtr<TweakContext>& aContext,
                             const Core::SharedPtr<Red::TweakDBManager>& aManager = nullptr);

    void SetManager(const Core::SharedPtr<Red::TweakDBManager>& aManager) override;

protected:
    static std::string ComposePath(const std::string& aParentPath, const std::string& aItemName);
    static std::string ComposePath(const std::string& aParentPath, int32_t aItemIndex);

    static std::string ComposeGroupName(const std::string& aParentName, const std::string& aGroupName);
    static std::string ComposeFlatName(const std::string& aParentName, const std::string& aFlatName);
    std::string ComposeInlineName(const std::string& aParentName, const Red::CClass* aRecordType,
                                  const std::filesystem::path& aSource, int32_t aItemIndex = -1);

    const Red::CBaseRTTIType* ResolveFlatInstanceType(TweakChangeset& aChangeset, Red::TweakDBID aFlatId);
    const Red::CClass* ResolveRecordInstanceType(TweakChangeset& aChangeset, Red::TweakDBID aRecordId);

    bool IsOriginalBaseRecord(Red::TweakDBID aRecordId);

    std::string ToName(const Red::CClass* aType);
    std::string ToName(const Red::CBaseRTTIType* aType, const Red::CClass* aKey = nullptr);

    Core::SharedPtr<Red::TweakDBManager> m_manager;
    Core::SharedPtr<Red::TweakDBReflection> m_reflection;
    Core::SharedPtr<App::TweakContext> m_context;
    Core::Map<std::string, int32_t> m_inlineIndexSuffix;
};
} // namespace App
