#pragma once

#include "App/Tweaks/Batch/TweakChangelog.hpp"
#include "App/Tweaks/Batch/TweakChangeset.hpp"
#include "App/Tweaks/TweakContext.hpp"
#include "Core/Logging/LoggingAgent.hpp"
#include "Red/TweakDB/Manager.hpp"
#include "TweakReader.hpp"

namespace App
{
class TweakImporter : Core::LoggingAgent
{
public:
    explicit TweakImporter(const Core::SharedPtr<TweakContext>& aContext);

    void SetManager(const Core::SharedPtr<Red::TweakDBManager>& aManager);

    void Load(const Core::Vector<std::filesystem::path>& aImportPaths);

    void ImportSchemas(const Core::SharedPtr<TweakChangelog>& aChangelog = nullptr, bool aDryRun = false);
    void ImportValues(const Core::SharedPtr<TweakChangelog>& aChangelog = nullptr, bool aDryRun = false);

private:
    Core::SharedPtr<ITweakReader> Load(const std::filesystem::path& aPath, const std::filesystem::path& aDir);

    bool Read(const Core::SharedPtr<TweakChangeset>& aChangeset, const std::filesystem::path& aPath,
              const std::filesystem::path& aDir);
    bool Apply(const Core::SharedPtr<TweakChangeset>& aChangeset, const Core::SharedPtr<TweakChangelog>& aChangelog);

    static bool IsFirstPriority(const std::filesystem::path& aPath);
    static bool IsLastPriority(const std::filesystem::path& aPath);

    Core::SharedPtr<Red::TweakDBManager> m_manager;
    Core::SharedPtr<TweakContext> m_context;

    Core::Vector<Core::SharedPtr<ITweakReader>> m_readers;
};
} // namespace App
