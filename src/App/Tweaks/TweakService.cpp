#include "TweakService.hpp"
#include "App/Tweaks/Declarative/TweakImporter.hpp"
#include "App/Tweaks/Executable/TweakExecutor.hpp"
#include "App/Tweaks/Metadata/MetadataExporter.hpp"
#include "App/Tweaks/Metadata/MetadataImporter.hpp"
#include "Record/ScriptableRecordManager.hpp"
#include "Red/TweakDB/Raws.hpp"

App::TweakService::TweakService(const Core::SemvVer& aProductVer, std::filesystem::path aGameDir,
                                std::filesystem::path aTweaksDir, std::filesystem::path aInheritanceMapPath,
                                std::filesystem::path aExtraFlatsPath, std::filesystem::path aSourcesDir)
    : m_gameDir(std::move(aGameDir))
    , m_tweaksDir(std::move(aTweaksDir))
    , m_sourcesDir(std::move(aSourcesDir))
    , m_inheritanceMapPath(std::move(aInheritanceMapPath))
    , m_extraFlatsPath(std::move(aExtraFlatsPath))
    , m_productVer(aProductVer)
    , m_manager(nullptr)
    , m_reflection(nullptr)
    , m_recordManager(Core::MakeShared<ScriptableRecordManager>(m_manager))
    , m_changelog(Core::MakeShared<TweakChangelog>())
    , m_context(Core::MakeShared<TweakContext>(aProductVer))
    , m_importer(Core::MakeShared<TweakImporter>(m_manager, m_reflection, m_recordManager, m_context))
{
    m_importPaths.push_back(m_tweaksDir);
}

void App::TweakService::OnBootstrap()
{
    CreateTweaksDir();

    SetupScriptableRecords();

    SetupTweakImporter();

    HookAfter<Raw::TryLoadTweakDB>([&](bool& aSuccess) {
        if (aSuccess)
        {
            m_reflection = Core::MakeShared<Red::TweakDBReflection>(Red::TweakDB::Get());
            m_manager = Core::MakeShared<Red::TweakDBManager>(m_reflection);
            m_executor = Core::MakeShared<App::TweakExecutor>(m_manager, m_reflection);
            m_changelog = Core::MakeShared<App::TweakChangelog>();

            if (ImportMetadata())
            {
                EnsureRuntimeAccess();
                ApplyPatches();
                InsertScriptableRecordDefaults();
                LoadTweaks(false);
            }

#ifndef NDEBUG
            m_recordManager->TestScriptableRecord();
#endif
        }
    });

    HookAfter<Raw::InitTweakDB>([&]() {
        EnsureRuntimeAccess();
        CheckForIssues();
    });

    HookWrap<Raw::CreateRecord>([&](const CreateRecordFunction aOriginal, Red::TweakDB* aTweakDB,
                                    const uint32_t aTypeHash, const Red::TweakDBID aTweakDBID) {
        if (!m_recordManager->CreateScriptableRecord(aTweakDB, aTypeHash, aTweakDBID))
        {
            aOriginal(aTweakDB, aTypeHash, aTweakDBID);
        }
    });
}

void App::TweakService::LoadTweaks(bool aCheckForIssues)
{
    if (m_manager)
    {
        m_importer->ImportValues(m_changelog);
        m_executor->ExecuteTweaks();

        if (aCheckForIssues)
        {
            m_changelog->CheckForIssues(m_manager);
        }
    }
}

void App::TweakService::ImportTweaks()
{
    if (m_manager)
    {
        m_importer->ImportValues(m_changelog);
    }
}

void App::TweakService::ExecuteTweaks()
{
    if (m_manager)
    {
        m_executor->ExecuteTweaks();
    }
}

void App::TweakService::ExecuteTweak(Red::CName aName)
{
    if (m_manager)
    {
        m_executor->ExecuteTweak(aName);
    }
}

void App::TweakService::EnsureRuntimeAccess()
{
    if (m_manager)
    {
        m_manager->GetTweakDB()->unk160 = 0;
    }
}

