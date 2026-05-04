#pragma once

namespace App::Migration
{
inline void CleanUp(const std::filesystem::path& aPath, const std::filesystem::path& aRedscriptExportPath)
{
    std::error_code error;
    if (std::filesystem::exists(aPath, error))
    {
        for (const auto& entry : std::filesystem::directory_iterator(aPath, error))
        {
            if (entry == aRedscriptExportPath)
                continue;

            std::filesystem::remove_all(entry, error);
        }
    }
}
}
