#pragma once

#ifndef NDEBUG

#include "App/Tweaks/TweakService.hpp"

namespace App::Tests
{
class ScriptableRecordTestRunner
{
public:
    static Core::SharedPtr<TweakService> GetTweakService();
    static Core::DeferredPtr<Red::TweakDBManager> GetTweakManager();
    static Core::SharedPtr<ScriptableRecordManager> GetRecordManager();
    static Core::SharedPtr<ScriptablePropertyHandler> GetPropertyHandler();
    static int Run(const std::filesystem::path& aDir);

private:
    ScriptableRecordTestRunner() = default;
    static void Setup();

    static inline bool s_isSetup = false;
};
} // namespace App::Tests

#endif