void App::TweakService::ApplyPatches()
{
    if (m_manager)
    {
        m_manager->CloneRecord("Vendors.IsPresent", "Vendors.Always_Present");
        m_manager->RegisterName("Vendors.IsPresent");
    }
}

void App::TweakService::CheckForIssues()
{
    if (m_manager && m_changelog)
    {
        m_changelog->CheckForIssues(m_manager);
    }
}

void App::TweakService::CreateTweaksDir()
{
    std::error_code error;

    if (!std::filesystem::exists(m_tweaksDir, error))
    {
        if (!std::filesystem::create_directories(m_tweaksDir, error))
        {
            LogWarning("Cannot create tweaks directory \"{}\": {}.",
                       std::filesystem::relative(m_tweaksDir, m_gameDir).string(), error.message());
        }
    }
}

bool App::TweakService::RegisterTweak(std::filesystem::path aPath)
{
    std::error_code error;

    if (aPath.is_relative())
    {
        aPath = m_gameDir / aPath;
    }

    if (!std::filesystem::exists(aPath, error) || !std::filesystem::is_regular_file(aPath, error))
    {
        LogError("Can't register non-existing tweak \"{}\".", std::filesystem::relative(aPath, m_gameDir).string());
        return false;
    }

    m_importPaths.emplace_back(std::move(aPath));
    return true;
}

bool App::TweakService::RegisterDirectory(std::filesystem::path aPath)
{
    std::error_code error;

    if (aPath.is_relative())
    {
        aPath = m_gameDir / aPath;
    }

    if (!std::filesystem::exists(aPath, error) || !std::filesystem::is_directory(aPath, error))
    {
        LogError("Can't register non-existing tweak directory \"{}\".",
                 std::filesystem::relative(aPath, m_gameDir).string());
        return false;
    }

    m_importPaths.emplace_back(std::move(aPath));
    return true;
}

bool App::TweakService::ImportMetadata()
{
    MetadataImporter importer{m_manager, m_reflection};

    LogInfo("Loading inheritance metadata...");

    if (!importer.ImportInheritanceMap(m_inheritanceMapPath))
    {
        LogError("Can't load inheritance metadata from \"{}\".", m_inheritanceMapPath.string());
        return false;
    }

    LogInfo("Loading extra flats metadata...");

    if (!importer.ImportExtraFlats(m_extraFlatsPath))
    {
        LogError("Can't load extra flats metadata from \"{}\".", m_extraFlatsPath.string());
        return false;
    }

    return true;
}

void App::TweakService::ExportMetadata()
{
    MetadataExporter exporter{m_manager, m_reflection};
    exporter.LoadSource(m_sourcesDir);
    exporter.ExportInheritanceMap(m_inheritanceMapPath);
    exporter.ExportExtraFlats(m_extraFlatsPath);
    exporter.ExportInheritanceMap(m_inheritanceMapPath.replace_extension(".yaml"));
    exporter.ExportExtraFlats(m_extraFlatsPath.replace_extension(".yaml"));
}

Core::DeferredPtr<Red::TweakDBManager> App::TweakService::GetManager()
{
    return m_manager;
}

Core::DeferredPtr<Red::TweakDBReflection> App::TweakService::GetReflection()
{
    return m_reflection;
}

Core::DeferredPtr<App::TweakChangelog> App::TweakService::GetChangelog()
{
    return m_changelog;
}

void App::TweakService::InsertScriptableRecordDefaults()
{
    m_recordManager->InsertScriptableRecordDefaults();
}

void App::TweakService::SetupScriptableRecords()
{
    static auto rtti = Red::CRTTISystem::Get();

    rtti->AddPostRegisterCallback(Red::Callback<void (*)()>([] {}));
}

void App::TweakService::SetupTweakImporter()
{
    static auto rtti = Red::CRTTISystem::Get();

    rtti->AddPostRegisterCallback(Red::Callback<void (*)()>{[&]() {
        m_importer->Load(m_importPaths);
        m_importer->ImportSchemas();

        m_recordManager->RegisterScriptableRecordSpecs();
        m_recordManager->DescribeScriptableRecordSpecs();
    }});
}
